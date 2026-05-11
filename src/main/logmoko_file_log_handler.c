#include "logmoko.h"

extern void lmk_format_log_line(struct lmk_log_handler *handler, char *out, size_t out_size,
                                 const struct lmk_log_request *req);

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
        fprintf(stderr, "logmoko: unable to open log file after rotation: %s\n", flh->filename);
}

static void *lmk_file_log_handler_thread_routine(void *arg) {
    struct lmk_file_log_handler *flh = (struct lmk_file_log_handler *) arg;
    int ring_buf_size = lmk_get_config()->ring_buffer_size;

    while (flh->running || flh->count > 0) {
        struct lmk_log_request req = {0};
        int got = 0;

        pthread_mutex_lock(&flh->base.lock);
        while (flh->count == 0 && flh->running)
            pthread_cond_wait(&flh->cond, &flh->base.lock);
        if (flh->count > 0) {
            req = flh->ring_buffer[flh->tail];
            flh->ring_buffer[flh->tail].data = NULL;
            flh->ring_buffer[flh->tail].file_name = NULL;
            flh->ring_buffer[flh->tail].handler_name = NULL;
            flh->tail = (flh->tail + 1) % ring_buf_size;
            flh->count--;
            got = 1;
        }
        pthread_mutex_unlock(&flh->base.lock);

        if (got && flh->log_fp != NULL) {
            size_t buf_size = lmk_get_config()->log_buffer_size;
            char out[buf_size];
            lmk_format_log_line(&flh->base, out, buf_size, &req);
            size_t len = strlen(out);
            fputs(out, flh->log_fp);
            fflush(flh->log_fp);
            flh->current_file_size += (long) len;
            if (flh->max_file_size > 0 && flh->current_file_size >= flh->max_file_size)
                lmk_rotate_log_file(flh);
            lmk_free(req.data);
            lmk_free(req.file_name);
            lmk_free(req.handler_name);
        }
    }
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
    if (flh->log_fp) {
        fseek(flh->log_fp, 0, SEEK_END);
        flh->current_file_size = ftell(flh->log_fp);
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
    if (flh->count >= lmk_get_config()->ring_buffer_size) {
        fprintf(stderr, "logmoko: file handler '%s' buffer full, discarding log\n", handler->name);
        pthread_mutex_unlock(&handler->lock);
        return;
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
    lmk_free((void *) flh->filename);
    flh->filename = NULL;
    pthread_mutex_unlock(&handler->lock);
}

LMK_API void lmk_set_log_rotation(struct lmk_log_handler *handler, long max_file_size, int max_backup_files) {
    if (handler && handler->type == LMK_LOG_HANDLER_TYPE_FILE) {
        struct lmk_file_log_handler *flh = (struct lmk_file_log_handler *) handler;
        pthread_mutex_lock(&handler->lock);
        flh->max_file_size = max_file_size;
        flh->max_backup_files = max_backup_files;
        pthread_mutex_unlock(&handler->lock);
    }
}

LMK_API void lmk_set_log_filename(struct lmk_log_handler *handler, const char *filename) {
    pthread_mutex_lock(&handler->lock);
    if (handler != NULL && filename != NULL) {
        if (handler->type == LMK_LOG_HANDLER_TYPE_FILE) {
            struct lmk_file_log_handler *flh = (struct lmk_file_log_handler *) handler;
            lmk_free((void *) flh->filename);
            flh->filename = lmk_strdup(filename);
        }
    }
    pthread_mutex_unlock(&handler->lock);
}
