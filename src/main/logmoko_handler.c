#include "logmoko.h"

extern void lmk_socket_log_handler_init(struct lmk_log_handler *handler, void *param);

extern void lmk_socket_log_handler_destroy(struct lmk_log_handler *handler, void *param);

extern void lmk_socket_log_handler_log_impl(struct lmk_log_handler *handler, void *param);

extern void lmk_file_log_handler_init(struct lmk_log_handler *handler, void *param);

extern void lmk_file_log_handler_destroy(struct lmk_log_handler *handler, void *param);

extern void lmk_file_log_handler_log_impl(struct lmk_log_handler *handler, void *param);

extern void lmk_console_log_handler_init(struct lmk_log_handler *handler, void *param);

extern void lmk_console_log_handler_destroy(struct lmk_log_handler *handler, void *param);

extern void lmk_console_log_handler_log_impl(struct lmk_log_handler *handler, void *param);

extern void lmk_syslog_log_handler_init(struct lmk_log_handler *handler, void *param);

extern void lmk_syslog_log_handler_destroy(struct lmk_log_handler *handler, void *param);

extern void lmk_syslog_log_handler_log_impl(struct lmk_log_handler *handler, void *param);

extern void lmk_sync_file_log_handler_init(struct lmk_log_handler *handler, void *param);

extern void lmk_sync_file_log_handler_destroy(struct lmk_log_handler *handler, void *param);

extern void lmk_sync_file_log_handler_log_impl(struct lmk_log_handler *handler, void *param);

extern struct lmk_list g_lmk_handler_list;

void lmk_init_base_log_handler(struct lmk_log_handler *handler, int type,
                               lmk_log_handler_function init,
                               lmk_log_handler_function destroy,
                               lmk_log_handler_function log_impl, const char *name) {
    if (handler != NULL) {
        memset(handler, 0, sizeof(struct lmk_log_handler));
        lmk_init_list(&handler->link);
        int name_len = strlen(name);
        if ((handler->name = lmk_malloc(name_len + 1)) != NULL) {
            strncpy((void *) handler->name, name, name_len);
            handler->name[name_len] = '\0';
        }
        handler->init = init;
        handler->destroy = destroy;
        handler->log_impl = log_impl;
        handler->type = type;
        handler->log_level = LMK_LOG_LEVEL_TRACE;
        pthread_mutex_init(&handler->lock, NULL);
    }
}

LMK_API struct lmk_log_handler *lmk_get_console_log_handler() {
    struct lmk_console_log_handler *clh = NULL;
    struct lmk_log_handler *handler = NULL;
    lmk_init();
    if ((handler = lmk_search_log_handler_by_name("console")) != NULL) {
        return handler;
    }
    if ((clh = (struct lmk_console_log_handler *) lmk_malloc(sizeof(struct lmk_console_log_handler))) != NULL) {
        lmk_init_base_log_handler(&clh->base, LMK_LOG_HANDLER_TYPE_CONSOLE,
                                  lmk_console_log_handler_init, lmk_console_log_handler_destroy,
                                  lmk_console_log_handler_log_impl, "console");
        clh->base.init(&clh->base, NULL);
        if (!clh->ring_buffer) {
            lmk_free((void *) clh->base.name);
            pthread_mutex_destroy(&clh->base.lock);
            lmk_free(clh);
            return NULL;
        }
        lmk_insert_list(&g_lmk_handler_list, &clh->base.link);
        clh->base.initialized = 1;
    }
    return ((struct lmk_log_handler *) clh);
}

LMK_API struct lmk_log_handler *lmk_get_file_log_handler(const char *name, const char *filename) {
    struct lmk_file_log_handler *flh = NULL;
    struct lmk_log_handler *handler = NULL;
    lmk_init();
    if (!name || !filename)
        return NULL;
    if ((handler = lmk_search_log_handler_by_name(name)) != NULL) {
        return handler;
    }
    if ((flh = (struct lmk_file_log_handler *) lmk_malloc(
            sizeof(struct lmk_file_log_handler))) != NULL) {
        lmk_init_base_log_handler(&flh->base, LMK_LOG_HANDLER_TYPE_FILE,
                                  lmk_file_log_handler_init, lmk_file_log_handler_destroy,
                                  lmk_file_log_handler_log_impl, name);
        flh->log_fp = NULL;
        flh->filename = lmk_strdup(filename);
        flh->max_file_size = LMK_DEFAULT_MAX_FILE_SIZE;
        flh->max_backup_files = LMK_DEFAULT_MAX_BACKUP_FILES;
        flh->base.init(&flh->base, NULL);
        if (!flh->ring_buffer) {
            lmk_free((void *) flh->filename);
            lmk_free((void *) flh->base.name);
            pthread_mutex_destroy(&flh->base.lock);
            lmk_free(flh);
            return NULL;
        }
        lmk_insert_list(&g_lmk_handler_list, &flh->base.link);
        flh->base.initialized = 1;
    }
    return ((struct lmk_log_handler *) flh);
}

