#include "logmoko.h"

static void lmk_rotate_log_file(struct lmk_file_log_handler *flh) {
    fclose(flh->log_fp);
    flh->log_fp = NULL;

    char old_path[512], new_path[512];
    for (int i = flh->max_backup_files - 1; i >= 1; i--) {
        snprintf(old_path, sizeof(old_path), "%s.%d", flh->filename, i);
        snprintf(new_path, sizeof(new_path), "%s.%d", flh->filename, i + 1);
        rename(old_path, new_path);
    }
    if (flh->max_backup_files > 0) {
        snprintf(new_path, sizeof(new_path), "%s.1", flh->filename);
        rename(flh->filename, new_path);
    }
    flh->log_fp = fopen(flh->filename, "wt");
    flh->current_file_size = 0;
    if (!flh->log_fp)
        fprintf(stderr, "logmoko: unable to open log file after rotation: %s\n",
                flh->filename);
}

/*
 * Wake the consumer.  Only called after a successful ring enqueue when the
 * sleeping flag indicates the consumer is blocked in cond_wait (#4).
 * The seq-cst fence ensures the ring slot's "ready" store is globally visible
 * before we read the sleeping flag, preventing a lost-wakeup race.
 */
static inline void lmk_file_maybe_wake(struct lmk_file_log_handler *flh) {
    atomic_thread_fence(memory_order_seq_cst);
    if (atomic_load_explicit(&flh->sleeping, memory_order_seq_cst)) {
        pthread_mutex_lock(&flh->wakeup_lock);
        pthread_cond_signal(&flh->wakeup_cond);
        pthread_mutex_unlock(&flh->wakeup_lock);
    }
}

#define LMK_WRITE_BUF_SIZE (64 * 1024)
/* Headroom reserved before formatting so snprintf always has enough space. */
#define LMK_MAX_LINE_SIZE  (LMK_LOG_MSG_MAX_SIZE + 512)

static void *lmk_file_log_handler_thread_routine(void *arg) {
    struct lmk_file_log_handler *flh = (struct lmk_file_log_handler *)arg;
    char wbuf[LMK_WRITE_BUF_SIZE];
    size_t wpos = 0;

    while (1) {
        /* Drain every ready slot, formatting directly into wbuf (#1). */
        struct lmk_ring_slot *slot;
        while ((slot = lmk_ring_peek_slot(flh->ring_buffer, flh->tail, flh->ring_mask))) {
            if (flh->log_fp) {
                /* Flush staging buffer first if there isn't enough headroom. */
                if (wpos + LMK_MAX_LINE_SIZE > LMK_WRITE_BUF_SIZE) {
                    fwrite(wbuf, 1, wpos, flh->log_fp);
                    flh->current_file_size += (long)wpos;
                    if (flh->max_file_size > 0 &&
                        flh->current_file_size >= flh->max_file_size)
                        lmk_rotate_log_file(flh);
                    wpos = 0;
                }
                int n = lmk_format_log_line(&flh->base, wbuf + wpos,
                                            LMK_WRITE_BUF_SIZE - wpos, &slot->req);
                if (n > 0)
                    wpos += (size_t)n;
            }
            lmk_ring_consume(flh->ring_buffer, &flh->tail, flh->ring_mask);
        }

        /* Ring drained — flush staging buffer then fflush (idle flush, #5). */
        if (wpos > 0 && flh->log_fp) {
            fwrite(wbuf, 1, wpos, flh->log_fp);
            flh->current_file_size += (long)wpos;
            if (flh->max_file_size > 0 &&
                flh->current_file_size >= flh->max_file_size)
                lmk_rotate_log_file(flh);
            wpos = 0;
            fflush(flh->log_fp);
        }

        /* Exit after draining all remaining items on shutdown. */
        if (!atomic_load_explicit(&flh->running, memory_order_acquire))
            break;

        /*
         * Sleep until the next enqueue.  Set sleeping=1 under wakeup_lock so
         * that a concurrent producer that sees sleeping=1 MUST acquire
         * wakeup_lock to signal — guaranteeing the signal isn't lost (#4).
         * Re-peek the ring under the lock to close the TOCTOU window.
         */
        pthread_mutex_lock(&flh->wakeup_lock);
        atomic_store_explicit(&flh->sleeping, 1, memory_order_seq_cst);
        while (!lmk_ring_peek_slot(flh->ring_buffer, flh->tail, flh->ring_mask) &&
               atomic_load_explicit(&flh->running, memory_order_relaxed))
            pthread_cond_wait(&flh->wakeup_cond, &flh->wakeup_lock);
        atomic_store_explicit(&flh->sleeping, 0, memory_order_relaxed);
        pthread_mutex_unlock(&flh->wakeup_lock);
    }

    /* Final flush of any remaining staged bytes after shutdown drain. */
    if (wpos > 0 && flh->log_fp) {
        fwrite(wbuf, 1, wpos, flh->log_fp);
        wpos = 0;
    }
    if (flh->log_fp)
        fflush(flh->log_fp);

    return NULL;
}

