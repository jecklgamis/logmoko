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

struct lmk_inet_socket_address {
    struct sockaddr_in addr;
};

struct lmk_inet_address {
    char ipv4_addr[32];
};

struct lmk_log_server {
    struct lmk_list link;
    struct lmk_inet_socket_address socket_addr;
};

struct lmk_buffer {
    unsigned char *addr;
    size_t size;
};

struct lmk_udp_packet {
    struct lmk_inet_socket_address *socket_addr;
    struct lmk_buffer *buffer;
};

struct lmk_udp_socket {
    int sockd;
    struct lmk_inet_address local_addr;
};

#define LMK_UDP_SOCKET_DEFAULT_SEND_BUFFER_SIZE 2048

LMK_API int lmk_open_udp_socket(struct lmk_udp_socket *socket, struct lmk_inet_address *local_addr);

LMK_API void lmk_close_udp_socket(struct lmk_udp_socket *socket);

LMK_API int lmk_send_udp_packet(struct lmk_udp_socket *socket, struct lmk_udp_packet *packet);

#endif
