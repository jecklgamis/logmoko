/*
 * The MIT License (MIT)
 *
 * Logmoko - A logging framework for C
 * Copyright 2013 Jerrico Gamis <jecklgamis@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "logmoko.h"

#define LMK_DEBUG_SOCKET_LOG_HANDLER 1

void lmk_socket_log_handler_init(lmk_log_handler *handler, void *param) {
    lmk_socket_log_handler *slh = NULL;
    char ts_buff[LMK_TSTAMP_BUFF_SIZE];
    LMK_LOCK_MUTEX(handler->lock);
    slh = (lmk_socket_log_handler *) handler;
    lmk_init_list(&slh->log_server_list);
    lmk_open_udp_socket(&slh->socket_object, NULL);
    LMK_UNLOCK_MUTEX(handler->lock);
}

void lmk_socket_log_handler_log_impl(lmk_log_handler *handler, void *param) {
    lmk_log_record *log_rec = NULL;;
    lmk_socket_log_handler *slh = NULL;
    lmk_list *cursor;
    char ts_buff[LMK_TSTAMP_BUFF_SIZE];
    char output_buff[LMK_LOG_BUFFER_SIZE];

    LMK_LOCK_MUTEX(handler->lock);
    slh = (lmk_socket_log_handler *) handler;
    log_rec = (lmk_log_record *) param;
    const char *level_str = lmk_get_log_level_str(log_rec->log_level);

    memset(output_buff, 0, LMK_LOG_BUFFER_SIZE);

    LMK_FOR_EACH_ENTRY(&slh->log_server_list, cursor) {
        lmk_log_server *log_server = (lmk_log_server *) cursor;
        lmk_buffer buffer;
        lmk_get_timestamp(ts_buff, LMK_TSTAMP_BUFF_SIZE);
        memset(output_buff, 0, LMK_LOG_BUFFER_SIZE);
        sprintf(output_buff, "[%-5s %s (%s:%d) %s] : %s\n", level_str, ts_buff,
                log_rec->file_name, log_rec->line_no, handler->name,
                log_rec->data);
        buffer.addr = (unsigned char *) output_buff;
        buffer.size = strlen(output_buff);
#ifdef LMK_DEBUG_SOCKET_LOG_HANDLER
        fprintf(stdout, "Writing %lu bytes to socket\n", buffer.size);
#endif
        lmk_udp_packet packet;
        packet.buffer = &buffer;
        packet.socket_addr = &log_server->socket_addr;
        lmk_send_udp_packet(&slh->socket_object, &packet);
    }
    LMK_UNLOCK_MUTEX(handler->lock);
}

void lmk_socket_log_handler_destroy(lmk_log_handler *handler, void *param) {
    lmk_socket_log_handler *slh = NULL;
    lmk_list *cursor;
    LMK_LOCK_MUTEX(handler->lock);
    slh = (lmk_socket_log_handler *) handler;

    LMK_FOR_EACH_ENTRY(&slh->log_server_list, cursor) {
        lmk_log_server *log_server = (lmk_log_server *) cursor;
        LMK_SAVE_CURSOR(cursor);
            lmk_remove_list(&log_server->link);
            lmk_free(log_server);
        LMK_RESTORE_CURSOR(cursor);
    }
    lmk_close_udp_socket(&slh->socket_object);
    LMK_UNLOCK_MUTEX(handler->lock);
}

LMK_API void lmk_attach_log_listener(lmk_log_handler *handler, const char *host,
                                     int port) {
    LMK_LOCK_MUTEX(handler->lock);
    if (handler != NULL && host != NULL && port > 0) {
        if (handler->type == LMK_LOG_HANDLER_TYPE_SOCKET) {
            lmk_socket_log_handler *slh = (lmk_socket_log_handler *) handler;
            lmk_log_server *log_server = NULL;
            if ((log_server = (lmk_log_server *) lmk_malloc(
                    sizeof(lmk_log_server))) != NULL) {
                memset(log_server, 0, sizeof(lmk_log_server));
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

