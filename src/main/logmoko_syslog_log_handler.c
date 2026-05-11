#include "logmoko.h"
#include <syslog.h>

static int lmk_to_syslog_priority(int level) {
    switch (level) {
        case LMK_LOG_LEVEL_TRACE: return LOG_DEBUG;
        case LMK_LOG_LEVEL_DEBUG: return LOG_DEBUG;
        case LMK_LOG_LEVEL_INFO:  return LOG_INFO;
        case LMK_LOG_LEVEL_WARN:  return LOG_WARNING;
        case LMK_LOG_LEVEL_ERROR: return LOG_ERR;
        default:                  return LOG_INFO;
    }
}

int lmk_parse_syslog_facility(const char *s) {
    if (!s || !strcasecmp(s, "user"))   return LOG_USER;
    if (!strcasecmp(s, "daemon"))       return LOG_DAEMON;
    if (!strcasecmp(s, "local0"))       return LOG_LOCAL0;
    if (!strcasecmp(s, "local1"))       return LOG_LOCAL1;
    if (!strcasecmp(s, "local2"))       return LOG_LOCAL2;
    if (!strcasecmp(s, "local3"))       return LOG_LOCAL3;
    if (!strcasecmp(s, "local4"))       return LOG_LOCAL4;
    if (!strcasecmp(s, "local5"))       return LOG_LOCAL5;
    if (!strcasecmp(s, "local6"))       return LOG_LOCAL6;
    if (!strcasecmp(s, "local7"))       return LOG_LOCAL7;
    return LOG_USER;
}

static inline void lmk_syslog_maybe_wake(struct lmk_syslog_log_handler *slh) {
    atomic_thread_fence(memory_order_seq_cst);
    if (atomic_load_explicit(&slh->sleeping, memory_order_seq_cst)) {
        pthread_mutex_lock(&slh->wakeup_lock);
        pthread_cond_signal(&slh->wakeup_cond);
        pthread_mutex_unlock(&slh->wakeup_lock);
    }
}

static void *lmk_syslog_log_handler_thread_routine(void *arg) {
    struct lmk_syslog_log_handler *slh = (struct lmk_syslog_log_handler *)arg;

    while (1) {
        struct lmk_ring_slot *slot;
        while ((slot = lmk_ring_peek_slot(slh->ring_buffer, slh->tail, slh->ring_mask))) {
            syslog(lmk_to_syslog_priority(slot->req.log_level),
                   "[%s] (%s:%d) %s",
                   lmk_get_log_level_str(slot->req.log_level),
                   slot->req.file_name, slot->req.line_no, slot->req.data);
            lmk_ring_consume(slh->ring_buffer, &slh->tail, slh->ring_mask);
        }

        if (!atomic_load_explicit(&slh->running, memory_order_acquire))
            break;

        pthread_mutex_lock(&slh->wakeup_lock);
        atomic_store_explicit(&slh->sleeping, 1, memory_order_seq_cst);
        while (!lmk_ring_peek_slot(slh->ring_buffer, slh->tail, slh->ring_mask) &&
               atomic_load_explicit(&slh->running, memory_order_relaxed))
            pthread_cond_wait(&slh->wakeup_cond, &slh->wakeup_lock);
        atomic_store_explicit(&slh->sleeping, 0, memory_order_relaxed);
        pthread_mutex_unlock(&slh->wakeup_lock);
    }
    return NULL;
}

void lmk_syslog_log_handler_init(struct lmk_log_handler *handler, void *param) {
    struct lmk_syslog_log_handler *slh = (struct lmk_syslog_log_handler *)handler;
    pthread_mutex_lock(&handler->lock);
    openlog(slh->ident[0] ? slh->ident : NULL, LOG_PID, slh->facility);

    size_t ring_size = lmk_next_pow2((size_t)lmk_get_config()->ring_buffer_size);
    slh->ring_mask   = ring_size - 1;
    slh->ring_buffer = lmk_malloc(sizeof(struct lmk_ring_slot) * ring_size);
    if (!slh->ring_buffer) {
        fprintf(stderr, "logmoko: unable to allocate syslog handler ring buffer\n");
        closelog();
        pthread_mutex_unlock(&handler->lock);
        return;
    }
    lmk_ring_init(slh->ring_buffer, ring_size);
    atomic_init(&slh->head, 0);
    slh->tail = 0;
    atomic_init(&slh->running, 1);
    atomic_init(&slh->sleeping, 0);
    atomic_init(&slh->dropped, 0);
    pthread_mutex_init(&slh->wakeup_lock, NULL);
    pthread_cond_init(&slh->wakeup_cond, NULL);

    if (pthread_create(&slh->thread, NULL,
                       lmk_syslog_log_handler_thread_routine, slh) != 0) {
        fprintf(stderr, "logmoko: unable to create syslog handler thread\n");
        lmk_free(slh->ring_buffer);
        slh->ring_buffer = NULL;
        atomic_store(&slh->running, 0);
        closelog();
        pthread_cond_destroy(&slh->wakeup_cond);
        pthread_mutex_destroy(&slh->wakeup_lock);
    }
    pthread_mutex_unlock(&handler->lock);
}

void lmk_syslog_log_handler_destroy(struct lmk_log_handler *handler, void *param) {
    struct lmk_syslog_log_handler *slh = (struct lmk_syslog_log_handler *)handler;

    atomic_store_explicit(&slh->running, 0, memory_order_release);
    pthread_mutex_lock(&slh->wakeup_lock);
    pthread_cond_broadcast(&slh->wakeup_cond);
    pthread_mutex_unlock(&slh->wakeup_lock);
    pthread_join(slh->thread, NULL);
    pthread_cond_destroy(&slh->wakeup_cond);
    pthread_mutex_destroy(&slh->wakeup_lock);

    unsigned long dropped = atomic_load(&slh->dropped);
    if (dropped)
        fprintf(stderr, "logmoko: syslog handler '%s' dropped %lu log(s), logged %lu\n",
                handler->name, dropped, atomic_load(&handler->nr_log_calls));
    lmk_free(slh->ring_buffer);
    slh->ring_buffer = NULL;
    closelog();
}

void lmk_syslog_log_handler_log_impl(struct lmk_log_handler *handler, void *param) {
    struct lmk_syslog_log_handler *slh = (struct lmk_syslog_log_handler *)handler;
    struct lmk_log_record *rec = (struct lmk_log_record *)param;

    if (!atomic_load_explicit(&slh->running, memory_order_relaxed))
        return;
    if (!lmk_ring_enqueue(slh->ring_buffer, &slh->head, slh->ring_mask,
                          rec, handler->name)) {
        atomic_fetch_add_explicit(&slh->dropped, 1, memory_order_relaxed);
        return;
    }
    atomic_fetch_add_explicit(&handler->nr_log_calls, 1, memory_order_relaxed);
    lmk_syslog_maybe_wake(slh);
}
