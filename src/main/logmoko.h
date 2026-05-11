#ifndef LOGMOKO_H
#define LOGMOKO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <time.h>
#include <stdarg.h>
#include <signal.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#include "logmoko_types.h"
#include "logmoko_time.h"
#include "logmoko_connector.h"
#include "logmoko_mem.h"

#define LMK_DEBUG 0

#define LMK_VERSION_STRING "2.0.0-rcX"

struct lmk_config {
    unsigned int log_buffer_size;
    unsigned int ring_buffer_size;
};

enum {
    LMK_LOG_LEVEL_TRACE = 0,
    LMK_LOG_LEVEL_DEBUG,
    LMK_LOG_LEVEL_INFO,
    LMK_LOG_LEVEL_WARN,
    LMK_LOG_LEVEL_ERROR,
    LMK_LOG_LEVEL_OFF,
    LMK_LOG_LEVEL_UNKNOWN
};

enum {
    LMK_LOG_HANDLER_TYPE_CONSOLE = 0,
    LMK_LOG_HANDLER_TYPE_FILE,
    LMK_LOG_HANDLER_TYPE_SOCKET,
    LMK_LOG_HANDLER_TYPE_SYSLOG
};

struct lmk_log_record {
    struct lmk_list link;
    int log_level;
    char *data;
    size_t data_len;
    int line_no;
    char *file_name;
};

struct lmk_log_formatter {
    const char *name;
};

typedef int (*lmk_format_fn)(char *out, size_t out_size,
                              int log_level, const char *level_str,
                              const char *timestamp,
                              const char *file_name, int line_no,
                              const char *handler_name,
                              const char *message);

struct lmk_log_handler {
    struct lmk_list link;
    int log_level;
    struct lmk_log_formatter *formatter;
    void (*init)(struct lmk_log_handler *, void *);
    void (*destroy)(struct lmk_log_handler *, void *);
    void (*log_impl)(struct lmk_log_handler *, void *);
    int type;
    char *name;
    _Atomic unsigned long nr_log_calls;
    unsigned int nr_logger_ref;
    int initialized;
    pthread_mutex_t lock;
    lmk_format_fn format_fn;
};

typedef void (*lmk_log_handler_function)(struct lmk_log_handler *, void *);

#define LMK_LOG_MSG_MAX_SIZE 2048

struct lmk_log_request {
    int log_level;
    char data[LMK_LOG_MSG_MAX_SIZE];
    int line_no;
    char *file_name;
    char *handler_name;
};

/* Lock-free MPMC ring slot. sequence drives the Vyukov handshake. */
struct lmk_ring_slot {
    _Atomic size_t sequence;
    struct lmk_log_request req;
};

/* Round v up to the next power of two (minimum 2). */
static inline size_t lmk_next_pow2(size_t v) {
    if (v < 2) return 2;
    v--;
    v |= v >> 1; v |= v >> 2; v |= v >> 4;
    v |= v >> 8; v |= v >> 16; v |= v >> 32;
    return v + 1;
}

/* Initialise sequence numbers for a freshly allocated ring of `size` slots. */
static inline void lmk_ring_init(struct lmk_ring_slot *ring, size_t size) {
    for (size_t i = 0; i < size; i++)
        atomic_init(&ring[i].sequence, i);
}

/*
 * Multi-producer lock-free enqueue (Vyukov MPMC).
 * Copies rec->data (data_len bytes) plus metadata into a claimed slot.
 * Returns 1 on success, 0 if the ring is full (caller should count as dropped).
 */
static inline int lmk_ring_enqueue(struct lmk_ring_slot *ring,
                                    _Atomic size_t *head, size_t mask,
                                    const struct lmk_log_record *rec,
                                    const char *handler_name) {
    size_t pos = atomic_load_explicit(head, memory_order_relaxed);
    for (;;) {
        struct lmk_ring_slot *slot = &ring[pos & mask];
        size_t seq = atomic_load_explicit(&slot->sequence, memory_order_acquire);
        intptr_t diff = (intptr_t)seq - (intptr_t)pos;
        if (diff == 0) {
            if (atomic_compare_exchange_weak_explicit(head, &pos, pos + 1,
                    memory_order_relaxed, memory_order_relaxed)) {
                /* Slot claimed — write data, then mark ready. */
                slot->req.log_level    = rec->log_level;
                slot->req.line_no      = rec->line_no;
                slot->req.file_name    = rec->file_name;
                slot->req.handler_name = (char *)handler_name;
                size_t n = rec->data_len < LMK_LOG_MSG_MAX_SIZE - 1
                           ? rec->data_len : LMK_LOG_MSG_MAX_SIZE - 1;
                memcpy(slot->req.data, rec->data, n);
                slot->req.data[n] = '\0';
                atomic_store_explicit(&slot->sequence, pos + 1, memory_order_release);
                return 1;
            }
            /* CAS lost — another producer took this position; retry. */
        } else if (diff < 0) {
            return 0; /* ring full */
        } else {
            /* diff > 0: this slot is already claimed/ready; reload head and retry. */
            pos = atomic_load_explicit(head, memory_order_relaxed);
        }
    }
}

