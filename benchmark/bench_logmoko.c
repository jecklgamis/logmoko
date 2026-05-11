/* logmoko benchmark routines — compiled as C to avoid C11/C++ atomic incompatibility */

#include <stdio.h>
#include <time.h>

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
