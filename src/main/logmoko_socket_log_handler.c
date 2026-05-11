#include "logmoko.h"

extern void lmk_format_log_line(struct lmk_log_handler *handler, char *out, size_t out_size,
                                 const struct lmk_log_request *req);

static void *lmk_socket_log_handler_thread_routine(void *arg) {
    struct lmk_socket_log_handler *slh = (struct lmk_socket_log_handler *) arg;
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

        size_t buf_size = lmk_get_config()->log_buffer_size;
        char out[buf_size];
        lmk_format_log_line(&slh->base, out, buf_size, slot);
        struct lmk_list *cursor;
        LMK_FOR_EACH_ENTRY(&slh->log_server_list, cursor) {
            struct lmk_log_server *log_server = (struct lmk_log_server *) cursor;
            struct lmk_buffer buffer;
            buffer.addr = (unsigned char *) out;
            buffer.size = strlen(out);
            struct lmk_udp_packet packet;
            packet.buffer = &buffer;
            packet.socket_addr = &log_server->socket_addr;
            lmk_send_udp_packet(&slh->socket_object, &packet);
        }

        pthread_mutex_lock(&slh->base.lock);
        slh->tail = (slh->tail + 1) % ring_buf_size;
        slh->count--;
        pthread_mutex_unlock(&slh->base.lock);
    }
    return NULL;
}

void lmk_socket_log_handler_init(struct lmk_log_handler *handler, void *param) {
    struct lmk_socket_log_handler *slh = (struct lmk_socket_log_handler *) handler;
    pthread_mutex_lock(&handler->lock);
    lmk_init_list(&slh->log_server_list);
    lmk_open_udp_socket(&slh->socket_object, NULL);
    slh->head = 0;
    slh->tail = 0;
    slh->count = 0;
    slh->running = 1;
    slh->ring_buffer = lmk_malloc(sizeof(struct lmk_log_request) * lmk_get_config()->ring_buffer_size);
    if (!slh->ring_buffer) {
        fprintf(stderr, "Unable to allocate ring buffer");
        pthread_mutex_unlock(&handler->lock);
        return;
    }
    pthread_cond_init(&slh->cond, NULL);
    pthread_create(&slh->thread, NULL, lmk_socket_log_handler_thread_routine, slh);
    pthread_mutex_unlock(&handler->lock);
}

void lmk_socket_log_handler_log_impl(struct lmk_log_handler *handler, void *param) {
    struct lmk_socket_log_handler *slh = (struct lmk_socket_log_handler *) handler;
    struct lmk_log_record *log_rec = (struct lmk_log_record *) param;

    pthread_mutex_lock(&handler->lock);
    if (slh->count >= lmk_get_config()->ring_buffer_size) {
        pthread_mutex_unlock(&handler->lock);
        return;
    }
    if (slh->running) {
        struct lmk_log_request *req = &slh->ring_buffer[slh->head];
        req->log_level = log_rec->log_level;
        req->line_no = log_rec->line_no;
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

void lmk_socket_log_handler_destroy(struct lmk_log_handler *handler, void *param) {
    struct lmk_socket_log_handler *slh = (struct lmk_socket_log_handler *) handler;
    struct lmk_list *cursor;
    pthread_mutex_lock(&handler->lock);
    slh->running = 0;
    pthread_cond_broadcast(&slh->cond);
    pthread_mutex_unlock(&handler->lock);
    pthread_join(slh->thread, NULL);
    pthread_cond_destroy(&slh->cond);

    pthread_mutex_lock(&handler->lock);
    LMK_FOR_EACH_ENTRY(&slh->log_server_list, cursor) {
        struct lmk_log_server *log_server = (struct lmk_log_server *) cursor;
        LMK_SAVE_CURSOR(cursor);
            lmk_remove_list(&log_server->link);
            lmk_free(log_server);
        LMK_RESTORE_CURSOR(cursor);
    }
    lmk_close_udp_socket(&slh->socket_object);
    pthread_mutex_unlock(&handler->lock);
}

LMK_API void lmk_attach_log_listener(struct lmk_log_handler *handler, const char *host,
                                     int port) {
    pthread_mutex_lock(&handler->lock);
    if (handler != NULL && host != NULL && port > 0) {
        if (handler->type == LMK_LOG_HANDLER_TYPE_SOCKET) {
            struct lmk_socket_log_handler *slh = (struct lmk_socket_log_handler *) handler;
            struct lmk_log_server *log_server = NULL;
            if ((log_server = (struct lmk_log_server *) lmk_malloc(
                    sizeof(struct lmk_log_server))) != NULL) {
                memset(log_server, 0, sizeof(struct lmk_log_server));
                lmk_init_list(&log_server->link);
                log_server->socket_addr.addr.sin_family = PF_INET;
                log_server->socket_addr.addr.sin_port = htons(port);
                inet_pton(PF_INET, host, &log_server->socket_addr.addr.sin_addr);
                lmk_insert_list(&slh->log_server_list, &log_server->link);
            }
        }
    }
    pthread_mutex_unlock(&handler->lock);
}
