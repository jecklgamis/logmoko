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

/** @brief Socket address */
typedef struct lmk_inet_socket_address {
    struct sockaddr_in addr;
} lmk_inet_socket_address;

/** @brief Socket address */
typedef struct lmk_inet_address {
    char ipv4_addr[32];
} lmk_inet_address;

/** @brief List of server listeners */
typedef struct lmk_log_server {
    lmk_list link;
    lmk_inet_socket_address socket_addr;
} lmk_log_server;

/** @brief A memory buffer */
typedef struct lmk_buffer {
    unsigned char *addr;
    size_t size;
} lmk_buffer;

/** @brief Datagram packet */
typedef struct lmk_datagram_packet {
    lmk_inet_socket_address *socket_addr;
    lmk_buffer *buffer;
} lmk_udp_packet;

/** @brief UDP socket */
typedef struct lmk_udp_socket {
    int sockd;
    lmk_inet_address local_addr;
} lmk_udp_socket;

#define LMK_UDP_SOCKET_DEFAULT_SEND_BUFFER_SIZE 2048

LMK_API int lmk_open_udp_socket(lmk_udp_socket *socket, lmk_inet_address *local_addr);
LMK_API void lmk_close_udp_socket(lmk_udp_socket *socket);
LMK_API int lmk_send_udp_packet(lmk_udp_socket *socket, lmk_udp_packet *packet);

#endif
