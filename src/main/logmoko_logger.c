#include "logmoko.h"

extern struct lmk_list g_lmk_logger_list;

static int lmk_init_logger(struct lmk_logger *logger);

extern int lmk_init_log_handler(struct lmk_log_handler *handler);

int lmk_log_impl(struct lmk_logger *logger, const char *file_name, int line_no,
                 int log_level, const char *data, size_t data_len);

/* Per-thread format buffer — eliminates the shared logger->log_buff and its mutex. */
static _Thread_local char lmk_tls_log_buf[LMK_LOG_MSG_MAX_SIZE];

LMK_API struct lmk_logger *lmk_get_logger(const char *name) {
    struct lmk_logger *logger = NULL;
    lmk_init();
    if (!name)
        return NULL;
    if ((logger = lmk_search_logger_by_name(name)) != NULL)
        return logger;
    if ((logger = (struct lmk_logger *)lmk_malloc(sizeof(struct lmk_logger))) != NULL) {
        lmk_init_logger(logger);
        int name_len = strlen(name);
        if ((logger->name = lmk_malloc(name_len + 1)) == NULL) {
            lmk_free(logger);
            return NULL;
        }
        strncpy((void *)logger->name, name, name_len);
        logger->name[name_len] = '\0';
        lmk_init_list(&logger->handler_ref_list);
        lmk_insert_list(&g_lmk_logger_list, &logger->link);
    }
    return logger;
}

static int lmk_init_logger(struct lmk_logger *logger) {
    if (!logger)
        return LMK_E_NG;
    if (!logger->initialized) {
        memset(logger, 0, sizeof(struct lmk_logger));
        lmk_init_list(&logger->link);
        lmk_init_list(&logger->handler_ref_list);
        logger->log_level = LMK_LOG_LEVEL_INFO;
        logger->initialized = 1;
    }
    return LMK_E_OK;
}

LMK_API int lmk_destroy_logger(struct lmk_logger **logger_addr) {
    struct lmk_list *cursor = NULL;
    lmk_init();
    if (!logger_addr || !*logger_addr)
        return LMK_E_NG;
    struct lmk_logger *logger = *logger_addr;

#if LMK_DEBUG
    fprintf(stdout, "Destroying logger %s (%p)\n", logger->name, logger);
#endif

    LMK_FOR_EACH_ENTRY(&logger->handler_ref_list, cursor) {
        struct lmk_log_handler_ref *handler_ref = (struct lmk_log_handler_ref *)cursor;
        LMK_SAVE_CURSOR(cursor)
            lmk_detach_log_handler(logger, handler_ref->handler);
        LMK_RESTORE_CURSOR(cursor)
    }
    logger->initialized = 0;
    lmk_remove_list(&logger->link);
    if (logger->name != NULL)
        lmk_free((void *)logger->name);
    lmk_free(logger);
    *logger_addr = NULL;
    return LMK_E_OK;
}

LMK_API void lmk_set_log_level(struct lmk_logger *logger, int log_level) {
    lmk_init();
    if (!logger)
        return;
    if (log_level >= LMK_LOG_LEVEL_TRACE && log_level <= LMK_LOG_LEVEL_OFF)
        logger->log_level = log_level;
}

LMK_API int lmk_get_log_level(struct lmk_logger *logger) {
    lmk_init();
    return logger ? logger->log_level : LMK_LOG_LEVEL_UNKNOWN;
}

struct lmk_log_handler_ref *lmk_search_log_handler_ref(struct lmk_logger *logger,
                                                         struct lmk_log_handler *handler) {
    if (logger && handler) {
        struct lmk_list *cursor = NULL;
        LMK_FOR_EACH_ENTRY(&logger->handler_ref_list, cursor) {
            struct lmk_log_handler_ref *ref = (struct lmk_log_handler_ref *)cursor;
            if (ref->handler == handler)
                return ref;
        }
    }
    return NULL;
}

struct lmk_log_handler_ref *lmk_search_log_handler_ref_by_name(struct lmk_logger *logger,
                                                                  const char *name) {
    struct lmk_list *cursor = NULL;
    if (!logger || !name)
        return NULL;
    LMK_FOR_EACH_ENTRY(&logger->handler_ref_list, cursor) {
        struct lmk_log_handler_ref *ref = (struct lmk_log_handler_ref *)cursor;
        if (!strcmp(ref->handler->name, name))
            return ref;
    }
    return NULL;
}

int lmk_is_log_handler_attached(struct lmk_logger *logger, struct lmk_log_handler *handler) {
    return (lmk_search_log_handler_ref(logger, handler) != NULL) ? 1 : 0;
}

