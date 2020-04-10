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

#include "logmoko_connector.h"

LMK_API int lmk_open_udp_socket(lmk_udp_socket *udp_socket, lmk_inet_address *local_addr) {
    int sockd;
    int send_size = LMK_UDP_SOCKET_DEFAULT_SEND_BUFFER_SIZE;

    if ((sockd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return LMK_E_NG;
    }
    setsockopt(sockd, SOL_SOCKET, SO_SNDBUF, (char *) &send_size, (int) sizeof(send_size));
    udp_socket->sockd = sockd;
    return LMK_E_OK;
}

LMK_API void lmk_close_udp_socket(lmk_udp_socket *udp_socket) {
    if (udp_socket != NULL) {
        close(udp_socket->sockd);
    }
}

LMK_API int lmk_send_udp_packet(lmk_udp_socket *udp_socket,
                                lmk_udp_packet *packet) {
    int nr_bytes = -1;

    if (udp_socket == NULL) {
        return LMK_E_NG;
    }
    if (packet == NULL || packet->buffer == NULL) {
        return LMK_E_NG;
    }
    nr_bytes = sendto(udp_socket->sockd, packet->buffer->addr,
                      packet->buffer->size, 0,
                      (struct sockaddr *) &packet->socket_addr->addr,
                      sizeof(packet->socket_addr->addr));

    if (nr_bytes < 0 || (nr_bytes != packet->buffer->size)) {
        return LMK_E_NG;
    }
    return LMK_E_OK;
}

