#include "logmoko.h"

static const char *g_log_level_str[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "OFF", "UNKNOWN"};
struct lmk_list g_lmk_logger_list;
struct lmk_list g_lmk_handler_list;
static int g_lmk_initialized = 0;
const char *g_config_file = NULL;
struct lmk_config *g_lmk_config = NULL;

const char *lmk_get_log_level_str(int log_level);

extern const char *lmk_get_log_handler_type_str(int type);

int get_unsigned_int_key_from_env(const char *name, int fallback) {
    char *value = getenv(name);
    if (!value) {
        return fallback;
    }
    return (unsigned int) atoi(value);
}

struct lmk_config *lmk_get_config() {
    struct lmk_config *config = lmk_malloc(sizeof(struct lmk_config));
    if (!config) {
        fprintf(stderr, "Unable to allocate config");
        return NULL;
    }
    config->log_buffer_size = get_unsigned_int_key_from_env("LMK_LOG_BUFFER_SIZE", 2048);
    config->ring_buffer_size = get_unsigned_int_key_from_env("LMK_RING_BUFFER_SIZE", 128);
    config->default_level = LMK_LOG_LEVEL_INFO;
#if LMK_DEBUG
    fprintf(stdout, "Using log_buffer_size = %d\n", config->log_buffer_size);
    fprintf(stdout, "Using ring_buffer_size = %d\n", config->ring_buffer_size);
    fprintf(stdout, "Using default_level = %d\n", config->default_level);
#endif
    return config;
}

void lmk_init() {
    if (!g_lmk_initialized) {
        lmk_init_list(&g_lmk_logger_list);
        lmk_init_list(&g_lmk_handler_list);
        g_lmk_config = lmk_get_config();
        if (!g_lmk_config) {
            printf("Unable to initialize");
            return;
        }
        g_lmk_initialized = 1;
    }
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
    g_config_file = NULL;
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
        fprintf(stdout, "[Logmoko Database Dump Begin]\n");
        if (!lmk_is_list_empty(&g_lmk_handler_list)) {
            fprintf(stdout, "%s+-[Log Handlers]\n", LEVEL_1_SEPARATOR);
            LMK_FOR_EACH_ENTRY(&g_lmk_handler_list, cursor) {
                struct lmk_log_handler *handler = (struct lmk_log_handler *) cursor;
                fprintf(stdout, "%s+-handler[name = %s, type = %s, level = %s, logger-refs = %u]\n",
                        LEVEL_2_SEPARATOR,
                        handler->name,
                        lmk_get_log_handler_type_str(handler->type),
                        lmk_get_log_level_str(handler->log_level),
                        handler->nr_logger_ref
                );
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
        fprintf(stdout, "[Logmoko Database Dump End]\n");
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