LMK_API int lmk_attach_log_handler(struct lmk_logger *logger, struct lmk_log_handler *handler) {
    struct lmk_log_handler_ref *handler_ref = NULL;
    lmk_init();
    if (!logger || !handler || !handler->initialized)
        return LMK_E_NG;
    if (lmk_is_log_handler_attached(logger, handler))
        return LMK_E_OK;
    if ((handler_ref = (struct lmk_log_handler_ref *)lmk_malloc(
            sizeof(struct lmk_log_handler_ref))) == NULL)
        return LMK_E_NG;
    handler_ref->handler = handler;
    lmk_init_list(&handler_ref->link);
    lmk_insert_list(&logger->handler_ref_list, &handler_ref->link);
    handler->nr_logger_ref++;
    return LMK_E_OK;
}

LMK_API int lmk_detach_log_handler(struct lmk_logger *logger, struct lmk_log_handler *handler) {
    struct lmk_log_handler_ref *handler_ref = NULL;
    lmk_init();
    if (!handler || !logger)
        return LMK_E_NG;

#if LMK_DEBUG
    fprintf(stdout, "Detaching handler %s from logger %s\n", handler->name, logger->name);
#endif

    handler_ref = lmk_search_log_handler_ref(logger, handler);
    if (!handler_ref)
        return LMK_E_NG;
    lmk_remove_list(&handler_ref->link);
    lmk_free(handler_ref);
    handler->nr_logger_ref--;
    return LMK_E_OK;
}

/* Shared helper: format into TLS buffer and forward to all attached handlers. */
static inline void lmk_log_common(struct lmk_logger *logger, const char *file_name,
                                   int line_no, int log_level,
                                   const char *format, va_list ap) {
    lmk_init();
    if (!logger || !LMK_IS_LOG_ENABLED(logger, log_level))
        return;
    int n = vsnprintf(lmk_tls_log_buf, LMK_LOG_MSG_MAX_SIZE, format, ap);
    if (n < 0) n = 0;
    if (n >= LMK_LOG_MSG_MAX_SIZE) n = LMK_LOG_MSG_MAX_SIZE - 1;
    lmk_log_impl(logger, file_name, line_no, log_level, lmk_tls_log_buf, (size_t)n);
}

LMK_API void lmk_log_trace(struct lmk_logger *logger, const char *file_name,
                            const int line_no, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    lmk_log_common(logger, file_name, line_no, LMK_LOG_LEVEL_TRACE, format, ap);
    va_end(ap);
}

LMK_API void lmk_log_debug(struct lmk_logger *logger, const char *file_name,
                            const int line_no, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    lmk_log_common(logger, file_name, line_no, LMK_LOG_LEVEL_DEBUG, format, ap);
    va_end(ap);
}

LMK_API void lmk_log_info(struct lmk_logger *logger, const char *file_name,
                           const int line_no, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    lmk_log_common(logger, file_name, line_no, LMK_LOG_LEVEL_INFO, format, ap);
    va_end(ap);
}

LMK_API void lmk_log_warn(struct lmk_logger *logger, const char *file_name,
                           const int line_no, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    lmk_log_common(logger, file_name, line_no, LMK_LOG_LEVEL_WARN, format, ap);
    va_end(ap);
}

LMK_API void lmk_log_error(struct lmk_logger *logger, const char *file_name,
                            const int line_no, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    lmk_log_common(logger, file_name, line_no, LMK_LOG_LEVEL_ERROR, format, ap);
    va_end(ap);
}

int lmk_log_impl(struct lmk_logger *logger, const char *file_name, int line_no,
                 int log_level, const char *data, size_t data_len) {
    struct lmk_list *cursor = NULL;
    struct lmk_log_handler *handler = NULL;
    struct lmk_log_record log_record;

    if (!logger || !logger->initialized || !data)
        return LMK_E_NG;
    if (!(log_level >= LMK_LOG_LEVEL_TRACE && log_level <= LMK_LOG_LEVEL_OFF))
        return LMK_E_NG;

    if (log_level >= logger->log_level) {
        log_record.log_level = log_level;
        log_record.data      = (char *)data;
        log_record.data_len  = data_len;
        log_record.line_no   = line_no;
        log_record.file_name = (char *)file_name;

        LMK_FOR_EACH_ENTRY(&logger->handler_ref_list, cursor) {
            handler = ((struct lmk_log_handler_ref *)cursor)->handler;
            if (log_level >= handler->log_level)
                handler->log_impl(handler, &log_record);
        }
    }
    return LMK_E_OK;
}

struct lmk_log_handler *lmk_find_handler(struct lmk_logger *logger, const char *handler_name) {
    struct lmk_list *cursor = NULL;
    if (logger && handler_name) {
        LMK_FOR_EACH_ENTRY(&logger->handler_ref_list, cursor) {
            struct lmk_log_handler *handler = ((struct lmk_log_handler_ref *)cursor)->handler;
            if (!strcmp(handler_name, handler->name))
                return handler;
        }
    }
    return NULL;
}
