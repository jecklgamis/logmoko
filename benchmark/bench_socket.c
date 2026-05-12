#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdatomic.h>

#include "../src/main/logmoko.h"
#include "../src/main/logmoko_encryption.h"
#include "../src/main/logmoko_rate_limiter.h"

#define BENCH_PORT      19876
#define RING_SZ         65536
#define NR_LOGS         50000
#define MAX_PKT         4096

#define MSG_SHORT "user=12345 action=login status=200 duration=42ms ip=192.168.1.1"
#define MSG_LONG  "user=12345 action=login status=200 duration=42ms ip=192.168.1.1 " \
                  "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"   \
                  "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"   \
                  "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"   \
                  "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"   \
                  "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"   \
                  "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"   \
                  "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"   \
                  "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"

/* ------------------------------------------------------------------ */
/* UDP receiver thread                                                  */
/* ------------------------------------------------------------------ */

struct receiver {
    pthread_t       thread;
    int             sockfd;
    _Atomic int     running;
    _Atomic long    nr_received;
    int             decrypt;
    unsigned char   key[CRYPTO_KEY_LEN];
};

static void *receiver_thread(void *arg) {
    struct receiver *r = (struct receiver *)arg;
    unsigned char buf[MAX_PKT];
    unsigned char plain[MAX_PKT];
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

    while (atomic_load_explicit(&r->running, memory_order_relaxed)) {
        struct timeval tv = { .tv_sec = 0, .tv_usec = 10000 };
        setsockopt(r->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        ssize_t n = recvfrom(r->sockfd, buf, sizeof(buf), 0, NULL, NULL);
        if (n <= 0)
            continue;

        if (r->decrypt) {
            int plain_len = 0;
            if (decrypt_packet(ctx, r->key, buf, (int)n, plain, &plain_len) == 0)
                atomic_fetch_add_explicit(&r->nr_received, 1, memory_order_relaxed);
        } else {
            atomic_fetch_add_explicit(&r->nr_received, 1, memory_order_relaxed);
        }
    }

    EVP_CIPHER_CTX_free(ctx);
    return NULL;
}

static struct receiver *receiver_start(int decrypt, const char *psk) {
    struct receiver *r = calloc(1, sizeof(*r));
    atomic_init(&r->running, 1);
    atomic_init(&r->nr_received, 0);
    r->decrypt = decrypt;
    if (decrypt && psk)
        derive_key(psk, r->key);

    r->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(BENCH_PORT),
        .sin_addr.s_addr = htonl(INADDR_LOOPBACK),
    };
    bind(r->sockfd, (struct sockaddr *)&addr, sizeof(addr));
    pthread_create(&r->thread, NULL, receiver_thread, r);
    return r;
}

static long receiver_stop(struct receiver *r) {
    atomic_store_explicit(&r->running, 0, memory_order_relaxed);
    pthread_join(r->thread, NULL);
    long n = atomic_load(&r->nr_received);
    close(r->sockfd);
    free(r);
    return n;
}

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */

static double now_sec(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

static struct lmk_socket_log_handler *socket_handler_of(struct lmk_log_handler *h) {
    return (struct lmk_socket_log_handler *)h;
}

/* ------------------------------------------------------------------ */
/* Benchmarks                                                           */
/* ------------------------------------------------------------------ */

static void bench_plain(int nr_logs, const char *label, const char *msg) {
    struct receiver *rx = receiver_start(0, NULL);

    lmk_init();
    lmk_get_config()->ring_buffer_size = RING_SZ;
    struct lmk_logger      *logger  = lmk_get_logger("bench");
    struct lmk_log_handler *handler = lmk_get_socket_log_handler("sh");
    lmk_attach_log_listener(handler, "127.0.0.1", BENCH_PORT);
    lmk_attach_log_handler(logger, handler);
    lmk_set_log_level(logger, LMK_LOG_LEVEL_INFO);

    /* warm up */
    LMK_LOG_INFO(logger, "warmup");
    usleep(5000);

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++)
        LMK_LOG_INFO(logger, "%s %d", msg, i);
    double enqueue_sec = now_sec() - t0;

    unsigned long dropped = atomic_load(&socket_handler_of(handler)->dropped);
    lmk_destroy();
    double total_sec = now_sec() - t0;

    usleep(20000); /* let loopback drain */
    long received = receiver_stop(rx);

    printf("plain   %-8s  enqueue %7.2fms  total %7.2fms  "
           "sent %-6d  recv %-6ld  dropped %-6lu  throughput ~%.0fK/sec\n",
           label, enqueue_sec * 1000, total_sec * 1000,
           nr_logs, received, dropped,
           (nr_logs - (int)dropped) / total_sec / 1000.0);
}