LMK_API struct lmk_log_handler *lmk_get_socket_log_handler(const char *name) {
    struct lmk_socket_log_handler *slh = NULL;
    struct lmk_log_handler *handler;
    lmk_init();
    if (name != NULL) {
        if ((handler = lmk_search_log_handler_by_name(name)) != NULL) {
            return handler;
        }
        if ((slh = (struct lmk_socket_log_handler *) lmk_malloc(
                sizeof(struct lmk_socket_log_handler))) != NULL) {
            lmk_init_base_log_handler(&slh->base, LMK_LOG_HANDLER_TYPE_SOCKET,
                                      lmk_socket_log_handler_init, lmk_socket_log_handler_destroy,
                                      lmk_socket_log_handler_log_impl, name);
            slh->base.init(&slh->base, NULL);
            if (!slh->ring_buffer) {
                lmk_free((void *) slh->base.name);
                pthread_mutex_destroy(&slh->base.lock);
                lmk_free(slh);
                return NULL;
            }
            lmk_insert_list(&g_lmk_handler_list, &slh->base.link);
            slh->base.initialized = 1;
        }
    }
    return ((struct lmk_log_handler *) slh);
}

LMK_API struct lmk_log_handler *lmk_get_syslog_log_handler(const char *name, const char *ident, int facility) {
    struct lmk_syslog_log_handler *slh = NULL;
    struct lmk_log_handler *handler;
    lmk_init();
    if (name != NULL) {
        if ((handler = lmk_search_log_handler_by_name(name)) != NULL)
            return handler;
        if ((slh = (struct lmk_syslog_log_handler *) lmk_malloc(
                sizeof(struct lmk_syslog_log_handler))) != NULL) {
            lmk_init_base_log_handler(&slh->base, LMK_LOG_HANDLER_TYPE_SYSLOG,
                                      lmk_syslog_log_handler_init, lmk_syslog_log_handler_destroy,
                                      lmk_syslog_log_handler_log_impl, name);
            if (ident)
                strncpy(slh->ident, ident, sizeof(slh->ident) - 1);
            slh->facility = facility;
            slh->base.init(&slh->base, NULL);
            if (!slh->ring_buffer) {
                lmk_free((void *) slh->base.name);
                pthread_mutex_destroy(&slh->base.lock);
                lmk_free(slh);
                return NULL;
            }
            lmk_insert_list(&g_lmk_handler_list, &slh->base.link);
            slh->base.initialized = 1;
        }
    }
    return ((struct lmk_log_handler *) slh);
}

LMK_API struct lmk_log_handler *lmk_get_sync_file_log_handler(const char *name,
                                                                const char *filename) {
    struct lmk_sync_file_log_handler *sfh = NULL;
    struct lmk_log_handler *handler = NULL;
    lmk_init();
    if (!name || !filename)
        return NULL;
    if ((handler = lmk_search_log_handler_by_name(name)) != NULL)
        return handler;
    if ((sfh = lmk_malloc(sizeof(struct lmk_sync_file_log_handler))) != NULL) {
        lmk_init_base_log_handler(&sfh->base, LMK_LOG_HANDLER_TYPE_SYNC_FILE,
                                  lmk_sync_file_log_handler_init,
                                  lmk_sync_file_log_handler_destroy,
                                  lmk_sync_file_log_handler_log_impl, name);
        sfh->filename          = lmk_strdup(filename);
        sfh->log_fp            = NULL;
        sfh->max_file_size     = LMK_DEFAULT_MAX_FILE_SIZE;
        sfh->max_backup_files  = LMK_DEFAULT_MAX_BACKUP_FILES;
        sfh->current_file_size = 0;
        sfh->base.init(&sfh->base, NULL);
        if (!sfh->base.initialized) {
            lmk_free((void *)sfh->filename);
            lmk_free((void *)sfh->base.name);
            pthread_mutex_destroy(&sfh->base.lock);
            lmk_free(sfh);
            return NULL;
        }
        lmk_insert_list(&g_lmk_handler_list, &sfh->base.link);
    }
    return (struct lmk_log_handler *)sfh;
}

int lmk_destroy_log_handler(struct lmk_log_handler **handler_addr) {
    struct lmk_log_handler *handler = NULL;

    if (handler_addr == NULL || *handler_addr == NULL) {
        return LMK_E_NG;
    }
    handler = *handler_addr;
    if (handler->nr_logger_ref > 0) {
#if LMK_DEBUG
        fprintf(stdout, "Cannot destroy log handler %s as it's still being referenced\n", handler->name);
#endif
        return LMK_E_NG;
    }
#if LMK_DEBUG
    fprintf(stdout, "Destroying log handler : %s (%p)\n", handler->name, handler);
#endif
    if (handler->destroy != NULL) {
        handler->destroy(handler, NULL);
    }
    lmk_remove_list(&handler->link);
    if (handler->name != NULL) {
        lmk_free((void *) handler->name);
    }
    pthread_mutex_destroy(&handler->lock);
    lmk_free(handler);
    *handler_addr = NULL;
    return LMK_E_OK;
}

