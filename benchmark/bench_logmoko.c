/* logmoko benchmark routines — compiled as C to avoid C11/C++ atomic incompatibility */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include "../src/main/logmoko.h"

#define LMK_RING_SZ 8192

#define MSG_PAD \
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" \
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" \
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" \
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" \
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" \
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" \
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" \
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" \
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" \
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" \
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" \
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" \
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" \
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" \
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" \
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" \
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" \
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" \
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" \
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" \
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" \
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" \
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" \
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"

static double lmk_bench_now_sec(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

static void lmk_bench_rate_limit(int idx, double start, int rps) {
    double deadline = start + (double)(idx + 1) / rps;
    double now = lmk_bench_now_sec();
    if (now < deadline) {
        double wait = deadline - now;
        struct timespec ts;
        ts.tv_sec  = (time_t)wait;
        ts.tv_nsec = (long)((wait - (time_t)wait) * 1e9);
        nanosleep(&ts, NULL);
    }
}

void bench_logmoko(int nr_logs) {
    lmk_init();
    lmk_get_config()->ring_buffer_size = LMK_RING_SZ;
    struct lmk_logger *logger = lmk_get_logger("bench");
    struct lmk_log_handler *fh = lmk_get_file_log_handler("fh", "bench_logmoko.log");
    lmk_attach_log_handler(logger, fh);
    lmk_set_log_level(logger, LMK_LOG_LEVEL_INFO);

    double t0 = lmk_bench_now_sec();
    for (int i = 0; i < nr_logs; i++)
        LMK_LOG_INFO(logger, "msg %d " MSG_PAD, i);
    double enqueue_sec = lmk_bench_now_sec() - t0;

    lmk_destroy();
    double total_sec = lmk_bench_now_sec() - t0;
    remove("bench_logmoko.log");

    printf("logmoko : enqueue %7.3fms  total(+flush) %7.3fms  (%d logs)\n",
           enqueue_sec * 1000, total_sec * 1000, nr_logs);
}

void bench_logmoko_full(int run, int nr_logs) {
    lmk_init();
    lmk_get_config()->ring_buffer_size = (unsigned int)nr_logs;
    struct lmk_logger *logger = lmk_get_logger("bench");
    struct lmk_log_handler *fh = lmk_get_file_log_handler("fh", "bench_logmoko_full.log");
    lmk_attach_log_handler(logger, fh);
    lmk_set_log_level(logger, LMK_LOG_LEVEL_INFO);

    double t0 = lmk_bench_now_sec();
    for (int i = 0; i < nr_logs; i++)
        LMK_LOG_INFO(logger, "msg %d " MSG_PAD, i);
    lmk_destroy();

    printf("logmoko run %d: %7.3fms total  (%d logs, ring=%d)\n",
           run, (lmk_bench_now_sec() - t0) * 1000, nr_logs, nr_logs);
    remove("bench_logmoko_full.log");
}

void bench_logmoko_rate_limited(int nr_logs, int rps) {
    lmk_init();
    lmk_get_config()->ring_buffer_size = LMK_RING_SZ;
    struct lmk_logger *logger = lmk_get_logger("bench");
    struct lmk_log_handler *fh = lmk_get_file_log_handler("fh", "bench_logmoko_rl.log");
    lmk_attach_log_handler(logger, fh);
    lmk_set_log_level(logger, LMK_LOG_LEVEL_INFO);

    double t0 = lmk_bench_now_sec();
    for (int i = 0; i < nr_logs; i++) {
        LMK_LOG_INFO(logger, "msg %d " MSG_PAD, i);
        lmk_bench_rate_limit(i, t0, rps);
    }
    double enqueue_sec = lmk_bench_now_sec() - t0;

    lmk_destroy();
    double total_sec = lmk_bench_now_sec() - t0;
    remove("bench_logmoko_rl.log");

    printf("logmoko : enqueue %7.3fms  total(+flush) %7.3fms  (%d logs @ %d/sec)\n",
           enqueue_sec * 1000, total_sec * 1000, nr_logs, rps);
}

/* ------------------------------------------------------------------ */
/* multi-threaded producer                                              */
/* ------------------------------------------------------------------ */

struct lmk_mt_args {
    struct lmk_logger *logger;
    int nr_logs;
};

static void *lmk_mt_thread(void *arg) {
    struct lmk_mt_args *a = (struct lmk_mt_args *)arg;
    for (int i = 0; i < a->nr_logs; i++)
        LMK_LOG_INFO(a->logger, "msg %d " MSG_PAD, i);
    return NULL;
}

void bench_logmoko_mt(int nr_threads, int nr_logs) {
    int per_thread = nr_logs / nr_threads;
    int total      = per_thread * nr_threads;

    lmk_init();
    lmk_get_config()->ring_buffer_size = (unsigned int)total;
    struct lmk_logger     *logger = lmk_get_logger("bench");
    struct lmk_log_handler *fh    = lmk_get_file_log_handler("fh", "bench_logmoko_mt.log");
    lmk_attach_log_handler(logger, fh);
    lmk_set_log_level(logger, LMK_LOG_LEVEL_INFO);

    pthread_t          *threads = (pthread_t *)malloc(sizeof(pthread_t) * (size_t)nr_threads);
    struct lmk_mt_args *args    = (struct lmk_mt_args *)malloc(sizeof(struct lmk_mt_args) * (size_t)nr_threads);

    double t0 = lmk_bench_now_sec();
    for (int i = 0; i < nr_threads; i++) {
        args[i].logger  = logger;
        args[i].nr_logs = per_thread;
        pthread_create(&threads[i], NULL, lmk_mt_thread, &args[i]);
    }
    for (int i = 0; i < nr_threads; i++)
        pthread_join(threads[i], NULL);
    double enqueue_sec = lmk_bench_now_sec() - t0;

    lmk_destroy();
    double total_sec = lmk_bench_now_sec() - t0;
    remove("bench_logmoko_mt.log");

    free(threads);
    free(args);

    printf("logmoko (%2d threads): enqueue %7.3fms  total %7.3fms  throughput ~%.0fK logs/sec\n",
           nr_threads, enqueue_sec * 1000, total_sec * 1000,
           total / total_sec / 1000.0);
}

/* ------------------------------------------------------------------ */
/* per-call tail latency                                                */
/* ------------------------------------------------------------------ */

#define MSG_SHORT "user=12345 action=login status=200 duration=42ms ip=192.168.1.1"

static int lmk_cmp_double(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

static void lmk_print_latency(const char *name, double *s, int n) {
    qsort(s, (size_t)n, sizeof(double), lmk_cmp_double);
    printf("logmoko %-6s  min=%5.2fus  p50=%5.2fus  p95=%5.2fus  p99=%5.2fus  p999=%6.2fus  max=%7.2fus\n",
           name,
           s[0]              * 1e6,
           s[(int)(0.500*n)] * 1e6,
           s[(int)(0.950*n)] * 1e6,
           s[(int)(0.990*n)] * 1e6,
           s[(int)(0.999*n)] * 1e6,
           s[n-1]            * 1e6);
}

void bench_logmoko_latency(int nr_logs) {
    double *samples = (double *)malloc(sizeof(double) * (size_t)nr_logs);

    /* ---- long message ---- */
    lmk_init();
    lmk_get_config()->ring_buffer_size = (unsigned int)nr_logs;
    struct lmk_logger     *logger = lmk_get_logger("bench");
    struct lmk_log_handler *fh    = lmk_get_file_log_handler("fh", "bench_logmoko_lat.log");
    lmk_attach_log_handler(logger, fh);
    lmk_set_log_level(logger, LMK_LOG_LEVEL_INFO);

    for (int i = 0; i < nr_logs; i++) {
        double t0 = lmk_bench_now_sec();
        LMK_LOG_INFO(logger, "msg %d " MSG_PAD, i);
        samples[i] = lmk_bench_now_sec() - t0;
    }
    lmk_destroy();
    remove("bench_logmoko_lat.log");
    lmk_print_latency("long ", samples, nr_logs);

    /* ---- short message ---- */
    lmk_init();
    lmk_get_config()->ring_buffer_size = (unsigned int)nr_logs;
    logger = lmk_get_logger("bench");
    fh     = lmk_get_file_log_handler("fh", "bench_logmoko_lat_s.log");
    lmk_attach_log_handler(logger, fh);
    lmk_set_log_level(logger, LMK_LOG_LEVEL_INFO);

    for (int i = 0; i < nr_logs; i++) {
        double t0 = lmk_bench_now_sec();
        LMK_LOG_INFO(logger, MSG_SHORT);
        samples[i] = lmk_bench_now_sec() - t0;
    }
    lmk_destroy();
    remove("bench_logmoko_lat_s.log");
    lmk_print_latency("short", samples, nr_logs);

    free(samples);
}

/* ------------------------------------------------------------------ */
/* short message throughput                                             */
/* ------------------------------------------------------------------ */

void bench_logmoko_short(int run, int nr_logs) {
    lmk_init();
    lmk_get_config()->ring_buffer_size = (unsigned int)nr_logs;
    struct lmk_logger     *logger = lmk_get_logger("bench");
    struct lmk_log_handler *fh    = lmk_get_file_log_handler("fh", "bench_logmoko_short.log");
    lmk_attach_log_handler(logger, fh);
    lmk_set_log_level(logger, LMK_LOG_LEVEL_INFO);

    double t0 = lmk_bench_now_sec();
    for (int i = 0; i < nr_logs; i++)
        LMK_LOG_INFO(logger, MSG_SHORT);
    lmk_destroy();
    double elapsed = lmk_bench_now_sec() - t0;
    remove("bench_logmoko_short.log");

    printf("logmoko short run %d: %7.3fms  (%d logs, ~%.0fK/sec)\n",
           run, elapsed * 1000, nr_logs, nr_logs / elapsed / 1000.0);
}

/* ------------------------------------------------------------------ */
/* filtered call overhead (DEBUG call, logger level INFO)               */
/* ------------------------------------------------------------------ */

void bench_logmoko_filtered(int nr_calls) {
    lmk_init();
    struct lmk_logger     *logger = lmk_get_logger("bench");
    struct lmk_log_handler *fh    = lmk_get_file_log_handler("fh", "bench_logmoko_filt.log");
    lmk_attach_log_handler(logger, fh);
    lmk_set_log_level(logger, LMK_LOG_LEVEL_INFO); /* DEBUG calls filtered here */

    double t0 = lmk_bench_now_sec();
    for (int i = 0; i < nr_calls; i++)
        LMK_LOG_DEBUG(logger, MSG_SHORT);
    double elapsed = lmk_bench_now_sec() - t0;

    lmk_destroy();
    remove("bench_logmoko_filt.log");

    printf("logmoko  filtered: %5.2f ns/call  (%dM calls in %.1fms)\n",
           elapsed * 1e9 / nr_calls, nr_calls / 1000000, elapsed * 1000);
}
