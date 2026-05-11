#include "logmoko.h"

extern void lmk_format_log_line(struct lmk_log_handler *handler, char *out, size_t out_size,
                                 const struct lmk_log_request *req);

static void *lmk_console_log_handler_thread_routine(void *arg) {
    struct lmk_console_log_handler *clh = (struct lmk_console_log_handler *) arg;
    int ring_buf_size = lmk_get_config()->ring_buffer_size;
    struct lmk_log_request *batch = lmk_malloc(ring_buf_size * sizeof(struct lmk_log_request));
    if (!batch) return NULL;

    while (clh->running || clh->count > 0) {
        int batch_count = 0;

        pthread_mutex_lock(&clh->base.lock);
        while (clh->count == 0 && clh->running) {
            pthread_cond_wait(&clh->cond, &clh->base.lock);
        }
        while (clh->count > 0) {
            struct lmk_log_request *req = &clh->ring_buffer[clh->tail];
            batch[batch_count++] = *req;
            req->data = NULL;
            req->file_name = NULL;
            req->handler_name = NULL;
            clh->tail = (clh->tail + 1) % ring_buf_size;
            clh->count--;
        }
        pthread_cond_broadcast(&clh->cond);
        pthread_mutex_unlock(&clh->base.lock);

        if (batch_count > 0) {
            size_t buf_size = lmk_get_config()->log_buffer_size;
            char out[buf_size];
            for (int i = 0; i < batch_count; i++) {
                lmk_format_log_line(&clh->base, out, buf_size, &batch[i]);
                fputs(out, stdout);
                lmk_free(batch[i].data);
                lmk_free(batch[i].file_name);
                lmk_free(batch[i].handler_name);
            }
            fflush(stdout);
        }
    }
    lmk_free(batch);
    return NULL;
}

void lmk_console_log_handler_init(struct lmk_log_handler *handler, void *param) {
    struct lmk_console_log_handler *clh = (struct lmk_console_log_handler *) handler;
    clh->ring_buffer = lmk_malloc(sizeof(struct lmk_log_request) * lmk_get_config()->ring_buffer_size);
    if (!clh->ring_buffer) {
        fprintf(stderr, "Unable to allocate ring buffer");
        return;
    }
    clh->head = 0;
    clh->tail = 0;
    clh->count = 0;
    clh->running = 1;
    pthread_cond_init(&clh->cond, NULL);
    pthread_create(&clh->thread, NULL, lmk_console_log_handler_thread_routine, clh);
}

void lmk_console_log_handler_destroy(struct lmk_log_handler *handler, void *param) {
    struct lmk_console_log_handler *clh = (struct lmk_console_log_handler *) handler;
    pthread_mutex_lock(&clh->base.lock);
    clh->running = 0;
    pthread_cond_broadcast(&clh->cond);
    pthread_mutex_unlock(&clh->base.lock);
    pthread_join(clh->thread, NULL);
    pthread_cond_destroy(&clh->cond);
}

void lmk_console_log_handler_log_impl(struct lmk_log_handler *handler, void *param) {
    struct lmk_console_log_handler *clh = (struct lmk_console_log_handler *) handler;
    struct lmk_log_record *log_rec = (struct lmk_log_record *) param;

    pthread_mutex_lock(&handler->lock);
    while (clh->count >= lmk_get_config()->ring_buffer_size && clh->running) {
        pthread_cond_wait(&clh->cond, &handler->lock);
    }
    if (clh->running) {
        struct lmk_log_request *req = &clh->ring_buffer[clh->head];
        req->log_level = log_rec->log_level;
        req->line_no = log_rec->line_no;
        req->data = lmk_strdup(log_rec->data);
        req->file_name = lmk_strdup(log_rec->file_name);
        req->handler_name = lmk_strdup(handler->name);
        clh->head = (clh->head + 1) % lmk_get_config()->ring_buffer_size;
        clh->count++;
        handler->nr_log_calls++;
        pthread_cond_signal(&clh->cond);
    }
    pthread_mutex_unlock(&handler->lock);
}
