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
struct lmk_log_record {
    struct lmk_list link;
    int log_level;
    char *data;
    int line_no;
    char *file_name;
};

/* Log formatter (not used at the moment) */
struct lmk_log_formatter {
    const char *name;
};

/* Log handler */
struct lmk_log_handler {
    struct lmk_list link;
    int log_level;
    struct lmk_log_formatter *formatter;

    void (*init)(struct lmk_log_handler *, void *);

    void (*destroy)(struct lmk_log_handler *, void *);

    void (*log_impl)(struct lmk_log_handler *, void *);

    int type;
    char *name;
    unsigned int nr_log_calls;
    unsigned int nr_logger_ref;
    int initialized;
    lmk_mutex lock;
};

/* The format of the log handler implementation functions */
typedef void (*lmk_log_handler_function)(struct lmk_log_handler *, void *);

/* Log request for asynchronous handlers */
#define LMK_RING_BUFFER_SIZE 32

struct lmk_log_request {
    int log_level;
    char *data;
    int line_no;
    char *file_name;
    char *handler_name; /* Added to support socket/file handler specific context if needed */
};

/* The file log handler */
struct lmk_file_log_handler {
    struct lmk_log_handler base;
    const char *filename;
    FILE *log_fp;
    struct lmk_log_request ring_buffer[LMK_RING_BUFFER_SIZE];
    int head;
    int tail;
    int count;
    pthread_t thread;
    int running;
    lmk_cond cond;
    lmk_cond space_cond;
};

/* File log handler properties */
enum {
    LMK_LOG_HANDLER_ATTR_FILENAME,
    LMK_LOG_HANDLER_ATTR_ROTATING,
    LMK_LOG_HANDLER_ATTR_ROTATE_INTERVAL,
};

/* The console log handler */
struct lmk_console_log_handler {
    struct lmk_log_handler base;
    struct lmk_log_request ring_buffer[LMK_RING_BUFFER_SIZE];
    int head;
    int tail;
    int count;
    pthread_t thread;
    int running;
    lmk_cond cond;
    lmk_cond space_cond;
};

/* The socket log handler */
struct lmk_socket_log_handler {
    struct lmk_log_handler base;
    struct lmk_list log_server_list;
    struct lmk_udp_socket socket_object;
    struct lmk_log_request ring_buffer[LMK_RING_BUFFER_SIZE];
    int head;
    int tail;
    int count;
    pthread_t thread;
    int running;
    lmk_cond cond;
    lmk_cond space_cond;
};

/* The serial log handler */
struct lmk_serial_log_handler {
    struct lmk_log_handler base;
};

/* Maximum logger name length */
#define LMK_LOG_MAX_LOGGER_NAME_SZ 64

/* Logger */
struct lmk_logger {
    struct lmk_list link;
    char *name;
    int log_level;
    struct lmk_list handler_ref_list;
    int initialized;
    char log_buff[LMK_LOG_BUFFER_SIZE];
    lmk_mutex log_lock;
};

/* Logger reference */
struct lmk_log_handler_ref {
    struct lmk_list link;
    struct lmk_log_handler *handler; /**< Points to the log handler */
};

/* Logging macros  */

LMK_API void lmk_log_trace(struct lmk_logger *logger, const char *file_name, const int line_no, const char *format, ...);

LMK_API void lmk_log_debug(struct lmk_logger *logger, const char *file_name, const int line_no, const char *format, ...);

LMK_API void lmk_log_info(struct lmk_logger *logger, const char *file_name, const int line_no, const char *format, ...);

LMK_API void lmk_log_error(struct lmk_logger *logger, const char *file_name, const int line_no, const char *format, ...);

LMK_API void lmk_log_warn(struct lmk_logger *logger, const char *file_name, const int line_no, const char *format, ...);

LMK_API void lmk_log_fatal(struct lmk_logger *logger, const char *file_name, const int line_no, const char *format, ...);

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

extern LMK_API struct lmk_logger *lmk_get_logger(const char *name);

extern LMK_API void lmk_set_log_level(struct lmk_logger *logger, int log_level);

extern LMK_API int lmk_get_log_level(struct lmk_logger *logger);

extern LMK_API int lmk_destroy_logger(struct lmk_logger **logger_addr);

extern LMK_API struct lmk_log_handler *lmk_get_console_log_handler();

extern LMK_API struct lmk_log_handler *lmk_get_file_log_handler(const char *name, const char *filename);

extern LMK_API struct lmk_log_handler *lmk_get_socket_log_handler(const char *name);

extern LMK_API void lmk_set_log_filename(struct lmk_log_handler *handler, const char *filename);

extern LMK_API int lmk_attach_log_handler(struct lmk_logger *logger, struct lmk_log_handler *handler);

extern LMK_API int lmk_detach_log_handler(struct lmk_logger *logger, struct lmk_log_handler *handler);

extern LMK_API void lmk_set_handler_log_level(struct lmk_log_handler *handler, int log_level);

extern LMK_API int lmk_get_handler_log_level(struct lmk_log_handler *handler);

extern LMK_API int lmk_destroy_log_handler(struct lmk_log_handler **handler_addr);

extern LMK_API int lmk_get_nr_loggers();

extern LMK_API int lmk_get_nr_handlers();

extern struct lmk_log_handler *lmk_find_handler(struct lmk_logger *logger, const char *handler_name);

extern struct lmk_logger *lmk_search_logger_by_name(const char *name);

extern struct lmk_log_handler *lmk_search_log_handler_by_name(const char *name);

extern const char *lmk_get_log_handler_type_str(int type);

extern const char *lmk_get_log_level_str(int log_level);

extern struct lmk_list *lmk_get_loggers();

extern struct lmk_list *lmk_get_handlers();

extern void lmk_attach_log_listener(struct lmk_log_handler *handler, const char *ip, int port);

#endif /* LOGMOKO_H */
