#include "logmoko.h"
#include <unistd.h>

static void lmk_sync_rotate_log_file(struct lmk_sync_file_log_handler *sfh) {
    fclose(sfh->log_fp);
    sfh->log_fp = NULL;

    char old_path[512], new_path[512];
    for (int i = sfh->max_backup_files - 1; i >= 1; i--) {
        snprintf(old_path, sizeof(old_path), "%s.%d", sfh->filename, i);
        snprintf(new_path, sizeof(new_path), "%s.%d", sfh->filename, i + 1);
        rename(old_path, new_path);
    }
    if (sfh->max_backup_files > 0) {
        snprintf(new_path, sizeof(new_path), "%s.1", sfh->filename);
        rename(sfh->filename, new_path);
    }
    sfh->log_fp = fopen(sfh->filename, "wt");
    sfh->current_file_size = 0;
    if (!sfh->log_fp)
        fprintf(stderr, "logmoko: unable to open sync log file after rotation: %s\n",
                sfh->filename);
}

static int lmk_sync_dequeue(struct lmk_sync_file_log_handler *sfh,
                            struct lmk_log_request *req) {
    pthread_mutex_lock(&sfh->queue_lock);
    while (sfh->count == 0 && sfh->running)
        pthread_cond_wait(&sfh->not_empty, &sfh->queue_lock);

    if (sfh->count == 0 && !sfh->running) {
        pthread_mutex_unlock(&sfh->queue_lock);
        return 0;
    }

    struct lmk_sync_file_log_slot *slot = &sfh->buffer[sfh->tail];
    req->log_level    = slot->log_level;
    req->line_no      = slot->line_no;
    req->file_name    = slot->file_name;
    req->handler_name = slot->handler_name;
    memcpy(req->data, slot->data, slot->data_len);
    req->data[slot->data_len] = '\0';
    sfh->tail = (sfh->tail + 1) % sfh->buffer_size;
    sfh->count--;
    pthread_cond_signal(&sfh->not_full);
    pthread_mutex_unlock(&sfh->queue_lock);
    return 1;
}

static void lmk_sync_flush(struct lmk_sync_file_log_handler *sfh) {
    if (sfh->log_fp) {
        fflush(sfh->log_fp);
        fsync(fileno(sfh->log_fp));
    }
}

#define LMK_SYNC_WRITE_BUF_SIZE (256 * 1024)
#define LMK_SYNC_MAX_LINE_SIZE  (LMK_LOG_MSG_MAX_SIZE + 512)

static void lmk_sync_write_buffer(struct lmk_sync_file_log_handler *sfh,
                                  const char *buf, size_t *pos) {
    if (*pos == 0 || !sfh->log_fp)
        return;
    fwrite(buf, 1, *pos, sfh->log_fp);
    sfh->current_file_size += (long)*pos;
    *pos = 0;
}

static void *lmk_sync_file_log_handler_thread_routine(void *arg) {
    struct lmk_sync_file_log_handler *sfh = (struct lmk_sync_file_log_handler *)arg;
    struct lmk_log_request req;
    char wbuf[LMK_SYNC_WRITE_BUF_SIZE];
    size_t wpos = 0;

    while (lmk_sync_dequeue(sfh, &req)) {
        if (wpos + LMK_SYNC_MAX_LINE_SIZE > LMK_SYNC_WRITE_BUF_SIZE) {
            pthread_mutex_lock(&sfh->base.lock);
            lmk_sync_write_buffer(sfh, wbuf, &wpos);
            if (sfh->max_file_size > 0 &&
                sfh->current_file_size >= sfh->max_file_size) {
                lmk_sync_flush(sfh);
                lmk_sync_rotate_log_file(sfh);
            }
            pthread_mutex_unlock(&sfh->base.lock);
        }

        int len = lmk_format_log_line(&sfh->base, wbuf + wpos,
                                      LMK_SYNC_WRITE_BUF_SIZE - wpos, &req);
        if (len <= 0)
            continue;
        wpos += (size_t)len;

        if (sfh->max_file_size > 0 &&
            sfh->current_file_size + (long)wpos >= sfh->max_file_size) {
            pthread_mutex_lock(&sfh->base.lock);
            lmk_sync_write_buffer(sfh, wbuf, &wpos);
            if (sfh->current_file_size >= sfh->max_file_size) {
                lmk_sync_flush(sfh);
                lmk_sync_rotate_log_file(sfh);
            }
            pthread_mutex_unlock(&sfh->base.lock);
        }
    }

    pthread_mutex_lock(&sfh->base.lock);
    lmk_sync_write_buffer(sfh, wbuf, &wpos);
    lmk_sync_flush(sfh);
    pthread_mutex_unlock(&sfh->base.lock);
    return NULL;
}

