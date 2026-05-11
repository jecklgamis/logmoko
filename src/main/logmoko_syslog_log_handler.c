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

static void *lmk_syslog_log_handler_thread_routine(void *arg) {
    struct lmk_syslog_log_handler *slh = (struct lmk_syslog_log_handler *) arg;
    int ring_buf_size = lmk_get_config()->ring_buffer_size;

    while (1) {
        pthread_mutex_lock(&slh->base.lock);
        while (slh->count == 0 && slh->running)
            pthread_cond_wait(&slh->cond, &slh->base.lock);
        if (slh->count == 0) {
            pthread_mutex_unlock(&slh->base.lock);
            break;
        }
        struct lmk_log_request *slot = &slh->ring_buffer[slh->tail];
        pthread_mutex_unlock(&slh->base.lock);

        syslog(lmk_to_syslog_priority(slot->log_level),
               "[%s] (%s:%d) %s",
               lmk_get_log_level_str(slot->log_level),
               slot->file_name, slot->line_no, slot->data);

        pthread_mutex_lock(&slh->base.lock);
        slh->tail = (slh->tail + 1) % ring_buf_size;
        slh->count--;
        pthread_mutex_unlock(&slh->base.lock);
    }
    return NULL;
}

void lmk_syslog_log_handler_init(struct lmk_log_handler *handler, void *param) {
    struct lmk_syslog_log_handler *slh = (struct lmk_syslog_log_handler *) handler;
    openlog(slh->ident[0] ? slh->ident : NULL, LOG_PID, slh->facility);
    slh->ring_buffer = lmk_malloc(sizeof(struct lmk_log_request) * lmk_get_config()->ring_buffer_size);
    if (!slh->ring_buffer) {
        fprintf(stderr, "Unable to allocate ring buffer\n");
        return;
    }
    slh->head = 0;
    slh->tail = 0;
    slh->count = 0;
    slh->running = 1;
    pthread_cond_init(&slh->cond, NULL);
    pthread_create(&slh->thread, NULL, lmk_syslog_log_handler_thread_routine, slh);
}

void lmk_syslog_log_handler_destroy(struct lmk_log_handler *handler, void *param) {
    struct lmk_syslog_log_handler *slh = (struct lmk_syslog_log_handler *) handler;
    pthread_mutex_lock(&slh->base.lock);
    slh->running = 0;
    pthread_cond_broadcast(&slh->cond);
    pthread_mutex_unlock(&slh->base.lock);
    pthread_join(slh->thread, NULL);
    pthread_cond_destroy(&slh->cond);
    closelog();
}

void lmk_syslog_log_handler_log_impl(struct lmk_log_handler *handler, void *param) {
    struct lmk_syslog_log_handler *slh = (struct lmk_syslog_log_handler *) handler;
    struct lmk_log_record *log_rec = (struct lmk_log_record *) param;

    pthread_mutex_lock(&handler->lock);
    if (slh->count >= lmk_get_config()->ring_buffer_size) {
        pthread_mutex_unlock(&handler->lock);
        return;
    }
    if (slh->running) {
        struct lmk_log_request *req = &slh->ring_buffer[slh->head];
        req->log_level = log_rec->log_level;
        req->line_no   = log_rec->line_no;
        strncpy(req->data, log_rec->data, LMK_LOG_MSG_MAX_SIZE - 1);
        req->data[LMK_LOG_MSG_MAX_SIZE - 1] = '\0';
        req->file_name = log_rec->file_name;
        req->handler_name = handler->name;
        slh->head = (slh->head + 1) % lmk_get_config()->ring_buffer_size;
        slh->count++;
        handler->nr_log_calls++;
        pthread_cond_signal(&slh->cond);
    }
    pthread_mutex_unlock(&handler->lock);
}