static void bench_encrypted(int nr_logs, const char *label, const char *msg) {
    const char *psk = "bench-secret-key";
    struct receiver *rx = receiver_start(1, psk);

    lmk_init();
    lmk_get_config()->ring_buffer_size = RING_SZ;
    struct lmk_logger      *logger  = lmk_get_logger("bench");
    struct lmk_log_handler *handler = lmk_get_socket_log_handler("sh");
    lmk_set_socket_psk(handler, psk);
    lmk_attach_log_listener(handler, "127.0.0.1", BENCH_PORT);
    lmk_attach_log_handler(logger, handler);
    lmk_set_log_level(logger, LMK_LOG_LEVEL_INFO);

    /* warm up */
    LMK_LOG_INFO(logger, "warmup");
    usleep(5000);

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++)
        LMK_LOG_INFO(logger, "%s %d", msg, i);
    double enqueue_sec = now_sec() - t0;

    unsigned long dropped = atomic_load(&socket_handler_of(handler)->dropped);
    lmk_destroy();
    double total_sec = now_sec() - t0;

    usleep(20000);
    long received = receiver_stop(rx);

    printf("encrypt %-8s  enqueue %7.2fms  total %7.2fms  "
           "sent %-6d  recv %-6ld  dropped %-6lu  throughput ~%.0fK/sec\n",
           label, enqueue_sec * 1000, total_sec * 1000,
           nr_logs, received, dropped,
           (nr_logs - (int)dropped) / total_sec / 1000.0);
}

static void bench_drop(int ring_sz, int nr_logs) {
    struct receiver *rx = receiver_start(0, NULL);

    lmk_init();
    lmk_get_config()->ring_buffer_size = (unsigned int)ring_sz;
    struct lmk_logger      *logger  = lmk_get_logger("bench");
    struct lmk_log_handler *handler = lmk_get_socket_log_handler("sh");
    lmk_attach_log_listener(handler, "127.0.0.1", BENCH_PORT);
    lmk_attach_log_handler(logger, handler);
    lmk_set_log_level(logger, LMK_LOG_LEVEL_INFO);

    LMK_LOG_INFO(logger, "warmup");
    usleep(5000);

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++)
        LMK_LOG_INFO(logger, MSG_SHORT " %d", i);
    double enqueue_sec = now_sec() - t0;

    unsigned long dropped = atomic_load(&socket_handler_of(handler)->dropped);
    lmk_destroy();

    usleep(20000);
    receiver_stop(rx);

    double drop_pct = 100.0 * (double)dropped / nr_logs;
    printf("drop    ring=%-6d  %d sent  %lu dropped (%4.1f%%)  avg %.1f ns/call\n",
           ring_sz, nr_logs, dropped, drop_pct,
           enqueue_sec * 1e9 / nr_logs);
}

static void bench_latency(int nr_logs) {
    struct receiver *rx = receiver_start(0, NULL);

    double *samples = malloc(sizeof(double) * (size_t)nr_logs);

    lmk_init();
    lmk_get_config()->ring_buffer_size = (unsigned int)nr_logs;
    struct lmk_logger      *logger  = lmk_get_logger("bench");
    struct lmk_log_handler *handler = lmk_get_socket_log_handler("sh");
    lmk_attach_log_listener(handler, "127.0.0.1", BENCH_PORT);
    lmk_attach_log_handler(logger, handler);
    lmk_set_log_level(logger, LMK_LOG_LEVEL_INFO);

    LMK_LOG_INFO(logger, "warmup");
    usleep(5000);

    for (int i = 0; i < nr_logs; i++) {
        double t0 = now_sec();
        LMK_LOG_INFO(logger, MSG_SHORT " %d", i);
        samples[i] = now_sec() - t0;
    }
    lmk_destroy();
    usleep(20000);
    receiver_stop(rx);

    /* sort and print percentiles */
    for (int i = 0; i < nr_logs - 1; i++)
        for (int j = i + 1; j < nr_logs; j++)
            if (samples[j] < samples[i]) {
                double tmp = samples[i]; samples[i] = samples[j]; samples[j] = tmp;
            }

    printf("latency (enqueue only, %d samples)\n", nr_logs);
    printf("  min=%5.2fus  p50=%5.2fus  p95=%5.2fus  p99=%5.2fus  p999=%6.2fus  max=%7.2fus\n",
           samples[0]                    * 1e6,
           samples[(int)(0.500 * nr_logs)] * 1e6,
           samples[(int)(0.950 * nr_logs)] * 1e6,
           samples[(int)(0.990 * nr_logs)] * 1e6,
           samples[(int)(0.999 * nr_logs)] * 1e6,
           samples[nr_logs - 1]           * 1e6);

    free(samples);
}