/*
 * Single-consumer peek: returns a pointer to the next ready slot without
 * consuming it, or NULL if the ring is empty / slot not yet ready.
 * Safe to call only from the single consumer thread.
 */
static inline struct lmk_ring_slot *lmk_ring_peek_slot(struct lmk_ring_slot *ring,
                                                         size_t tail, size_t mask) {
    struct lmk_ring_slot *slot = &ring[tail & mask];
    size_t seq = atomic_load_explicit(&slot->sequence, memory_order_acquire);
    return (intptr_t)seq - (intptr_t)(tail + 1) == 0 ? slot : NULL;
}

/*
 * Advance the consumer tail after processing the slot returned by
 * lmk_ring_peek_slot().  Marks the slot free for the next producer wrap-around.
 */
static inline void lmk_ring_consume(struct lmk_ring_slot *ring,
                                     size_t *tail, size_t mask) {
    size_t pos = *tail;
    atomic_store_explicit(&ring[pos & mask].sequence,
                          pos + mask + 1, memory_order_release);
    *tail = pos + 1;
}

/* ------------------------------------------------------------------ */
/* Handler structs — all use the lock-free ring.                       */
/* ------------------------------------------------------------------ */

#define LMK_DEFAULT_MAX_FILE_SIZE    (20L * 1024 * 1024)
#define LMK_DEFAULT_MAX_BACKUP_FILES 10

struct lmk_file_log_handler {
    struct lmk_log_handler base;
    const char *filename;
    FILE *log_fp;
    struct lmk_ring_slot *ring_buffer;
    _Atomic size_t head;      /* producer cursor (monotonically increasing) */
    size_t tail;              /* consumer cursor (single consumer)           */
    size_t ring_mask;         /* ring_size - 1, ring_size is a power of two  */
    pthread_t thread;
    _Atomic int running;
    _Atomic int sleeping;     /* 1 while consumer is in cond_wait            */
    pthread_mutex_t wakeup_lock;
    pthread_cond_t  wakeup_cond;
    long max_file_size;
    int  max_backup_files;
    long current_file_size;
    _Atomic unsigned long dropped;
};

struct lmk_console_log_handler {
    struct lmk_log_handler base;
    struct lmk_ring_slot *ring_buffer;
    _Atomic size_t head;
    size_t tail;
    size_t ring_mask;
    pthread_t thread;
    _Atomic int running;
    _Atomic int sleeping;
    pthread_mutex_t wakeup_lock;
    pthread_cond_t  wakeup_cond;
    _Atomic unsigned long dropped;
};

struct lmk_socket_log_handler {
    struct lmk_log_handler base;
    struct lmk_list log_server_list;
    struct lmk_udp_socket socket_object;
    struct lmk_ring_slot *ring_buffer;
    _Atomic size_t head;
    size_t tail;
    size_t ring_mask;
    pthread_t thread;
    _Atomic int running;
    _Atomic int sleeping;
    pthread_mutex_t wakeup_lock;
    pthread_cond_t  wakeup_cond;
    _Atomic unsigned long dropped;
};

struct lmk_syslog_log_handler {
    struct lmk_log_handler base;
    char ident[64];
    int  facility;
    struct lmk_ring_slot *ring_buffer;
    _Atomic size_t head;
    size_t tail;
    size_t ring_mask;
    pthread_t thread;
    _Atomic int running;
    _Atomic int sleeping;
    pthread_mutex_t wakeup_lock;
    pthread_cond_t  wakeup_cond;
    _Atomic unsigned long dropped;
};

#define LMK_LOG_MAX_LOGGER_NAME_SZ 64

/* log_buff and log_lock removed — callers use a thread-local buffer. */
struct lmk_logger {
    struct lmk_list link;
    char *name;
    int log_level;
    struct lmk_list handler_ref_list;
    int initialized;
};

struct lmk_log_handler_ref {
    struct lmk_list link;
    struct lmk_log_handler *handler;
};

