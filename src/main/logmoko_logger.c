/*
 * Logmoko - A logging framework for C.
 * Copyright (C) 2013 Jerrico Gamis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "logmoko.h"

extern lmk_list g_lmk_logger_list;

extern int lmk_init_logger(lmk_logger *logger);
extern int lmk_init_log_handler(lmk_log_handler *handler);
int lmk_log_impl(lmk_logger *logger, const char *file_name, const int line_no,
        int log_level, const char *data);

/**
 *  @brief Retrieves a named logger or creates a new one if non-existent
 *  @brief The logger
 */
LMK_API lmk_logger * lmk_get_logger(const char *name) {
    lmk_logger *logger = NULL;
    lmk_init();
    if (name != NULL) {
        if ((logger = lmk_srch_logger_by_name(name)) != NULL) {
            return logger;
        }
        if ((logger = (lmk_logger*) lmk_malloc(sizeof (lmk_logger))) != NULL) {
            lmk_init_logger(logger);
            logger->name = name;
            lmk_init_list(&logger->handler_ref_list);
            lmk_insert_list(&g_lmk_logger_list, &logger->link);
        }
    }
    return logger;
}

/**
 *  @brief Initializes a logger
 *  @return Operation error code (LMK_E_OK, LMK_E_NG)
 */

int lmk_init_logger(lmk_logger *logger) {
    if (logger != NULL) {
        if (!logger->initialized) {
            memset(logger, 0x00, sizeof (lmk_logger));
            lmk_init_list(&logger->link);
            lmk_init_list(&logger->handler_ref_list);
            memset(logger->log_buff, 0, LMK_LOG_BUFFER_SIZE);
            logger->log_level = LMK_LOG_LEVEL_TRACE;
            logger->initialized = 1;
            LMK_INIT_MUTEX(logger->log_lock);
        }
    }
    return LMK_E_OK;
}

LMK_API int lmk_destroy_logger(lmk_logger **logger_addr) {
    lmk_list *cursor = NULL;
    lmk_init();
    if (logger_addr == NULL || *logger_addr == NULL) {
        return LMK_E_NG;
    }
    lmk_logger *logger = *logger_addr;
#if LMK_DEBUG
    fprintf(stdout, "Destroying logger %s (%p)\n", logger->name, logger);
#endif

    LMK_FOR_EACH_ENTRY(&logger->handler_ref_list, cursor) {
        lmk_log_handler_ref * handler_ref = (lmk_log_handler_ref*) cursor;
        LMK_SAVE_CURSOR(cursor)
        lmk_detach_log_handler(logger, handler_ref->handler);
        LMK_RESTORE_CURSOR(cursor)
    }
    logger->initialized = 0;
    lmk_remove_list(&logger->link);
    lmk_free(logger);
    *logger_addr = NULL;
    return LMK_E_OK;
}

/**
 *  @brief Sets the current log level
 *  @param[logger] Logger
 *  @param[in] level Log level
 *  @return Operation error code (LMK_E_OK, LMK_E_NG)
 */

LMK_API void lmk_set_log_level(lmk_logger *logger, int log_level) {
    lmk_init();
    if (logger != NULL && (log_level >= LMK_LOG_LEVEL_TRACE && log_level
            <= LMK_LOG_LEVEL_OFF)) {
        logger->log_level = log_level;
    }
}

/**
 *  @brief Retrieves the current log level
 *  @param[logger] Logger
 *  @return Current log level
 */
LMK_API int lmk_get_log_level(lmk_logger *logger) {
    lmk_init();
    if (logger != NULL) {
        return logger->log_level;
    }
    return LMK_LOG_LEVEL_UNKNOWN;
}

/**
 *  @brief Searches a log handler from the given logger.
 *  @param[in] logger Logger
 *  @param[in] handler Log handler
 *  @return Operation error code (LMK_E_OK, LMK_E_NG)
 */
lmk_log_handler_ref *lmk_srch_log_handler_ref(lmk_logger *logger,
        lmk_log_handler *handler) {
    if (logger != NULL && handler != NULL) {
        lmk_list *cursor = NULL;
        LMK_FOR_EACH_ENTRY(&logger->handler_ref_list, cursor) {
            lmk_log_handler_ref *handler_ref = (lmk_log_handler_ref*) cursor;
            if (handler_ref->handler == handler) {
                return handler_ref;
            }
        }
    }
    return NULL;
}

/**
 *  @brief Searches a log handler from the given logger by name
 *  @param[in] logger Logger
 *  @param[in] handler Log handler
 *  @return Operation error code (LMK_E_OK, LMK_E_NG)
 */
