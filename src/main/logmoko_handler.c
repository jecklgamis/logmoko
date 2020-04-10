/*
 * The MIT License (MIT)
 *
 * Logmoko - A logging framework for C
 * Copyright 2013 Jerrico Gamis <jecklgamis@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
        memset(handler, 0, sizeof(lmk_log_handler));
        lmk_init_list(&handler->link);
        int name_len = strlen(name);
        if ((handler->name = lmk_malloc(name_len)) != NULL) {
            strncpy((void *) handler->name, name, name_len);
            handler->name[name_len] = '\0';
        }
        handler->init = init;
        handler->destroy = destroy;
        handler->log_impl = log_impl;
        handler->type = type;
        handler->log_level = LMK_LOG_LEVEL_TRACE;
    }
}

/* Retrieves the console log handler */
LMK_API lmk_log_handler *lmk_get_console_log_handler() {
    lmk_console_log_handler *clh = NULL;
    lmk_log_handler *handler = NULL;
    lmk_init();
    if ((handler = lmk_srch_log_handler_by_name("console")) != NULL) {
        return handler;
    }
    if ((clh = (lmk_console_log_handler *) lmk_malloc(
            sizeof(lmk_console_log_handler))) != NULL) {
        lmk_init_base_log_handler(&clh->base, LMK_LOG_HANDLER_TYPE_CONSOLE,
                                  lmk_console_log_handler_init, lmk_console_log_handler_destroy,
                                  lmk_console_log_handler_log_impl, "console");
        lmk_insert_list(&g_lmk_handler_list, &clh->base.link);
        clh->base.init(&clh->base, NULL);
        clh->base.initialized = 1;
    }
    return ((lmk_log_handler *) clh);
}

LMK_API lmk_log_handler *lmk_get_file_log_handler(const char *name, const char *filename) {
    lmk_file_log_handler *flh = NULL;
    lmk_log_handler *handler = NULL;
    lmk_init();
    if (name != NULL) {
        if ((handler = lmk_srch_log_handler_by_name(name)) != NULL) {
            return handler;
        }
        if ((flh = (lmk_file_log_handler *) lmk_malloc(
                sizeof(lmk_file_log_handler))) != NULL) {
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
    return ((lmk_log_handler *) flh);
}

LMK_API lmk_log_handler *lmk_get_socket_log_handler(const char *name) {
    lmk_socket_log_handler *slh = NULL;
    lmk_log_handler *handler;
    lmk_init();
    if (name != NULL) {
        if ((handler = lmk_srch_log_handler_by_name(name)) != NULL) {
            return handler;
        }
        if ((slh = (lmk_socket_log_handler *) lmk_malloc(
                sizeof(lmk_socket_log_handler))) != NULL) {
            lmk_init_base_log_handler(&slh->base, LMK_LOG_HANDLER_TYPE_SOCKET,
                                      lmk_socket_log_handler_init, lmk_socket_log_handler_destroy,
                                      lmk_socket_log_handler_log_impl, name);
            lmk_insert_list(&g_lmk_handler_list, &slh->base.link);
            slh->base.init(&slh->base, NULL);
            slh->base.initialized = 1;
        }
    }
    return ((lmk_log_handler *) slh);
}

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
        lmk_free((void *) handler->name);
    }
    *handler_addr = NULL;
    return LMK_E_OK;
}

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

static const char *g_log_hnd_type_str[] = {"CONSOLE", "FILE", "SOCKET"};

const char *lmk_get_log_handler_type_str(int type) {
    const char *type_str = "UNKNOWN";
    if (type >= LMK_LOG_HANDLER_TYPE_CONSOLE && type
                                                <= LMK_LOG_HANDLER_TYPE_SOCKET) {
        type_str = g_log_hnd_type_str[type];
    }
    return type_str;
}

