#include "logmoko.h"

static void *lmk_socket_log_handler_thread_routine(void *arg) {
    struct lmk_socket_log_handler *slh = (struct lmk_socket_log_handler *) arg;
    char output_buff[g_lmk_config->log_buffer_size];

    while (slh->running || slh->count > 0) {
        struct lmk_log_request *req = NULL;
        LMK_LOCK_MUTEX(slh->base.lock);
        while (slh->count == 0 && slh->running) {
            LMK_WAIT_COND(slh->cond, slh->base.lock);
        }
        if (slh->count > 0) {
            req = &slh->ring_buffer[slh->tail];
            const char *level_str = lmk_get_log_level_str(req->log_level);
            char ts_buff[LMK_TSTAMP_BUFF_SIZE];
            struct lmk_list *cursor;

            LMK_FOR_EACH_ENTRY(&slh->log_server_list, cursor) {
                struct lmk_log_server *log_server = (struct lmk_log_server *) cursor;
                struct lmk_buffer buffer;
                lmk_get_timestamp(ts_buff, LMK_TSTAMP_BUFF_SIZE);
                memset(output_buff, 0, g_lmk_config->log_buffer_size);
                sprintf(output_buff, "[%s %s (%s:%d) %s] : %s\n", level_str, ts_buff,
                        req->file_name, req->line_no, req->handler_name,
                        req->data);
                buffer.addr = (unsigned char *) output_buff;
                buffer.size = strlen(output_buff);
                struct lmk_udp_packet packet;
                packet.buffer = &buffer;
                packet.socket_addr = &log_server->socket_addr;
                lmk_send_udp_packet(&slh->socket_object, &packet);
            }

            lmk_free(req->data);
            lmk_free(req->file_name);
            lmk_free(req->handler_name);
            slh->tail = (slh->tail + 1) % g_lmk_config->ring_buffer_size;
            slh->count--;
        }
        LMK_UNLOCK_MUTEX(slh->base.lock);
    }
    return NULL;
}

void lmk_socket_log_handler_init(struct lmk_log_handler *handler, void *param) {
    struct lmk_socket_log_handler *slh = (struct lmk_socket_log_handler *) handler;
    LMK_LOCK_MUTEX(handler->lock);
    lmk_init_list(&slh->log_server_list);
    lmk_open_udp_socket(&slh->socket_object, NULL);
    slh->head = 0;
    slh->tail = 0;
    slh->count = 0;
    slh->running = 1;
    slh->ring_buffer = lmk_malloc(sizeof(struct lmk_log_request) * g_lmk_config->ring_buffer_size);
    if (!slh->ring_buffer) {
        fprintf(stderr, "Unable to allocate ring buffer");
        LMK_UNLOCK_MUTEX(handler->lock);
        return;
    }
    LMK_INIT_COND(slh->cond);
    pthread_create(&slh->thread, NULL, lmk_socket_log_handler_thread_routine, slh);
    LMK_UNLOCK_MUTEX(handler->lock);
}

void lmk_socket_log_handler_log_impl(struct lmk_log_handler *handler, void *param) {
    struct lmk_socket_log_handler *slh = (struct lmk_socket_log_handler *) handler;
    struct lmk_log_record *log_rec = (struct lmk_log_record *) param;

    LMK_LOCK_MUTEX(handler->lock);
    if (slh->count < g_lmk_config->ring_buffer_size && slh->running) {
        struct lmk_log_request *req = &slh->ring_buffer[slh->head];
        req->log_level = log_rec->log_level;
        req->line_no = log_rec->line_no;
        req->data = lmk_strdup(log_rec->data);
        req->file_name = lmk_strdup(log_rec->file_name);
        req->handler_name = lmk_strdup(handler->name);
        slh->head = (slh->head + 1) % g_lmk_config->ring_buffer_size;
        slh->count++;
        handler->nr_log_calls++;
        LMK_SIGNAL_COND(slh->cond);
    } else {
#if LMK_DEBUG
        fprintf(stderr, "WARNING: Socket ring buffer full, dropping log\n");
#endif
    }
    LMK_UNLOCK_MUTEX(handler->lock);
}

void lmk_socket_log_handler_destroy(struct lmk_log_handler *handler, void *param) {
    struct lmk_socket_log_handler *slh = (struct lmk_socket_log_handler *) handler;
    struct lmk_list *cursor;
    LMK_LOCK_MUTEX(handler->lock);
    slh->running = 0;
    LMK_SIGNAL_COND(slh->cond);
    LMK_UNLOCK_MUTEX(handler->lock);
    pthread_join(slh->thread, NULL);
    LMK_DESTROY_COND(slh->cond);

    LMK_LOCK_MUTEX(handler->lock);
    LMK_FOR_EACH_ENTRY(&slh->log_server_list, cursor) {
        struct lmk_log_server *log_server = (struct lmk_log_server *) cursor;
        LMK_SAVE_CURSOR(cursor);
            lmk_remove_list(&log_server->link);
            lmk_free(log_server);
        LMK_RESTORE_CURSOR(cursor);
    }
    lmk_close_udp_socket(&slh->socket_object);
    LMK_UNLOCK_MUTEX(handler->lock);
}

LMK_API void lmk_attach_log_listener(struct lmk_log_handler *handler, const char *host,
                                     int port) {
    LMK_LOCK_MUTEX(handler->lock);
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
    LMK_UNLOCK_MUTEX(handler->lock);
}