lmk_log_handler_ref *lmk_srch_log_handler_ref_by_name(lmk_logger *logger,
        const char *name) {
    lmk_list *cursor = NULL;
    if (logger == NULL || name == NULL) {
        return NULL;
    }

    LMK_FOR_EACH_ENTRY(&logger->handler_ref_list, cursor) {
        lmk_log_handler_ref *handler_ref = (lmk_log_handler_ref*) cursor;
        if (!strcmp(handler_ref->handler->name, name)) {
            return handler_ref;
        }
    }
    return NULL;
}

/**
 *  @brief Returns true if the given log handler is registered
 *  @param[in] logger Logger
 *  @param[in] handler Log handler
 *  @return Operation error code (LMK_E_OK, LMK_E_NG)
 */
int lmk_is_log_handler_attached(lmk_logger *logger, lmk_log_handler *handler) {
    return (lmk_srch_log_handler_ref(logger, handler) != NULL) ? 1 : 0;
}

/**
 *  @brief Registers a log handler
 *  @param[in] handler Log handler
 *  @return Operation error code (LMK_E_OK, LMK_E_NG)
 */
LMK_API int lmk_attach_log_handler(lmk_logger *logger, lmk_log_handler *handler) {
    lmk_log_handler_ref *handler_ref = NULL;
    lmk_init();
    if (logger == NULL || handler == NULL || !handler->initialized) {
        return LMK_E_NG;
    }
    if (lmk_is_log_handler_attached(logger, handler)) {
        return LMK_E_OK;
    }
    if ((handler_ref = (lmk_log_handler_ref*) lmk_malloc(
            sizeof (lmk_log_handler_ref))) == NULL) {
        return LMK_E_NG;
    }
    handler_ref->handler = handler;
    lmk_init_list(&handler_ref->link);
    lmk_insert_list(&logger->handler_ref_list, &handler_ref->link);
    handler->nr_logger_ref++;
    return LMK_E_OK;
}

/** Detaches a log handler from the logger.
 *
 * */
LMK_API int lmk_detach_log_handler(lmk_logger *logger, lmk_log_handler *handler) {
    lmk_log_handler_ref *handler_ref = NULL;
    lmk_init();
    if (handler == NULL || logger == NULL) {
        return LMK_E_NG;
    }

#if LMK_DEBUG
    fprintf(stdout, "Detaching handler %s from logger %s (logger ref = %u)\n", handler->name, logger->name, handler->nr_logger_ref);
#endif

    handler_ref = lmk_srch_log_handler_ref(logger, handler);
    if (handler_ref == NULL) {
#if LMK_DEBUG
        fprintf(stdout, "Handler %s is not attached to logger %s \n", handler->name, logger->name);
#endif
        return LMK_E_NG;
    }
    lmk_remove_list(&handler_ref->link);
    lmk_free(handler_ref);
    handler->nr_logger_ref--;

#if LMK_DEBUG
    fprintf(stdout, "Handler %s detached from logger %s (logger ref = %u)\n", handler->name, logger->name, handler->nr_logger_ref);
#endif

    return LMK_E_OK;
}

LMK_API void lmk_log_trace(lmk_logger *logger, const char* file_name,
        const int line_no, const char *format, ...) {
    va_list ap;
    lmk_init();
    if (logger != NULL && LMK_IS_LOG_ENABLED(logger, LMK_LOG_LEVEL_TRACE)) {
        LMK_LOCK_MUTEX(logger->log_lock);
        memset(logger->log_buff, 0x00, LMK_LOG_BUFFER_SIZE);
        va_start(ap, format);
        vsprintf(logger->log_buff, format, ap);
        va_end(ap);
        lmk_log_impl(logger, file_name, line_no, LMK_LOG_LEVEL_TRACE,
                logger->log_buff);
        LMK_UNLOCK_MUTEX(logger->log_lock);
    }
}

LMK_API void lmk_log_info(lmk_logger *logger, const char* file_name,
        const int line_no, const char *format, ...) {
    lmk_init();
    if (logger != NULL && LMK_IS_LOG_ENABLED(logger, LMK_LOG_LEVEL_INFO)) {
        LMK_LOCK_MUTEX(logger->log_lock);
        va_list ap;
        memset(logger->log_buff, 0x00, LMK_LOG_BUFFER_SIZE);
        va_start(ap, format);
        vsprintf(logger->log_buff, format, ap);
        va_end(ap);
        lmk_log_impl(logger, file_name, line_no, LMK_LOG_LEVEL_INFO,
                logger->log_buff);
        LMK_UNLOCK_MUTEX(logger->log_lock);
    }
}

LMK_API void lmk_log_debug(lmk_logger *logger, const char* file_name,
        const int line_no, const char *format, ...) {
    va_list ap;
    lmk_init();
    if (logger != NULL && LMK_IS_LOG_ENABLED(logger, LMK_LOG_LEVEL_DEBUG)) {
        LMK_LOCK_MUTEX(logger->log_lock);
        memset(logger->log_buff, 0x00, LMK_LOG_BUFFER_SIZE);
        va_start(ap, format);
        vsprintf(logger->log_buff, format, ap);
        va_end(ap);
        lmk_log_impl(logger, file_name, line_no, LMK_LOG_LEVEL_DEBUG,
                logger->log_buff);
        LMK_UNLOCK_MUTEX(logger->log_lock);
    }
}

