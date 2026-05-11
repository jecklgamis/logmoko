#include "logmoko.h"

extern void lmk_format_log_line(struct lmk_log_handler *handler, char *out, size_t out_size,
                                 const struct lmk_log_request *req);

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

static void *lmk_console_log_handler_thread_routine(void *arg) {
    struct lmk_console_log_handler *clh = (struct lmk_console_log_handler *) arg;
    int ring_buf_size = lmk_get_config()->ring_buffer_size;

    while (1) {
        pthread_mutex_lock(&clh->base.lock);
        while (clh->count == 0 && clh->running)
            pthread_cond_wait(&clh->cond, &clh->base.lock);
        if (clh->count == 0) {
            pthread_mutex_unlock(&clh->base.lock);
            break;
        }
        struct lmk_log_request *slot = &clh->ring_buffer[clh->tail];
        pthread_mutex_unlock(&clh->base.lock);

        size_t buf_size = lmk_get_config()->log_buffer_size;
        char out[buf_size];
        lmk_format_log_line(&clh->base, out, buf_size, slot);
        fputs(lmk_console_level_color(slot->log_level), stdout);
        fputs(out, stdout);
        fputs(ANSI_RESET, stdout);
        fflush(stdout);

        pthread_mutex_lock(&clh->base.lock);
        clh->tail = (clh->tail + 1) % ring_buf_size;
        clh->count--;
        pthread_mutex_unlock(&clh->base.lock);
    }
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
    if (clh->dropped)
        fprintf(stderr, "logmoko: console handler dropped %lu log(s), logged %lu\n",
                clh->dropped, handler->nr_log_calls);
    lmk_free(clh->ring_buffer);
    clh->ring_buffer = NULL;
}

void lmk_console_log_handler_log_impl(struct lmk_log_handler *handler, void *param) {
    struct lmk_console_log_handler *clh = (struct lmk_console_log_handler *) handler;
    struct lmk_log_record *log_rec = (struct lmk_log_record *) param;

    pthread_mutex_lock(&handler->lock);
    if (clh->count >= lmk_get_config()->ring_buffer_size) {
        clh->dropped++;
        pthread_mutex_unlock(&handler->lock);
        return;
    }
    if (clh->running) {
        struct lmk_log_request *req = &clh->ring_buffer[clh->head];
        req->log_level = log_rec->log_level;
        req->line_no = log_rec->line_no;
        strncpy(req->data, log_rec->data, LMK_LOG_MSG_MAX_SIZE - 1);
        req->data[LMK_LOG_MSG_MAX_SIZE - 1] = '\0';
        req->file_name = log_rec->file_name;
        req->handler_name = handler->name;
        clh->head = (clh->head + 1) % lmk_get_config()->ring_buffer_size;
        clh->count++;
        handler->nr_log_calls++;
        pthread_cond_signal(&clh->cond);
    }
    pthread_mutex_unlock(&handler->lock);
}
