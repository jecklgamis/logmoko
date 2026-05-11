#include "logmoko.h"

#define ANSI_RESET  "\033[0m"
#define ANSI_GRAY   "\033[2;37m"
#define ANSI_CYAN   "\033[0;36m"
#define ANSI_WHITE  "\033[0;37m"
#define ANSI_YELLOW "\033[0;33m"
#define ANSI_RED    "\033[0;31m"

static const char *lmk_console_level_color(int level) {
    switch (level) {
        case LMK_LOG_LEVEL_TRACE: return ANSI_GRAY;
        case LMK_LOG_LEVEL_DEBUG: return ANSI_CYAN;
        case LMK_LOG_LEVEL_INFO:  return ANSI_WHITE;
        case LMK_LOG_LEVEL_WARN:  return ANSI_YELLOW;
        case LMK_LOG_LEVEL_ERROR: return ANSI_RED;
        default:                  return ANSI_RESET;
    }
}

static inline void lmk_console_maybe_wake(struct lmk_console_log_handler *clh) {
    atomic_thread_fence(memory_order_seq_cst);
    if (atomic_load_explicit(&clh->sleeping, memory_order_seq_cst)) {
        pthread_mutex_lock(&clh->wakeup_lock);
        pthread_cond_signal(&clh->wakeup_cond);
        pthread_mutex_unlock(&clh->wakeup_lock);
    }
}

static void *lmk_console_log_handler_thread_routine(void *arg) {
    struct lmk_console_log_handler *clh = (struct lmk_console_log_handler *)arg;
    char out[LMK_LOG_MSG_MAX_SIZE + 512];

    while (1) {
        struct lmk_ring_slot *slot;
        while ((slot = lmk_ring_peek_slot(clh->ring_buffer, clh->tail, clh->ring_mask))) {
            lmk_format_log_line(&clh->base, out, sizeof(out), &slot->req);
            fputs(lmk_console_level_color(slot->req.log_level), stdout);
            fputs(out, stdout);
            fputs(ANSI_RESET, stdout);
            lmk_ring_consume(clh->ring_buffer, &clh->tail, clh->ring_mask);
        }
        fflush(stdout);

        if (!atomic_load_explicit(&clh->running, memory_order_acquire))
            break;

        pthread_mutex_lock(&clh->wakeup_lock);
        atomic_store_explicit(&clh->sleeping, 1, memory_order_seq_cst);
        while (!lmk_ring_peek_slot(clh->ring_buffer, clh->tail, clh->ring_mask) &&
               atomic_load_explicit(&clh->running, memory_order_relaxed))
            pthread_cond_wait(&clh->wakeup_cond, &clh->wakeup_lock);
        atomic_store_explicit(&clh->sleeping, 0, memory_order_relaxed);
        pthread_mutex_unlock(&clh->wakeup_lock);
    }
    return NULL;
}

void lmk_console_log_handler_init(struct lmk_log_handler *handler, void *param) {
    struct lmk_console_log_handler *clh = (struct lmk_console_log_handler *)handler;

    size_t ring_size = lmk_next_pow2((size_t)lmk_get_config()->ring_buffer_size);
    clh->ring_mask   = ring_size - 1;
    clh->ring_buffer = lmk_malloc(sizeof(struct lmk_ring_slot) * ring_size);
    if (!clh->ring_buffer) {
        fprintf(stderr, "logmoko: unable to allocate console ring buffer\n");
        return;
    }
    lmk_ring_init(clh->ring_buffer, ring_size);
    atomic_init(&clh->head, 0);
    clh->tail = 0;
    atomic_init(&clh->running, 1);
    atomic_init(&clh->sleeping, 0);
    atomic_init(&clh->dropped, 0);
    pthread_mutex_init(&clh->wakeup_lock, NULL);
    pthread_cond_init(&clh->wakeup_cond, NULL);

    if (pthread_create(&clh->thread, NULL,
                       lmk_console_log_handler_thread_routine, clh) != 0) {
        fprintf(stderr, "logmoko: unable to create console handler thread\n");
        lmk_free(clh->ring_buffer);
        clh->ring_buffer = NULL;
        atomic_store(&clh->running, 0);
        pthread_cond_destroy(&clh->wakeup_cond);
        pthread_mutex_destroy(&clh->wakeup_lock);
    }
}

void lmk_console_log_handler_destroy(struct lmk_log_handler *handler, void *param) {
    struct lmk_console_log_handler *clh = (struct lmk_console_log_handler *)handler;

    atomic_store_explicit(&clh->running, 0, memory_order_release);
    pthread_mutex_lock(&clh->wakeup_lock);
    pthread_cond_broadcast(&clh->wakeup_cond);
    pthread_mutex_unlock(&clh->wakeup_lock);
    pthread_join(clh->thread, NULL);
    pthread_cond_destroy(&clh->wakeup_cond);
    pthread_mutex_destroy(&clh->wakeup_lock);

    unsigned long dropped = atomic_load(&clh->dropped);
    if (dropped)
        fprintf(stderr, "logmoko: console handler dropped %lu log(s), logged %lu\n",
                dropped, atomic_load(&handler->nr_log_calls));

    lmk_free(clh->ring_buffer);
    clh->ring_buffer = NULL;
}

void lmk_console_log_handler_log_impl(struct lmk_log_handler *handler, void *param) {
    struct lmk_console_log_handler *clh = (struct lmk_console_log_handler *)handler;
    struct lmk_log_record *rec = (struct lmk_log_record *)param;

    if (!atomic_load_explicit(&clh->running, memory_order_relaxed))
        return;
    if (!lmk_ring_enqueue(clh->ring_buffer, &clh->head, clh->ring_mask,
                          rec, handler->name)) {
        atomic_fetch_add_explicit(&clh->dropped, 1, memory_order_relaxed);
        return;
    }
    atomic_fetch_add_explicit(&handler->nr_log_calls, 1, memory_order_relaxed);
    lmk_console_maybe_wake(clh);
}