LMK_API void lmk_log_warn(lmk_logger *logger, const char* file_name,
        const int line_no, const char *format, ...) {
    va_list ap;
    lmk_init();
    if (logger != NULL && LMK_IS_LOG_ENABLED(logger, LMK_LOG_LEVEL_WARN)) {
        LMK_LOCK_MUTEX(logger->log_lock);
        memset(logger->log_buff, 0x00, LMK_LOG_BUFFER_SIZE);
        va_start(ap, format);
        vsprintf(logger->log_buff, format, ap);
        va_end(ap);
        lmk_log_impl(logger, file_name, line_no, LMK_LOG_LEVEL_WARN,
                logger->log_buff);
        LMK_UNLOCK_MUTEX(logger->log_lock);
    }
}

LMK_API void lmk_log_error(lmk_logger *logger, const char* file_name,
        const int line_no, const char *format, ...) {
    va_list ap;
    lmk_init();
    if (logger != NULL && LMK_IS_LOG_ENABLED(logger, LMK_LOG_LEVEL_ERROR)) {
        LMK_LOCK_MUTEX(logger->log_lock);
        memset(logger->log_buff, 0x00, LMK_LOG_BUFFER_SIZE);
        va_start(ap, format);
        vsprintf(logger->log_buff, format, ap);
        va_end(ap);
        lmk_log_impl(logger, file_name, line_no, LMK_LOG_LEVEL_ERROR,
                logger->log_buff);
        LMK_UNLOCK_MUTEX(logger->log_lock);
    }
}

LMK_API void lmk_log_fatal(lmk_logger *logger, const char* file_name,
        const int line_no, const char *format, ...) {
    va_list ap;
    lmk_init();
    if (logger != NULL && LMK_IS_LOG_ENABLED(logger, LMK_LOG_LEVEL_FATAL)) {
        LMK_LOCK_MUTEX(logger->log_lock);
        memset(logger->log_buff, 0x00, LMK_LOG_BUFFER_SIZE);
        va_start(ap, format);
        vsprintf(logger->log_buff, format, ap);
        va_end(ap);
        lmk_log_impl(logger, file_name, line_no, LMK_LOG_LEVEL_FATAL,
                logger->log_buff);
        LMK_UNLOCK_MUTEX(logger->log_lock);
    }
}

lmk_log_record *lmk_create_log_record() {
    lmk_log_record *log_rec = NULL;
    if ((log_rec = (lmk_log_record*) lmk_malloc(sizeof (lmk_log_record)))
            != NULL) {
        memset(log_rec, 0, sizeof (lmk_log_record));
        lmk_init_list(&log_rec->link);
    }
    return log_rec;
}

/**
 *  @brief Main log implementation
 *  @param[in] level Log level
 *  @param[in] data Log data
 *  @return Operation error code (LMK_E_OK, LMK_E_NG)
 */

int lmk_log_impl(lmk_logger *logger, const char *file_name, const int line_no,
        int log_level, const char *data) {
    lmk_list *cursor = NULL;
    lmk_log_handler *handler = NULL;
    const char *level_str = NULL;
    lmk_log_record log_record;

    if (logger == NULL || !logger->initialized) {
        return LMK_E_NG;
    }
    if (data == NULL) {
        return LMK_E_NG;
    }
    if (!(log_level >= LMK_LOG_LEVEL_TRACE && log_level <= LMK_LOG_LEVEL_OFF)) {
        return LMK_E_NG;
    }
    level_str = lmk_get_log_level_str(log_level);

    // check if the requested log level is allowed
    if (log_level >= logger->log_level) {
        LMK_FOR_EACH_ENTRY(&logger->handler_ref_list, cursor) {
            handler = ((lmk_log_handler_ref*) cursor)->handler;
#if LMK_DEBUG
            fprintf(stdout, "[%s:%d], logger = %s, handler(%s) = %s, request = %s\n",
                    file_name, line_no,
                    lmk_get_log_level_str(logger->log_level),
                    lmk_get_log_handler_type_str(handler->type),
                    lmk_get_log_level_str(handler->log_level),
                    lmk_get_log_level_str(log_level));

#endif
            if (log_level >= handler->log_level) {
                lmk_log_record *log_rec = &log_record;
                log_rec->log_level = log_level;
                log_rec->data = (char*) data;
                log_rec->line_no = line_no;
                log_rec->file_name = (char*) file_name;
                handler->log_impl(handler, log_rec);
                handler->nr_log_calls++;
            }
        }
    }
    return LMK_E_OK;
}