void lmk_sync_file_log_handler_init(struct lmk_log_handler *handler, void *param) {
    struct lmk_sync_file_log_handler *sfh = (struct lmk_sync_file_log_handler *)handler;
    pthread_mutex_lock(&handler->lock);
    if (sfh->log_fp) {
        fclose(sfh->log_fp);
        sfh->log_fp = NULL;
    }
    if ((sfh->log_fp = fopen(sfh->filename, "at")) == NULL) {
        fprintf(stderr, "logmoko: unable to open sync log file: %s\n", sfh->filename);
        pthread_mutex_unlock(&handler->lock);
        return;
    }
    if (fseek(sfh->log_fp, 0, SEEK_END) == 0) {
        long sz = ftell(sfh->log_fp);
        sfh->current_file_size = sz >= 0 ? sz : 0;
    }

    sfh->buffer_size = lmk_next_pow2((size_t)lmk_get_config()->ring_buffer_size);
    sfh->buffer = lmk_malloc(sizeof(struct lmk_sync_file_log_slot) * sfh->buffer_size);
    if (!sfh->buffer) {
        fprintf(stderr, "logmoko: unable to allocate sync file buffer\n");
        fclose(sfh->log_fp);
        sfh->log_fp = NULL;
        pthread_mutex_unlock(&handler->lock);
        return;
    }

    sfh->head = 0;
    sfh->tail = 0;
    sfh->count = 0;
    sfh->running = 1;
    pthread_mutex_init(&sfh->queue_lock, NULL);
    pthread_cond_init(&sfh->not_empty, NULL);
    pthread_cond_init(&sfh->not_full, NULL);

    if (pthread_create(&sfh->thread, NULL,
                       lmk_sync_file_log_handler_thread_routine, sfh) != 0) {
        fprintf(stderr, "logmoko: unable to create sync file handler thread\n");
        sfh->running = 0;
        pthread_cond_destroy(&sfh->not_full);
        pthread_cond_destroy(&sfh->not_empty);
        pthread_mutex_destroy(&sfh->queue_lock);
        lmk_free(sfh->buffer);
        sfh->buffer = NULL;
        fclose(sfh->log_fp);
        sfh->log_fp = NULL;
        pthread_mutex_unlock(&handler->lock);
        return;
    }

    handler->initialized = 1;
    pthread_mutex_unlock(&handler->lock);
}

void lmk_sync_file_log_handler_destroy(struct lmk_log_handler *handler, void *param) {
    struct lmk_sync_file_log_handler *sfh = (struct lmk_sync_file_log_handler *)handler;

    pthread_mutex_lock(&sfh->queue_lock);
    sfh->running = 0;
    pthread_cond_broadcast(&sfh->not_empty);
    pthread_cond_broadcast(&sfh->not_full);
    pthread_mutex_unlock(&sfh->queue_lock);

    pthread_join(sfh->thread, NULL);
    pthread_cond_destroy(&sfh->not_full);
    pthread_cond_destroy(&sfh->not_empty);
    pthread_mutex_destroy(&sfh->queue_lock);
    lmk_free(sfh->buffer);
    sfh->buffer = NULL;

    pthread_mutex_lock(&handler->lock);
    if (sfh->log_fp) { fclose(sfh->log_fp); sfh->log_fp = NULL; }
    lmk_free((void *)sfh->filename);
    sfh->filename = NULL;
    pthread_mutex_unlock(&handler->lock);
}

void lmk_sync_file_log_handler_log_impl(struct lmk_log_handler *handler, void *param) {
    struct lmk_sync_file_log_handler *sfh = (struct lmk_sync_file_log_handler *)handler;
    struct lmk_log_record *rec = (struct lmk_log_record *)param;
    size_t n = rec->data_len < LMK_LOG_MSG_MAX_SIZE - 1
               ? rec->data_len : LMK_LOG_MSG_MAX_SIZE - 1;

    pthread_mutex_lock(&sfh->queue_lock);
    while (sfh->count == sfh->buffer_size && sfh->running)
        pthread_cond_wait(&sfh->not_full, &sfh->queue_lock);

    if (!sfh->running) {
        pthread_mutex_unlock(&sfh->queue_lock);
        return;
    }

    struct lmk_sync_file_log_slot *slot = &sfh->buffer[sfh->head];
    slot->log_level    = rec->log_level;
    slot->line_no      = rec->line_no;
    slot->file_name    = rec->file_name;
    slot->handler_name = (char *)handler->name;
    slot->data_len     = n;
    memcpy(slot->data, rec->data, n);
    slot->data[n] = '\0';
    sfh->head = (sfh->head + 1) % sfh->buffer_size;
    sfh->count++;
    pthread_cond_signal(&sfh->not_empty);
    pthread_mutex_unlock(&sfh->queue_lock);

    atomic_fetch_add_explicit(&handler->nr_log_calls, 1, memory_order_relaxed);
}
