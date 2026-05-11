#include "logmoko.h"

extern void lmk_format_log_line(struct lmk_log_handler *handler, char *out, size_t out_size,
                                 const struct lmk_log_request *req);

static void *lmk_file_log_handler_thread_routine(void *arg) {
    struct lmk_file_log_handler *flh = (struct lmk_file_log_handler *) arg;
    int ring_buf_size = lmk_get_config()->ring_buffer_size;
    struct lmk_log_request *batch = lmk_malloc(ring_buf_size * sizeof(struct lmk_log_request));
    if (!batch) return NULL;

    while (flh->running || flh->count > 0) {
        int batch_count = 0;

        pthread_mutex_lock(&flh->base.lock);
        while (flh->count == 0 && flh->running) {
            pthread_cond_wait(&flh->cond, &flh->base.lock);
        }
        while (flh->count > 0) {
            struct lmk_log_request *req = &flh->ring_buffer[flh->tail];
            batch[batch_count++] = *req;
            req->data = NULL;
            req->file_name = NULL;
            req->handler_name = NULL;
            flh->tail = (flh->tail + 1) % ring_buf_size;
            flh->count--;
        }
        pthread_cond_broadcast(&flh->cond);
        pthread_mutex_unlock(&flh->base.lock);

        if (batch_count > 0 && flh->log_fp != NULL) {
            size_t buf_size = lmk_get_config()->log_buffer_size;
            char out[buf_size];
            for (int i = 0; i < batch_count; i++) {
                lmk_format_log_line(&flh->base, out, buf_size, &batch[i]);
                fputs(out, flh->log_fp);
                lmk_free(batch[i].data);
                lmk_free(batch[i].file_name);
                lmk_free(batch[i].handler_name);
            }
            fflush(flh->log_fp);
        }
    }
    lmk_free(batch);
    return NULL;
}

void lmk_file_log_handler_init(struct lmk_log_handler *handler, void *param) {
    struct lmk_file_log_handler *flh = (struct lmk_file_log_handler *) handler;
    pthread_mutex_lock(&handler->lock);
    if (flh->log_fp != NULL) {
        fclose(flh->log_fp);
        flh->log_fp = NULL;
    }
    if ((flh->log_fp = fopen(flh->filename, "at")) == NULL) {
        fprintf(stderr, "Unable to open file for writing : %s (%s:%d)\n",
                flh->filename, __FILE__, __LINE__);
    }
    flh->head = 0;
    flh->tail = 0;
    flh->count = 0;
    flh->running = 1;
    flh->ring_buffer = lmk_malloc(sizeof(struct lmk_log_request) * lmk_get_config()->ring_buffer_size);
    if (!flh->ring_buffer) {
        fprintf(stderr,"Unable to allocate ring buffer\n");
        pthread_mutex_unlock(&handler->lock);
        return;
    }
    pthread_cond_init(&flh->cond, NULL);
    pthread_create(&flh->thread, NULL, lmk_file_log_handler_thread_routine, flh);
    pthread_mutex_unlock(&handler->lock);
}

void lmk_file_log_handler_log_impl(struct lmk_log_handler *handler, void *param) {
    struct lmk_file_log_handler *flh = (struct lmk_file_log_handler *) handler;
    struct lmk_log_record *log_rec = (struct lmk_log_record *) param;

    pthread_mutex_lock(&handler->lock);
    while (flh->count >= lmk_get_config()->ring_buffer_size && flh->running) {
        pthread_cond_wait(&flh->cond, &handler->lock);
    }
    if (flh->running) {
        struct lmk_log_request *req = &flh->ring_buffer[flh->head];
        req->log_level = log_rec->log_level;
        req->line_no = log_rec->line_no;
        req->data = lmk_strdup(log_rec->data);
        req->file_name = lmk_strdup(log_rec->file_name);
        req->handler_name = lmk_strdup(handler->name);
        flh->head = (flh->head + 1) % lmk_get_config()->ring_buffer_size;
        flh->count++;
        handler->nr_log_calls++;
        pthread_cond_signal(&flh->cond);
    }
    pthread_mutex_unlock(&handler->lock);
}

void lmk_file_log_handler_destroy(struct lmk_log_handler *handler, void *param) {
    struct lmk_file_log_handler *flh = (struct lmk_file_log_handler *) handler;
    pthread_mutex_lock(&handler->lock);
    flh->running = 0;
    pthread_cond_broadcast(&flh->cond);
    pthread_mutex_unlock(&handler->lock);
    pthread_join(flh->thread, NULL);
    pthread_cond_destroy(&flh->cond);

    pthread_mutex_lock(&handler->lock);
    if (flh->log_fp != NULL) {
        fclose(flh->log_fp);
        flh->log_fp = NULL;
    }
    pthread_mutex_unlock(&handler->lock);
}

LMK_API void lmk_set_log_filename(struct lmk_log_handler *handler, const char *filename) {
    pthread_mutex_lock(&handler->lock);
    if (handler != NULL && filename != NULL) {
        if (handler->type == LMK_LOG_HANDLER_TYPE_FILE) {
            struct lmk_file_log_handler *flh = (struct lmk_file_log_handler *) handler;
            flh->filename = filename;
        }
    }
    pthread_mutex_unlock(&handler->lock);
}
