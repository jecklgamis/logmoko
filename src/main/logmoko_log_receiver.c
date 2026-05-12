#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <openssl/evp.h>
#include "logmoko_encryption.h"

#define DEFAULT_PORT     9000
#define MAX_PACKET_SIZE  4096

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s -p <psk> [-b <host>] [-l <port>]\n", prog);
    fprintf(stderr, "  -p <psk>   Preshared key (required)\n");
    fprintf(stderr, "  -b <host>  Host to bind to (default: 0.0.0.0)\n");
    fprintf(stderr, "  -l <port>  UDP port to listen on (default: %d)\n", DEFAULT_PORT);
}

int main(int argc, char *argv[]) {
    const char *psk  = NULL;
    const char *host = "0.0.0.0";
    int port         = DEFAULT_PORT;

    int opt;
    while ((opt = getopt(argc, argv, "p:b:l:")) != -1) {
        switch (opt) {
            case 'p': psk  = optarg;       break;
            case 'b': host = optarg;       break;
            case 'l': port = atoi(optarg); break;
            default:  usage(argv[0]); return EXIT_FAILURE;
        }
    }
    if (!psk) {
        fprintf(stderr, "error: -p <psk> is required\n");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    fprintf(stderr, "Logmoko log receiver started\n");
    fprintf(stderr, "Listening on %s:%d\n", host, port);

    unsigned char key[CRYPTO_KEY_LEN];
    derive_key(psk, key);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(port),
    };
    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        fprintf(stderr, "error: invalid bind address '%s'\n", host);
        close(sockfd);
        return EXIT_FAILURE;
    }
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sockfd);
        return EXIT_FAILURE;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        fprintf(stderr, "EVP_CIPHER_CTX_new failed\n");
        close(sockfd);
        return EXIT_FAILURE;
    }

    unsigned char buf[MAX_PACKET_SIZE];
    unsigned char plain[MAX_PACKET_SIZE];

    while (1) {
        struct sockaddr_in peer;
        socklen_t peer_len = sizeof(peer);
        ssize_t n = recvfrom(sockfd, buf, sizeof(buf), 0,
                             (struct sockaddr *)&peer, &peer_len);
        if (n < 0) {
            perror("recvfrom");
            break;
        }

        int plain_len = 0;
        if (decrypt_packet(ctx, key, buf, (int)n, plain, &plain_len) != 0) {
            char peer_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &peer.sin_addr, peer_ip, sizeof(peer_ip));
            fprintf(stderr, "decrypt failed (bad psk or corrupt packet) from %s:%d\n",
                    peer_ip, ntohs(peer.sin_port));
            continue;
        }

        fwrite(plain, 1, plain_len, stdout);
        if (plain_len > 0 && plain[plain_len - 1] != '\n')
            fputc('\n', stdout);
        fflush(stdout);
    }

    EVP_CIPHER_CTX_free(ctx);
    close(sockfd);
    return EXIT_SUCCESS;
}
