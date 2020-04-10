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

#ifndef LOGMOKO_CONNECTOR_H
#define LOGMOKO_CONNECTOR_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <logmoko_types.h>
#include <arpa/inet.h>

#include "logmoko_list.h"

/* Socket address */
typedef struct lmk_inet_socket_address {
    struct sockaddr_in addr;
} lmk_inet_socket_address;

/* Socket address */
typedef struct lmk_inet_address {
    char ipv4_addr[32];
} lmk_inet_address;

/* List of server listeners */
typedef struct lmk_log_server {
    lmk_list link;
    lmk_inet_socket_address socket_addr;
} lmk_log_server;

/* A memory buffer */
typedef struct lmk_buffer {
    unsigned char *addr;
    size_t size;
} lmk_buffer;

/* Datagram packet */
typedef struct lmk_datagram_packet {
    lmk_inet_socket_address *socket_addr;
    lmk_buffer *buffer;
} lmk_udp_packet;

/* UDP socket */
typedef struct lmk_udp_socket {
    int sockd;
    lmk_inet_address local_addr;
} lmk_udp_socket;

#define LMK_UDP_SOCKET_DEFAULT_SEND_BUFFER_SIZE 2048

LMK_API int lmk_open_udp_socket(lmk_udp_socket *socket, lmk_inet_address *local_addr);

LMK_API void lmk_close_udp_socket(lmk_udp_socket *socket);

LMK_API int lmk_send_udp_packet(lmk_udp_socket *socket, lmk_udp_packet *packet);

#endif