void lmk_file_log_handler_init(struct lmk_log_handler *handler, void *param) {
    struct lmk_file_log_handler *flh = (struct lmk_file_log_handler *)handler;
    pthread_mutex_lock(&handler->lock);

    if (flh->log_fp) {
        fclose(flh->log_fp);
        flh->log_fp = NULL;
    }
    if ((flh->log_fp = fopen(flh->filename, "at")) == NULL) {
        fprintf(stderr, "Unable to open file for writing : %s (%s:%d)\n",
                flh->filename, __FILE__, __LINE__);
    }
    if (flh->log_fp) {
        if (fseek(flh->log_fp, 0, SEEK_END) == 0) {
            long sz = ftell(flh->log_fp);
            flh->current_file_size = sz >= 0 ? sz : 0;
        }
    }

    /* Round ring size up to the next power of two for the index mask (#1). */
    size_t ring_size = lmk_next_pow2((size_t)lmk_get_config()->ring_buffer_size);
    flh->ring_mask = ring_size - 1;
    flh->ring_buffer = lmk_malloc(sizeof(struct lmk_ring_slot) * ring_size);
    if (!flh->ring_buffer) {
        fprintf(stderr, "logmoko: unable to allocate file handler ring buffer\n");
        if (flh->log_fp) { fclose(flh->log_fp); flh->log_fp = NULL; }
        pthread_mutex_unlock(&handler->lock);
        return;
    }
    lmk_ring_init(flh->ring_buffer, ring_size);
    atomic_init(&flh->head, 0);
    flh->tail = 0;
    atomic_init(&flh->running, 1);
    atomic_init(&flh->sleeping, 0);
    atomic_init(&flh->dropped, 0);

    pthread_mutex_init(&flh->wakeup_lock, NULL);
    pthread_cond_init(&flh->wakeup_cond, NULL);

    if (pthread_create(&flh->thread, NULL,
                       lmk_file_log_handler_thread_routine, flh) != 0) {
        fprintf(stderr, "logmoko: unable to create file handler thread\n");
        lmk_free(flh->ring_buffer);
        flh->ring_buffer = NULL;
        atomic_store(&flh->running, 0);
        if (flh->log_fp) { fclose(flh->log_fp); flh->log_fp = NULL; }
        pthread_cond_destroy(&flh->wakeup_cond);
        pthread_mutex_destroy(&flh->wakeup_lock);
    }
    pthread_mutex_unlock(&handler->lock);
}

void lmk_file_log_handler_log_impl(struct lmk_log_handler *handler, void *param) {
    struct lmk_file_log_handler *flh = (struct lmk_file_log_handler *)handler;
    struct lmk_log_record *rec = (struct lmk_log_record *)param;

    if (!atomic_load_explicit(&flh->running, memory_order_relaxed))
        return;

    /* Lock-free enqueue (#1). Data is copied from the TLS buffer (#2/#3). */
    if (!lmk_ring_enqueue(flh->ring_buffer, &flh->head, flh->ring_mask,
                          rec, handler->name)) {
        atomic_fetch_add_explicit(&flh->dropped, 1, memory_order_relaxed);
        return;
    }
    atomic_fetch_add_explicit(&handler->nr_log_calls, 1, memory_order_relaxed);
    lmk_file_maybe_wake(flh); /* signal only when consumer is sleeping (#4) */
}

void lmk_file_log_handler_destroy(struct lmk_log_handler *handler, void *param) {
    struct lmk_file_log_handler *flh = (struct lmk_file_log_handler *)handler;

    atomic_store_explicit(&flh->running, 0, memory_order_release);
    /* Wake the consumer so it can drain and exit. */
    pthread_mutex_lock(&flh->wakeup_lock);
    pthread_cond_broadcast(&flh->wakeup_cond);
    pthread_mutex_unlock(&flh->wakeup_lock);
    pthread_join(flh->thread, NULL);

    pthread_cond_destroy(&flh->wakeup_cond);
    pthread_mutex_destroy(&flh->wakeup_lock);
    lmk_free(flh->ring_buffer);
    flh->ring_buffer = NULL;

    unsigned long dropped = atomic_load(&flh->dropped);
    unsigned long logged  = atomic_load(&handler->nr_log_calls);
    if (dropped)
        fprintf(stderr, "logmoko: file handler '%s' dropped %lu log(s), logged %lu\n",
                handler->name, dropped, logged);

    pthread_mutex_lock(&handler->lock);
    if (flh->log_fp) { fclose(flh->log_fp); flh->log_fp = NULL; }
    lmk_free((void *)flh->filename);
    flh->filename = NULL;
    pthread_mutex_unlock(&handler->lock);
}

LMK_API void lmk_set_log_rotation(struct lmk_log_handler *handler,
                                   long max_file_size, int max_backup_files) {
    if (handler && handler->type == LMK_LOG_HANDLER_TYPE_FILE) {
        struct lmk_file_log_handler *flh = (struct lmk_file_log_handler *)handler;
        pthread_mutex_lock(&handler->lock);
        flh->max_file_size    = max_file_size;
        flh->max_backup_files = max_backup_files;
        pthread_mutex_unlock(&handler->lock);
    }
}

LMK_API void lmk_set_log_filename(struct lmk_log_handler *handler, const char *filename) {
    pthread_mutex_lock(&handler->lock);
    if (handler && filename && handler->type == LMK_LOG_HANDLER_TYPE_FILE) {
        struct lmk_file_log_handler *flh = (struct lmk_file_log_handler *)handler;
        lmk_free((void *)flh->filename);
        flh->filename = lmk_strdup(filename);
    }
    pthread_mutex_unlock(&handler->lock);
}
