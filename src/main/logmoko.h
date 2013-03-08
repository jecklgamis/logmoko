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

/** @brief Log levels */
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

/** @brief Log handler types */
enum {
    LMK_LOG_HANDLER_TYPE_CONSOLE = 0,
    LMK_LOG_HANDLER_TYPE_FILE,
    LMK_LOG_HANDLER_TYPE_SOCKET
};

/** @brief Log message buffer size */
#define LMK_LOG_BUFFER_SIZE   2048

/** @brief Log record */
typedef struct lmk_log_record {
    lmk_list link;
    int log_level;
    char *data;
    int line_no;
    char *file_name;
} lmk_log_record;

/** @brief Log formatter (not used at the moment) */
typedef struct lmk_log_formatter {
    const char *name;
} lmk_log_formatter;

/** @brief Log handler */
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

/** @brief The format of the log handler implementation functions */
typedef void (*lmk_log_handler_function)(struct lmk_log_handler*, void*);

/** @brief The file log handler */
typedef struct lmk_file_log_handler {
    lmk_log_handler base;
    const char *filename;
    FILE *log_fp;
} lmk_file_log_handler;

/** @brief File log handler properties */
enum {
    LMK_LOG_HANDLER_ATTR_FILENAME,
    LMK_LOG_HANDLER_ATTR_ROTATING,
    LMK_LOG_HANDLER_ATTR_ROTATE_INTERVAL,
};

/** @brief The console log handler */
typedef struct lmk_console_log_handler {
    lmk_log_handler base;
} lmk_console_log_handler;

/** @brief The socket log handler */
typedef struct lmk_socket_log_handler {
    lmk_log_handler base;
    lmk_list log_server_list;
    lmk_udp_socket socket_object;
} lmk_socket_log_handler;

/** @brief The serial log handler */
typedef struct lmk_serial_log_handler {
    lmk_log_handler base;
} lmk_serial_log_handler;

/** @brief Maximum logger name length */
#define LMK_LOG_MAX_LOGGER_NAME_SZ 64

/** @brief Logger */
typedef struct lmk_logger {
    lmk_list link;
    char* name;
    int log_level;
    lmk_list handler_ref_list;
    int initialized;
    char log_buff[LMK_LOG_BUFFER_SIZE];
    lmk_mutex log_lock;
} lmk_logger;

/** @brief Logger reference */
typedef struct lmk_log_handler_ref {
    lmk_list link;
    lmk_log_handler *handler; /**< Points to the log handler */
} lmk_log_handler_ref;

/** Logging macros  */

/** @brief Logs a trace message */
#define LMK_LOG_TRACE(logger, format...) \
	lmk_log_trace(logger, __FILE__, __LINE__, format);

/** @brief Logs an info message */
#define LMK_LOG_INFO(logger, format...) \
	lmk_log_info(logger, __FILE__, __LINE__, format);

/** @brief Logs a debug message */
#define LMK_LOG_DEBUG(logger, format...) \
	lmk_log_debug(logger, __FILE__, __LINE__, format);

/** @brief Logs a warning message */
#define LMK_LOG_WARN(logger, format...) \
	lmk_log_warn(logger, __FILE__, __LINE__, format);

/** @brief Logs an error message */
#define LMK_LOG_ERROR(logger, format...) \
	lmk_log_error(logger, __FILE__, __LINE__, format);

/** @brief Logs a fatal message */
#define LMK_LOG_FATAL(logger, format...) \
	lmk_log_fatal(logger, __FILE__, __LINE__, format);

#define LMK_IS_LOG_ENABLED(logger_or_handler, level) \
	(level >= (logger_or_handler)->log_level)

#define LMK_IS_VALID_LOG_LEVEL(level) \
	(level >=LMK_LOG_LEVEL_TRACE && level <= LMK_LOG_LEVEL_OFF)

/** @brief Initializes the framework */
extern LMK_API void lmk_init();

/** @brief Destroys the framework */
extern LMK_API void lmk_destroy();

/** @brief Prints all the loggers and the handlers to the console */
extern void lmk_dump_loggers();

/** @brief Retrieves a logger */
extern LMK_API lmk_logger * lmk_get_logger(const char *name);

/** @brief Sets a logger's log level */
extern LMK_API void lmk_set_log_level(lmk_logger *logger, int log_level);

/** @brief Gets a logger's log level */
extern LMK_API int lmk_get_log_level(lmk_logger *logger);

/** @brief Destroys a logger */
extern LMK_API int lmk_destroy_logger(lmk_logger **logger_addr);

/** @brief Retrieves a console log handler  */
extern LMK_API lmk_log_handler *lmk_get_console_log_handler();

/** @brief Retrieves a file log handler  */
extern LMK_API lmk_log_handler *lmk_get_file_log_handler(const char *name, const char *filename);

/** @brief Retrieves a socket log handler  */
extern LMK_API lmk_log_handler *lmk_get_socket_log_handler(const char *name);

extern LMK_API void lmk_set_log_filename(lmk_log_handler *handler,
        const char *filename);

/** @brief Attaches a log handler to a logger */
extern LMK_API int lmk_attach_log_handler(lmk_logger *logger,
        lmk_log_handler *handler);

/** @brief Detaches a log handler from a logger */
extern LMK_API int lmk_detach_log_handler(lmk_logger *logger,
        lmk_log_handler *handler);

/** @brief Sets a log handler's log level */
extern LMK_API void lmk_set_handler_log_level(lmk_log_handler *handler,
        int log_level);

/** @brief Gets a log handler's log level */
extern LMK_API int lmk_get_handler_log_level(lmk_log_handler *handler);

/** @brief Destroys the log handler */
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

