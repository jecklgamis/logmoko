/*
 * Logmoko - A logging framework for C.
 * Copyright (C) 2013 Jerrico Gamis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
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

/** @brief Log level string */
static const char* g_log_level_str[] = {
    "TRACE", "DEBUG", "INFO", "WARN",
    "ERROR", "FATAL", "OFF", "UNKNOWN"
};

/** @brief The list of all loggers */
lmk_list g_lmk_logger_list;

/*** @brief The list of all log handlers */
lmk_list g_lmk_handler_list;

/** @brief Framework initialization flag */
static int g_lmk_initialized = 0;

/** @brief the current configuration file */
const char *g_config_file = NULL;


/** Forward/external declarations */
const char *lmk_get_log_level_str(int log_level);
extern const char *lmk_get_log_handler_type_str(int type);

void lmk_configure(const char *config_file) {
    if (config_file != NULL) {
        g_config_file = config_file;
    }
}

void lmk_init() {
    if (!g_lmk_initialized) {
        lmk_init_list(&g_lmk_logger_list);
        lmk_init_list(&g_lmk_handler_list);
        if (g_config_file != NULL) {
            lmk_init_config(g_config_file);
        }
        g_lmk_initialized = 1;
    }
}

/**
 * @brief Destroys the framework. All the loggers will be detached from the
 * handlers and all handlers will be destroyed.
 */
LMK_API void lmk_destroy() {
    lmk_list *cursor;
    if (g_lmk_initialized) {
#if LMK_DEBUG
        lmk_dump_loggers();
#endif
        LMK_FOR_EACH_ENTRY(&g_lmk_logger_list, cursor) {
            lmk_logger *logger = (lmk_logger*) cursor;
            LMK_SAVE_CURSOR(cursor)
            lmk_destroy_logger(&logger);
            LMK_RESTORE_CURSOR(cursor)
        }
        // destroy all log handlers. at this point no loggers should be
        // referencing any log handlers

        LMK_FOR_EACH_ENTRY(&g_lmk_handler_list, cursor) {
            lmk_log_handler *handler = (lmk_log_handler*) cursor;
            LMK_SAVE_CURSOR(cursor)
            lmk_destroy_log_handler(&handler);
            LMK_RESTORE_CURSOR(cursor)
        }
        g_lmk_initialized = 0;
        g_config_file = NULL;
    }
}

/**
 *  @brief Searches a logger from the global list loggers
 *  @param[in] name Logger name
 */
lmk_logger *lmk_srch_logger_by_name(const char *name) {
    lmk_list *cursor = NULL;
    if (name != NULL) {
        LMK_FOR_EACH_ENTRY(&g_lmk_logger_list, cursor) {
            lmk_logger * logger = (lmk_logger*) cursor;
            if (!strcmp(logger->name, name)) {
                return logger;
            }
        }
    }
    return NULL;
}

/**
 *  @brief Searches a log handler from the global log handlers
 *  @param[in] name Log handler name
 *  TODO separate log handler namespace by type?
 */
lmk_log_handler *lmk_srch_log_handler_by_name(const char *name) {
    lmk_list *cursor = NULL;
    if (name != NULL) {
        LMK_FOR_EACH_ENTRY(&g_lmk_handler_list, cursor) {
            lmk_log_handler *handler = (lmk_log_handler*) cursor;
            if (!strcmp(name, handler->name)) {
                return handler;
            }
        }
    }
    return NULL;
}

LMK_API void lmk_dump_loggers() {
    lmk_list *cursor;
    lmk_list *cursor2;
#define LEVEL_1_SEPARATOR "    "
#define LEVEL_2_SEPARATOR "        "
#define LEVEL_3_SEPARATOR "            "

    if (g_lmk_initialized) {
        fprintf(stdout, "[Logmoko Database Dump Begin]\n");
        if (!lmk_is_list_empty(&g_lmk_handler_list)) {
            fprintf(stdout, "%s+-[Log Handlers]\n", LEVEL_1_SEPARATOR);
            LMK_FOR_EACH_ENTRY(&g_lmk_handler_list, cursor) {
                lmk_log_handler *handler = (lmk_log_handler*) cursor;
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
                lmk_logger *logger = (lmk_logger*) cursor;
                fprintf(stdout, "%s+-logger[name = %s, level = %s]\n", LEVEL_2_SEPARATOR, logger->name,
                        lmk_get_log_level_str(logger->log_level));

                LMK_FOR_EACH_ENTRY(&logger->handler_ref_list, cursor2) {
                    lmk_log_handler *handler = ((lmk_log_handler_ref*) cursor2)->handler;
                    fprintf(stdout, "%s+-attached handler[name = %s]\n", LEVEL_3_SEPARATOR, handler->name);
                }
            }
        }
        fprintf(stdout, "[Logmoko Database Dump End]\n");
    }
}

/**
 *  @brief Retrieves the log level string
 *  @param[in] level Log level
 *  @return log level string
 */

const char *lmk_get_log_level_str(int log_level) {
    if (!(log_level >= LMK_LOG_LEVEL_TRACE && log_level <= LMK_LOG_LEVEL_OFF)) {
        log_level = LMK_LOG_LEVEL_UNKNOWN;
    }
    return g_log_level_str[log_level];
}

LMK_API int lmk_get_nr_loggers() {
    lmk_init();
    return lmk_get_list_size(&g_lmk_logger_list);
}

LMK_API int lmk_get_nr_handlers() {
    lmk_init();
    return lmk_get_list_size(&g_lmk_handler_list);
}

