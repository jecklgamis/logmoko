#include "logmoko.h"

static void *lmk_file_log_handler_thread_routine(void *arg) {
    struct lmk_file_log_handler *flh = (struct lmk_file_log_handler *) arg;
    while (flh->running || flh->count > 0) {
        struct lmk_log_request *req = NULL;
        LMK_LOCK_MUTEX(flh->base.lock);
        while (flh->count == 0 && flh->running) {
            LMK_WAIT_COND(flh->cond, flh->base.lock);
        }
        if (flh->count > 0) {
            req = &flh->ring_buffer[flh->tail];
            if (flh->log_fp != NULL) {
                const char *level_str = lmk_get_log_level_str(req->log_level);
                char ts_buff[LMK_TSTAMP_BUFF_SIZE];
                lmk_get_timestamp(ts_buff, LMK_TSTAMP_BUFF_SIZE);
                fprintf(flh->log_fp, "[%-5s %s (%s:%d) %s] : %s\n", level_str, ts_buff,
                        req->file_name, req->line_no, req->handler_name,
                        req->data);
                fflush(flh->log_fp);
            }
            lmk_free(req->data);
            lmk_free(req->file_name);
            lmk_free(req->handler_name);
            flh->tail = (flh->tail + 1) % LMK_RING_BUFFER_SIZE;
            flh->count--;
            LMK_SIGNAL_COND(flh->space_cond);
        }
        LMK_UNLOCK_MUTEX(flh->base.lock);
    }
    return NULL;
}

void lmk_file_log_handler_init(struct lmk_log_handler *handler, void *param) {
    struct lmk_file_log_handler *flh = (struct lmk_file_log_handler *) handler;
    LMK_LOCK_MUTEX(handler->lock);
    if (flh->log_fp != NULL) {
        fclose(flh->log_fp);
        flh->log_fp = NULL;
    }
    if ((flh->log_fp = fopen(flh->filename, "wt")) == NULL) {
        fprintf(stderr, "Unable to open file for writing : %s (%s:%d)\n",
                flh->filename, __FILE__, __LINE__);
    }
    flh->head = 0;
    flh->tail = 0;
    flh->count = 0;
    flh->running = 1;
    LMK_INIT_COND(flh->cond);
    LMK_INIT_COND(flh->space_cond);
    pthread_create(&flh->thread, NULL, lmk_file_log_handler_thread_routine, flh);
    LMK_UNLOCK_MUTEX(handler->lock);
}

void lmk_file_log_handler_log_impl(struct lmk_log_handler *handler, void *param) {
    struct lmk_file_log_handler *flh = (struct lmk_file_log_handler *) handler;
    struct lmk_log_record *log_rec = (struct lmk_log_record *) param;

    LMK_LOCK_MUTEX(handler->lock);
    while (flh->count == LMK_RING_BUFFER_SIZE && flh->running) {
        LMK_WAIT_COND(flh->space_cond, handler->lock);
    }
    if (flh->running) {
        struct lmk_log_request *req = &flh->ring_buffer[flh->head];
        req->log_level = log_rec->log_level;
        req->line_no = log_rec->line_no;
        req->data = lmk_strdup(log_rec->data);
        req->file_name = lmk_strdup(log_rec->file_name);
        req->handler_name = lmk_strdup(handler->name);
        flh->head = (flh->head + 1) % LMK_RING_BUFFER_SIZE;
        flh->count++;
        handler->nr_log_calls++;
        LMK_SIGNAL_COND(flh->cond);
    }
    LMK_UNLOCK_MUTEX(handler->lock);
}

void lmk_file_log_handler_destroy(struct lmk_log_handler *handler, void *param) {
    struct lmk_file_log_handler *flh = (struct lmk_file_log_handler *) handler;
    LMK_LOCK_MUTEX(handler->lock);
    flh->running = 0;
    LMK_SIGNAL_COND(flh->cond);
    LMK_UNLOCK_MUTEX(handler->lock);
    pthread_join(flh->thread, NULL);
    LMK_DESTROY_COND(flh->cond);
    LMK_DESTROY_COND(flh->space_cond);

    LMK_LOCK_MUTEX(handler->lock);
    if (flh->log_fp != NULL) {
        fclose(flh->log_fp);
        flh->log_fp = NULL;
    }
    LMK_UNLOCK_MUTEX(handler->lock);
}

LMK_API void lmk_set_log_filename(struct lmk_log_handler *handler, const char *filename) {
    LMK_LOCK_MUTEX(handler->lock);
    if (handler != NULL && filename != NULL) {
        if (handler->type == LMK_LOG_HANDLER_TYPE_FILE) {
            struct lmk_file_log_handler *flh = (struct lmk_file_log_handler *) handler;
            flh->filename = filename;
        }
    }
    LMK_UNLOCK_MUTEX(handler->lock);
}
