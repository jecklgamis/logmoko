/*
 * The MIT License (MIT)
 *
 * Logmoko - A log framework for C
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

#ifndef LOGMOKO_H
#define LOGMOKO_H

/** Standard C library header files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <time.h>
#include <stdarg.h>
#include <signal.h>
#include <pthread.h>

/** Other Logmoko header files */
#include "logmoko_types.h"
#include "logmoko_time.h"
#include "logmoko_sync.h"
#include "logmoko_connector.h"
#include "logmoko_mem.h"

/** Sets the framework in debug mode */
#define LMK_DEBUG 0

#define LMK_VERSION_STRING "1.0"

/* Log levels */
enum {
    LMK_LOG_LEVEL_TRACE = 0,
    LMK_LOG_LEVEL_DEBUG,
    LMK_LOG_LEVEL_INFO,
    LMK_LOG_LEVEL_WARN,
    LMK_LOG_LEVEL_ERROR,
    LMK_LOG_LEVEL_FATAL,
    LMK_LOG_LEVEL_OFF,
    LMK_LOG_LEVEL_UNKNOWN
};

/* Log handler types */
enum {
    LMK_LOG_HANDLER_TYPE_CONSOLE = 0,
    LMK_LOG_HANDLER_TYPE_FILE,
    LMK_LOG_HANDLER_TYPE_SOCKET
};

/* Log message buffer size */
#define LMK_LOG_BUFFER_SIZE   2048

/* Log record */
typedef struct lmk_log_record {
    lmk_list link;
    int log_level;
    char *data;
    int line_no;
    char *file_name;
} lmk_log_record;

/* Log formatter (not used at the moment) */
typedef struct lmk_log_formatter {
    const char *name;
} lmk_log_formatter;

/* Log handler */
typedef struct lmk_log_handler {
    lmk_list link;
    int log_level;
    lmk_log_formatter *formatter;
    void (*init)(struct lmk_log_handler*, void*);
    void (*destroy)(struct lmk_log_handler*, void*);
    void (*log_impl)(struct lmk_log_handler*, void*);
    int type;
    char *name;
    unsigned int nr_log_calls;
    unsigned int nr_logger_ref;
    int initialized;
    lmk_mutex lock;
} lmk_log_handler;

/* The format of the log handler implementation functions */
typedef void (*lmk_log_handler_function)(struct lmk_log_handler*, void*);

/* The file log handler */
typedef struct lmk_file_log_handler {
    lmk_log_handler base;
    const char *filename;
    FILE *log_fp;
} lmk_file_log_handler;

/* File log handler properties */
enum {
    LMK_LOG_HANDLER_ATTR_FILENAME,
    LMK_LOG_HANDLER_ATTR_ROTATING,
    LMK_LOG_HANDLER_ATTR_ROTATE_INTERVAL,
};

/* The console log handler */
typedef struct lmk_console_log_handler {
    lmk_log_handler base;
} lmk_console_log_handler;

/* The socket log handler */
typedef struct lmk_socket_log_handler {
    lmk_log_handler base;
    lmk_list log_server_list;
    lmk_udp_socket socket_object;
} lmk_socket_log_handler;

/* The serial log handler */
typedef struct lmk_serial_log_handler {
    lmk_log_handler base;
} lmk_serial_log_handler;

/* Maximum logger name length */
#define LMK_LOG_MAX_LOGGER_NAME_SZ 64

/* Logger */
typedef struct lmk_logger {
    lmk_list link;
    char* name;
    int log_level;
    lmk_list handler_ref_list;
    int initialized;
    char log_buff[LMK_LOG_BUFFER_SIZE];
    lmk_mutex log_lock;
} lmk_logger;

/* Logger reference */
typedef struct lmk_log_handler_ref {
    lmk_list link;
    lmk_log_handler *handler; /**< Points to the log handler */
} lmk_log_handler_ref;

/* Logging macros  */

/* Logs a trace message */
#define LMK_LOG_TRACE(logger, format...) \
	lmk_log_trace(logger, __FILE__, __LINE__, format);

/* Logs an info message */
#define LMK_LOG_INFO(logger, format...) \
	lmk_log_info(logger, __FILE__, __LINE__, format);

/* Logs a debug message */
#define LMK_LOG_DEBUG(logger, format...) \
	lmk_log_debug(logger, __FILE__, __LINE__, format);

/* Logs a warning message */
#define LMK_LOG_WARN(logger, format...) \
	lmk_log_warn(logger, __FILE__, __LINE__, format);

/* Logs an error message */
#define LMK_LOG_ERROR(logger, format...) \
	lmk_log_error(logger, __FILE__, __LINE__, format);

/* Logs a fatal message */
#define LMK_LOG_FATAL(logger, format...) \
	lmk_log_fatal(logger, __FILE__, __LINE__, format);

#define LMK_IS_LOG_ENABLED(logger_or_handler, level) \
	(level >= (logger_or_handler)->log_level)

#define LMK_IS_VALID_LOG_LEVEL(level) \
	(level >=LMK_LOG_LEVEL_TRACE && level <= LMK_LOG_LEVEL_OFF)

extern LMK_API void lmk_init();
extern LMK_API void lmk_destroy();
extern void lmk_dump_loggers();
extern LMK_API lmk_logger * lmk_get_logger(const char *name);
extern LMK_API void lmk_set_log_level(lmk_logger *logger, int log_level);
extern LMK_API int lmk_get_log_level(lmk_logger *logger);
extern LMK_API int lmk_destroy_logger(lmk_logger **logger_addr);
extern LMK_API lmk_log_handler *lmk_get_console_log_handler();
extern LMK_API lmk_log_handler *lmk_get_file_log_handler(const char *name, const char *filename);
extern LMK_API lmk_log_handler *lmk_get_socket_log_handler(const char *name);
extern LMK_API void lmk_set_log_filename(lmk_log_handler *handler, const char *filename);
extern LMK_API int lmk_attach_log_handler(lmk_logger *logger, lmk_log_handler *handler);
extern LMK_API int lmk_detach_log_handler(lmk_logger *logger, lmk_log_handler *handler);
extern LMK_API void lmk_set_handler_log_level(lmk_log_handler *handler, int log_level);
extern LMK_API int lmk_get_handler_log_level(lmk_log_handler *handler);
extern LMK_API int lmk_destroy_log_handler(lmk_log_handler **handler_addr);
extern lmk_log_handler *lmk_find_handler(lmk_logger *logger, const char *handler_name);
extern lmk_logger *lmk_srch_logger_by_name(const char *name);
extern lmk_log_handler *lmk_srch_log_handler_by_name(const char *name);
extern const char *lmk_get_log_handler_type_str(int type);
extern const char *lmk_get_log_level_str(int log_level);
extern lmk_list *lmk_get_loggers();
extern lmk_list *lmk_get_handlers();
extern void lmk_attach_log_listener(lmk_log_handler *handler, const char *ip, int port);

#endif /* LOGMOKO_H */

