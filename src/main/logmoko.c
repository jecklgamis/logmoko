#include "logmoko.h"

static const char *g_log_level_str[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "OFF", "UNKNOWN"};
struct lmk_list g_lmk_logger_list;
struct lmk_list g_lmk_handler_list;
static int g_lmk_initialized = 0;
static pthread_mutex_t g_lmk_init_mutex = PTHREAD_MUTEX_INITIALIZER;
struct lmk_config *g_lmk_config = NULL;

const char *lmk_get_log_level_str(int log_level);

extern const char *lmk_get_log_handler_type_str(int type);

extern struct lmk_config *lmk_create_config();

extern void lmk_apply_auto_config();

struct lmk_config *lmk_get_config() {
    return g_lmk_config;
}

void lmk_init() {
    if (g_lmk_initialized)
        return;
    pthread_mutex_lock(&g_lmk_init_mutex);
    if (!g_lmk_initialized) {
        lmk_init_list(&g_lmk_logger_list);
        lmk_init_list(&g_lmk_handler_list);
        g_lmk_config = lmk_create_config();
        if (!g_lmk_config) {
            fprintf(stderr, "Unable to initialize\n");
            pthread_mutex_unlock(&g_lmk_init_mutex);
            return;
        }
        g_lmk_initialized = 1;
        lmk_apply_auto_config();
        if (g_lmk_config->log_buffer_size <= 0)
            g_lmk_config->log_buffer_size = LMK_CFG_DEFAULT_LOG_BUFFER_SIZE;
        if (g_lmk_config->ring_buffer_size <= 0)
            g_lmk_config->ring_buffer_size = LMK_CFG_DEFAULT_RING_BUFFER_SIZE;
    }
    pthread_mutex_unlock(&g_lmk_init_mutex);
}

LMK_API void lmk_destroy() {
    if (!g_lmk_initialized) {
        return;
    }
#if LMK_DEBUG
    lmk_dump_loggers();
#endif
    struct lmk_list *cursor;
    LMK_FOR_EACH_ENTRY(&g_lmk_logger_list, cursor) {
        struct lmk_logger *logger = (struct lmk_logger *) cursor;
        LMK_SAVE_CURSOR(cursor)
            lmk_destroy_logger(&logger);
        LMK_RESTORE_CURSOR(cursor)
    }
    /* destroy all log handlers. at this point no loggers should be  referencing any log handlers */
    LMK_FOR_EACH_ENTRY(&g_lmk_handler_list, cursor) {
        struct lmk_log_handler *handler = (struct lmk_log_handler *) cursor;
        LMK_SAVE_CURSOR(cursor)
            lmk_destroy_log_handler(&handler);
        LMK_RESTORE_CURSOR(cursor)
    }
    g_lmk_initialized = 0;
}

struct lmk_logger *lmk_search_logger_by_name(const char *name) {
    struct lmk_list *cursor = NULL;
    if (!name) {
        return NULL;
    }
    LMK_FOR_EACH_ENTRY(&g_lmk_logger_list, cursor) {
        struct lmk_logger *logger = (struct lmk_logger *) cursor;
        if (!strcmp(logger->name, name)) {
            return logger;
        }
    }
    return NULL;
}

struct lmk_log_handler *lmk_search_log_handler_by_name(const char *name) {
    struct lmk_list *cursor = NULL;
    if (!name) {
        return NULL;
    }
    LMK_FOR_EACH_ENTRY(&g_lmk_handler_list, cursor) {
        struct lmk_log_handler *handler = (struct lmk_log_handler *) cursor;
        if (!strcmp(name, handler->name)) {
            return handler;
        }
    }
    return NULL;
}

LMK_API void lmk_dump_loggers() {
    struct lmk_list *cursor;
    struct lmk_list *cursor2;
#define LEVEL_1_SEPARATOR "    "
#define LEVEL_2_SEPARATOR "        "
#define LEVEL_3_SEPARATOR "            "

    if (g_lmk_initialized) {
        fprintf(stdout, "[Logmoko Dump Begin]\n");
        if (!lmk_is_list_empty(&g_lmk_handler_list)) {
            fprintf(stdout, "%s+-[Log Handlers]\n", LEVEL_1_SEPARATOR);
            LMK_FOR_EACH_ENTRY(&g_lmk_handler_list, cursor) {
                struct lmk_log_handler *handler = (struct lmk_log_handler *) cursor;
                fprintf(stdout, "%s+-handler[name = %s, type = %s, level = %s, logger-refs = %u]\n",
                        LEVEL_2_SEPARATOR,
                        handler->name,
                        lmk_get_log_handler_type_str(handler->type),
                        lmk_get_log_level_str(handler->log_level),
                        handler->nr_logger_ref);
                if (handler->type == LMK_LOG_HANDLER_TYPE_FILE) {
                    struct lmk_file_log_handler *flh = (struct lmk_file_log_handler *) handler;
                    fprintf(stdout, "%s+-filename = %s\n", LEVEL_3_SEPARATOR,
                            flh->filename ? flh->filename : "(none)");
                } else if (handler->type == LMK_LOG_HANDLER_TYPE_SOCKET) {
                    struct lmk_socket_log_handler *slh = (struct lmk_socket_log_handler *) handler;
                    struct lmk_list *sc;
                    LMK_FOR_EACH_ENTRY(&slh->log_server_list, sc) {
                        struct lmk_log_server *ls = (struct lmk_log_server *) sc;
                        char ip[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &ls->socket_addr.sin_addr, ip, sizeof(ip));
                        fprintf(stdout, "%s+-listener = %s:%d\n", LEVEL_3_SEPARATOR,
                                ip, ntohs(ls->socket_addr.sin_port));
                    }
                }
            }
        }
        if (!lmk_is_list_empty(&g_lmk_logger_list)) {
            fprintf(stdout, "%s+-[Loggers]\n", LEVEL_1_SEPARATOR);

            LMK_FOR_EACH_ENTRY(&g_lmk_logger_list, cursor) {
                struct lmk_logger *logger = (struct lmk_logger *) cursor;
                fprintf(stdout, "%s+-logger[name = %s, level = %s]\n", LEVEL_2_SEPARATOR, logger->name,
                        lmk_get_log_level_str(logger->log_level));

                LMK_FOR_EACH_ENTRY(&logger->handler_ref_list, cursor2) {
                    struct lmk_log_handler *handler = ((struct lmk_log_handler_ref *) cursor2)->handler;
                    fprintf(stdout, "%s+-attached handler[name = %s]\n", LEVEL_3_SEPARATOR, handler->name);
                }
            }
        }
        fprintf(stdout, "[Logmoko Dump End]\n");
    }
}

const char *lmk_get_log_level_str(int log_level) {
    if (log_level >= LMK_LOG_LEVEL_TRACE && log_level <= LMK_LOG_LEVEL_OFF)
        return g_log_level_str[log_level];
    return g_log_level_str[LMK_LOG_LEVEL_UNKNOWN];
}

LMK_API int lmk_get_nr_loggers() {
    lmk_init();
    return lmk_get_list_size(&g_lmk_logger_list);
}

LMK_API int lmk_get_nr_handlers() {
    lmk_init();
    return lmk_get_list_size(&g_lmk_handler_list);
}