LMK_API void lmk_log_trace(struct lmk_logger *logger, const char *file_name, const int line_no, const char *format, ...);
LMK_API void lmk_log_debug(struct lmk_logger *logger, const char *file_name, const int line_no, const char *format, ...);
LMK_API void lmk_log_info (struct lmk_logger *logger, const char *file_name, const int line_no, const char *format, ...);
LMK_API void lmk_log_error(struct lmk_logger *logger, const char *file_name, const int line_no, const char *format, ...);
LMK_API void lmk_log_warn (struct lmk_logger *logger, const char *file_name, const int line_no, const char *format, ...);

#define LMK_LOG_TRACE(logger, format...) lmk_log_trace(logger, __FILE__, __LINE__, format);
#define LMK_LOG_INFO( logger, format...) lmk_log_info (logger, __FILE__, __LINE__, format);
#define LMK_LOG_DEBUG(logger, format...) lmk_log_debug(logger, __FILE__, __LINE__, format);
#define LMK_LOG_WARN( logger, format...) lmk_log_warn (logger, __FILE__, __LINE__, format);
#define LMK_LOG_ERROR(logger, format...) lmk_log_error(logger, __FILE__, __LINE__, format);

#define LMK_IS_LOG_ENABLED(logger_or_handler, level) \
    (level >= (logger_or_handler)->log_level)

#define LMK_IS_VALID_LOG_LEVEL(level) \
    (level >= LMK_LOG_LEVEL_TRACE && level <= LMK_LOG_LEVEL_OFF)

extern LMK_API void lmk_init();
extern LMK_API int  lmk_init_from_file(const char *path);
extern LMK_API void lmk_destroy();
extern void lmk_dump_loggers();

extern LMK_API struct lmk_logger *lmk_get_logger(const char *name);
extern LMK_API void lmk_set_log_level(struct lmk_logger *logger, int log_level);
extern LMK_API int  lmk_get_log_level(struct lmk_logger *logger);
extern LMK_API int  lmk_destroy_logger(struct lmk_logger **logger_addr);

extern LMK_API struct lmk_log_handler *lmk_get_console_log_handler();
extern LMK_API struct lmk_log_handler *lmk_get_file_log_handler(const char *name, const char *filename);
extern LMK_API struct lmk_log_handler *lmk_get_socket_log_handler(const char *name);
extern LMK_API struct lmk_log_handler *lmk_get_syslog_log_handler(const char *name, const char *ident, int facility);

extern LMK_API void lmk_set_log_filename(struct lmk_log_handler *handler, const char *filename);
extern LMK_API void lmk_set_log_rotation(struct lmk_log_handler *handler, long max_file_size, int max_backup_files);
extern LMK_API int  lmk_attach_log_handler(struct lmk_logger *logger, struct lmk_log_handler *handler);
extern LMK_API int  lmk_detach_log_handler(struct lmk_logger *logger, struct lmk_log_handler *handler);
extern LMK_API void lmk_set_handler_log_level(struct lmk_log_handler *handler, int log_level);
extern LMK_API int  lmk_get_handler_log_level(struct lmk_log_handler *handler);
extern LMK_API int  lmk_destroy_log_handler(struct lmk_log_handler **handler_addr);
extern LMK_API void lmk_set_log_format(struct lmk_log_handler *handler, lmk_format_fn fn);

extern lmk_format_fn lmk_get_format_fn(const char *name);
extern LMK_API int  lmk_get_nr_loggers();
extern LMK_API int  lmk_get_nr_handlers();

extern struct lmk_log_handler *lmk_find_handler(struct lmk_logger *logger, const char *handler_name);
extern struct lmk_logger      *lmk_search_logger_by_name(const char *name);
extern struct lmk_log_handler *lmk_search_log_handler_by_name(const char *name);
extern const char *lmk_get_log_handler_type_str(int type);
extern const char *lmk_get_log_level_str(int log_level);
extern struct lmk_list *lmk_get_loggers();
extern struct lmk_list *lmk_get_handlers();
extern void lmk_attach_log_listener(struct lmk_log_handler *handler, const char *ip, int port);
extern struct lmk_config *lmk_get_config();

/* Returns the number of characters written (snprintf return value, clamped). */
extern int lmk_format_log_line(struct lmk_log_handler *handler, char *out, size_t out_size,
                                const struct lmk_log_request *req);

#define LMK_CFG_MAX_LISTENERS           16
#define LMK_CFG_MAX_HANDLERS            32
#define LMK_CFG_MAX_LOGGERS             64
#define LMK_CFG_MAX_HANDLERS_PER_LOGGER 16
#define LMK_CFG_LINE_MAX                512
#define LMK_CFG_DEFAULT_RING_BUFFER_SIZE 1024
#define LMK_CFG_DEFAULT_LOG_BUFFER_SIZE  2048

#endif /* LOGMOKO_H */
