#include "logmoko.h"

static void *lmk_console_log_handler_thread_routine(void *arg) {
    struct lmk_console_log_handler *clh = (struct lmk_console_log_handler *) arg;
    while (clh->running || clh->count > 0) {
        struct lmk_log_request *req = NULL;
        LMK_LOCK_MUTEX(clh->base.lock);
        while (clh->count == 0 && clh->running) {
            LMK_WAIT_COND(clh->cond, clh->base.lock);
        }
        if (clh->count > 0) {
            req = &clh->ring_buffer[clh->tail];
            const char *level_str = lmk_get_log_level_str(req->log_level);
            char timestamp[LMK_TSTAMP_BUFF_SIZE];
            lmk_get_timestamp(timestamp, LMK_TSTAMP_BUFF_SIZE);
            fprintf(stdout, "[%s %s (%s:%d)] : %s\n", level_str, timestamp, req->file_name, req->line_no,
                    req->data);
            fflush(stdout);
            lmk_free(req->data);
            lmk_free(req->file_name);
            lmk_free(req->handler_name);
            clh->tail = (clh->tail + 1) % g_lmk_config->ring_buffer_size;
            clh->count--;
        }
        LMK_UNLOCK_MUTEX(clh->base.lock);
    }
    return NULL;
}

void lmk_console_log_handler_init(struct lmk_log_handler *handler, void *param) {
    struct lmk_console_log_handler *clh = (struct lmk_console_log_handler *) handler;
    clh->ring_buffer = lmk_malloc(sizeof(struct lmk_log_request) * g_lmk_config->ring_buffer_size);
    if (!clh->ring_buffer) {
        fprintf(stderr, "Unable to allocate ring buffer");
        return;
    }
    clh->head = 0;
    clh->tail = 0;
    clh->count = 0;
    clh->running = 1;
    LMK_INIT_COND(clh->cond);
    pthread_create(&clh->thread, NULL, lmk_console_log_handler_thread_routine, clh);
}

void lmk_console_log_handler_destroy(struct lmk_log_handler *handler, void *param) {
    struct lmk_console_log_handler *clh = (struct lmk_console_log_handler *) handler;
    LMK_LOCK_MUTEX(clh->base.lock);
    clh->running = 0;
    LMK_SIGNAL_COND(clh->cond);
    LMK_UNLOCK_MUTEX(clh->base.lock);
    pthread_join(clh->thread, NULL);
    LMK_DESTROY_COND(clh->cond);
}

void lmk_console_log_handler_log_impl(struct lmk_log_handler *handler, void *param) {
    struct lmk_console_log_handler *clh = (struct lmk_console_log_handler *) handler;
    struct lmk_log_record *log_rec = (struct lmk_log_record *) param;

    LMK_LOCK_MUTEX(handler->lock);
    if (clh->count < g_lmk_config->ring_buffer_size && clh->running) {
        struct lmk_log_request *req = &clh->ring_buffer[clh->head];
        req->log_level = log_rec->log_level;
        req->line_no = log_rec->line_no;
        req->data = lmk_strdup(log_rec->data);
        req->file_name = lmk_strdup(log_rec->file_name);
        req->handler_name = lmk_strdup(handler->name);
        clh->head = (clh->head + 1) % g_lmk_config->ring_buffer_size;
        clh->count++;
        handler->nr_log_calls++;
        LMK_SIGNAL_COND(clh->cond);
    } else {
#if LMK_DEBUG
        fprintf(stderr, "WARNING: Console ring buffer full, dropping log\n");
#endif
    }
    LMK_UNLOCK_MUTEX(handler->lock);
}
