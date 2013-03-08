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

extern void lmk_socket_log_handler_init(lmk_log_handler *handler, void *param);
extern void lmk_socket_log_handler_destroy(lmk_log_handler *handler, void *param);
extern void lmk_socket_log_handler_log_impl(lmk_log_handler *handler, void *param);

extern void lmk_file_log_handler_init(lmk_log_handler *handler, void *param);
extern void lmk_file_log_handler_destroy(lmk_log_handler *handler, void *param);
extern void lmk_file_log_handler_log_impl(lmk_log_handler *handler, void *param);

extern void lmk_console_log_handler_init(lmk_log_handler *handler, void *param);
extern void lmk_console_log_handler_destroy(lmk_log_handler *handler, void *param);
extern void lmk_console_log_handler_log_impl(lmk_log_handler *handler, void *param);

extern lmk_list g_lmk_handler_list;

void lmk_init_base_log_handler(lmk_log_handler *handler, int type,
        lmk_log_handler_function init,
        lmk_log_handler_function destroy,
        lmk_log_handler_function log_impl, const char *name) {
    if (handler != NULL) {
        memset(handler, 0, sizeof (lmk_log_handler));
        lmk_init_list(&handler->link);
        int name_len = strlen(name);
        if ((handler->name = lmk_malloc(name_len)) != NULL) {
            strncpy((void*)handler->name, name, name_len);
            handler->name[name_len] = '\0';
        }
        handler->init = init;
        handler->destroy = destroy;
        handler->log_impl = log_impl;
        handler->type = type;
        handler->log_level = LMK_LOG_LEVEL_TRACE;
    }
}

/**
 * Retrieves the console log handler.
 */
LMK_API lmk_log_handler *lmk_get_console_log_handler() {
    lmk_console_log_handler *clh = NULL;
    lmk_log_handler *handler = NULL;
    lmk_init();
    if ((handler = lmk_srch_log_handler_by_name("console")) != NULL) {
        return handler;
    }
    if ((clh = (lmk_console_log_handler*) lmk_malloc(
            sizeof (lmk_console_log_handler))) != NULL) {
        lmk_init_base_log_handler(&clh->base, LMK_LOG_HANDLER_TYPE_CONSOLE,
                lmk_console_log_handler_init, lmk_console_log_handler_destroy,
                lmk_console_log_handler_log_impl, "console");
        lmk_insert_list(&g_lmk_handler_list, &clh->base.link);
        clh->base.init(&clh->base, NULL);
        clh->base.initialized = 1;
    }
    return ((lmk_log_handler*) clh);
}

LMK_API lmk_log_handler *lmk_get_file_log_handler(const char *name, const char *filename) {
    lmk_file_log_handler *flh = NULL;
    lmk_log_handler *handler = NULL;
    lmk_init();
    if (name != NULL) {
        if ((handler = lmk_srch_log_handler_by_name(name)) != NULL) {
            return handler;
        }
        if ((flh = (lmk_file_log_handler*) lmk_malloc(
                sizeof (lmk_file_log_handler))) != NULL) {
            lmk_init_base_log_handler(&flh->base, LMK_LOG_HANDLER_TYPE_FILE,
                    lmk_file_log_handler_init, lmk_file_log_handler_destroy,
                    lmk_file_log_handler_log_impl, name);
            flh->log_fp = NULL;
            flh->filename = filename;
            lmk_insert_list(&g_lmk_handler_list, &flh->base.link);
            flh->base.init(&flh->base, NULL);
            flh->base.initialized = 1;
        }
    }
    return ((lmk_log_handler*) flh);
}

LMK_API lmk_log_handler *lmk_get_socket_log_handler(const char* name) {
    lmk_socket_log_handler *slh = NULL;
    lmk_log_handler *handler;
    lmk_init();
    if (name != NULL) {
        if ((handler = lmk_srch_log_handler_by_name(name)) != NULL) {
            return handler;
        }
        if ((slh = (lmk_socket_log_handler*) lmk_malloc(
                sizeof (lmk_socket_log_handler))) != NULL) {
            lmk_init_base_log_handler(&slh->base, LMK_LOG_HANDLER_TYPE_SOCKET,
                    lmk_socket_log_handler_init, lmk_socket_log_handler_destroy,
                    lmk_socket_log_handler_log_impl, name);
            lmk_insert_list(&g_lmk_handler_list, &slh->base.link);
            slh->base.init(&slh->base, NULL);
            slh->base.initialized = 1;
        }
    }
    return ((lmk_log_handler*) slh);
}

/**
 *  @brief Stops a log handler
 *  @param[in] handler_addr Log handler address
 *  @return Operation error code (LMK_E_OK, LMK_E_NG)
 */

int lmk_destroy_log_handler(lmk_log_handler **handler_addr) {
    lmk_log_handler *handler = NULL;

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
    lmk_free(handler);
    if (handler->name != NULL) {
        lmk_free((void*)handler->name);
    }
    *handler_addr = NULL;
    return LMK_E_OK;
}

/**
 *  @brief Sets the current log level
 *  @param[logger] Logger
 *  @param[in] level Log level
 *  @return Operation error code (LMK_E_OK, LMK_E_NG)
 */

LMK_API void lmk_set_handler_log_level(lmk_log_handler *handler, int log_level) {
    lmk_init();
    if (handler != NULL && (log_level >= LMK_LOG_LEVEL_TRACE && log_level
            <= LMK_LOG_LEVEL_OFF)) {
        handler->log_level = log_level;
    }
}

LMK_API int lmk_get_handler_log_level(lmk_log_handler *handler) {
    lmk_init();
    if (handler != NULL && handler->initialized) {
        return handler->log_level;
    }
    return LMK_LOG_LEVEL_UNKNOWN;
}

/** @brief Log handler type string */
static const char* g_log_hnd_type_str[] = {"CONSOLE", "FILE", "SOCKET"};

/**
 *  @brief Returns the log handler type string
 *  @param[in] level Log level
 *  @return Log handler type string
 */
const char *lmk_get_log_handler_type_str(int type) {
    const char *type_str = "UNKNOWN";
    if (type >= LMK_LOG_HANDLER_TYPE_CONSOLE && type
            <= LMK_LOG_HANDLER_TYPE_SOCKET) {
        type_str = g_log_hnd_type_str[type];
    }
    return type_str;
}