LMK_API void lmk_set_handler_log_level(struct lmk_log_handler *handler, int log_level) {
    lmk_init();
    if (handler != NULL && (log_level >= LMK_LOG_LEVEL_TRACE && log_level
                                                                <= LMK_LOG_LEVEL_OFF)) {
        handler->log_level = log_level;
    }
}

LMK_API int lmk_get_handler_log_level(struct lmk_log_handler *handler) {
    lmk_init();
    if (handler != NULL && handler->initialized) {
        return handler->log_level;
    }
    return LMK_LOG_LEVEL_UNKNOWN;
}

static int lmk_default_format(char *out, size_t out_size,
                               int log_level, const char *level_str,
                               const char *timestamp,
                               const char *file_name, int line_no,
                               const char *handler_name,
                               const char *message) {
    return snprintf(out, out_size, "[%-5s %s (%s:%d) %s] : %s\n",
                    level_str, timestamp, file_name, line_no, handler_name, message);
}

static int lmk_simple_format(char *out, size_t out_size,
                              int log_level, const char *level_str,
                              const char *timestamp,
                              const char *file_name, int line_no,
                              const char *handler_name,
                              const char *message) {
    return snprintf(out, out_size, "%s [%-5s] %s\n", timestamp, level_str, message);
}

static void lmk_json_escape(char *dst, size_t dst_size, const char *src) {
    size_t i = 0;
    while (*src && i + 2 < dst_size) {
        if (*src == '"' || *src == '\\') {
            dst[i++] = '\\';
        } else if (*src == '\n') {
            dst[i++] = '\\';
            dst[i++] = 'n';
            src++;
            continue;
        }
        dst[i++] = *src++;
    }
    dst[i] = '\0';
}

static int lmk_json_format(char *out, size_t out_size,
                            int log_level, const char *level_str,
                            const char *timestamp,
                            const char *file_name, int line_no,
                            const char *handler_name,
                            const char *message) {
    char escaped[1024];
    lmk_json_escape(escaped, sizeof(escaped), message);
    return snprintf(out, out_size,
                    "{\"timestamp\":\"%s\",\"level\":\"%s\",\"file\":\"%s\","
                    "\"line\":%d,\"handler\":\"%s\",\"message\":\"%s\"}\n",
                    timestamp, level_str, file_name, line_no, handler_name, escaped);
}

lmk_format_fn lmk_get_format_fn(const char *name) {
    if (!name || !strcasecmp(name, "default"))
        return NULL;
    if (!strcasecmp(name, "simple"))
        return lmk_simple_format;
    if (!strcasecmp(name, "json"))
        return lmk_json_format;
    return NULL;
}

int lmk_format_log_line(struct lmk_log_handler *handler, char *out, size_t out_size,
                         const struct lmk_log_request *req) {
    const char *level_str = lmk_get_log_level_str(req->log_level);

    /*
     * Timestamp cache: time() + localtime_r + strftime runs once per second
     * per consumer thread instead of once per log line (#2).
     */
    static _Thread_local time_t ts_last_sec = 0;
    static _Thread_local char   ts_cache[LMK_TSTAMP_BUFF_SIZE];
    time_t now = time(NULL);
    if (now != ts_last_sec) {
        struct tm tm_info;
        localtime_r(&now, &tm_info);
        strftime(ts_cache, sizeof(ts_cache), "%Y-%m-%d %H:%M:%S", &tm_info);
        ts_last_sec = now;
    }

    lmk_format_fn fn = handler->format_fn ? handler->format_fn : lmk_default_format;
    int n = fn(out, out_size, req->log_level, level_str, ts_cache,
               req->file_name, req->line_no, req->handler_name, req->data);
    return n > 0 ? (n < (int)out_size ? n : (int)out_size - 1) : 0;
}

LMK_API void lmk_set_log_format(struct lmk_log_handler *handler, lmk_format_fn fn) {
    if (handler) {
        handler->format_fn = fn;
    }
}

static const char *g_log_hnd_type_str[] = {"CONSOLE", "FILE", "SOCKET", "SYSLOG", "SYNC_FILE"};

const char *lmk_get_log_handler_type_str(int type) {
    const char *type_str = "UNKNOWN";
    if (type >= LMK_LOG_HANDLER_TYPE_CONSOLE && type
                                                <= LMK_LOG_HANDLER_TYPE_SYSLOG) {
        type_str = g_log_hnd_type_str[type];
    }
    return type_str;
}