/* ------------------------------------------------------------------ */
/* Rate-limited benchmarks                                              */
/* ------------------------------------------------------------------ */

static unsigned long bench_rate_once(int nr_logs, int rps, int encrypted) {
    const char *psk = "bench-secret-key";
    struct receiver *rx = receiver_start(encrypted, encrypted ? psk : NULL);

    lmk_init();
    lmk_get_config()->ring_buffer_size = RING_SZ;
    struct lmk_logger      *logger  = lmk_get_logger("bench");
    struct lmk_log_handler *handler = lmk_get_socket_log_handler("sh");
    if (encrypted)
        lmk_set_socket_psk(handler, psk);
    lmk_attach_log_listener(handler, "127.0.0.1", BENCH_PORT);
    lmk_attach_log_handler(logger, handler);
    lmk_set_log_level(logger, LMK_LOG_LEVEL_INFO);

    LMK_LOG_INFO(logger, "warmup");
    usleep(5000);

    struct lmk_rate_limiter rl;
    lmk_rate_limiter_init(&rl, rps);
    for (int i = 0; i < nr_logs; i++) {
        LMK_LOG_INFO(logger, MSG_SHORT " %d", i);
        lmk_rate_limiter_wait(&rl);
    }

    unsigned long dropped = atomic_load(&socket_handler_of(handler)->dropped);
    lmk_destroy();
    usleep(20000);
    receiver_stop(rx);
    return dropped;
}

static void bench_sweep(int nr_logs, int encrypted) {
    static const int rates[] = {
        100000, 200000, 300000, 400000, 500000,
        600000, 700000, 800000, 900000, 1000000,
        1250000, 1500000, 2000000
    };
    int nr_rates = (int)(sizeof(rates) / sizeof(rates[0]));
    const char *mode = encrypted ? "encrypt" : "plain  ";

    printf("%s sweep  (ring=%d, %d logs/rate)\n", mode, RING_SZ, nr_logs);
    printf("  %-10s  %-10s  %-10s  %s\n", "rate/sec", "dropped", "logged", "status");
    printf("  %-10s  %-10s  %-10s  %s\n", "--------", "-------", "------", "------");

    int limit = 0;
    for (int i = 0; i < nr_rates; i++) {
        int rps = rates[i];
        unsigned long dropped = bench_rate_once(nr_logs, rps, encrypted);
        unsigned long logged  = (unsigned long)nr_logs - dropped;
        int ok = dropped == 0;
        if (ok) limit = rps;
        printf("  %-10d  %-10lu  %-10lu  %s\n",
               rps, dropped, logged, ok ? "OK" : "DROPS");
    }
    printf("  => drop-free ceiling: %dK/sec\n\n", limit / 1000);
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */

int main(void) {
    printf("=== Socket handler benchmark (%d logs, loopback UDP) ===\n\n", NR_LOGS);

    printf("--- Throughput: short messages ---\n");
    for (int r = 0; r < 3; r++) bench_plain    (NR_LOGS, "short", MSG_SHORT);
    for (int r = 0; r < 3; r++) bench_encrypted(NR_LOGS, "short", MSG_SHORT);
    printf("\n");

    printf("--- Throughput: long messages ---\n");
    for (int r = 0; r < 3; r++) bench_plain    (NR_LOGS, "long", MSG_LONG);
    for (int r = 0; r < 3; r++) bench_encrypted(NR_LOGS, "long", MSG_LONG);
    printf("\n");

    printf("--- Drop behaviour (ring overload) ---\n");
    bench_drop(64,   NR_LOGS);
    bench_drop(256,  NR_LOGS);
    bench_drop(1024, NR_LOGS);
    bench_drop(8192, NR_LOGS);
    printf("\n");

    printf("--- Per-call enqueue latency ---\n");
    bench_latency(10000);
    printf("\n");

    printf("--- Rate sweep: find drop-free throughput ceiling ---\n\n");
    bench_sweep(NR_LOGS, 0);
    bench_sweep(NR_LOGS, 1);

    return 0;
}
