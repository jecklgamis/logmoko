#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <syslog.h>
#include <zlog.h>

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>

extern "C" {
#include "../src/main/logmoko.h"
}

#define NR_LOGS          100000
#define LMK_RING_SZ      8192
#define RATE_LIMIT_RPS   400000

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

static double now_sec() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

static void rate_limit(int idx, double start, int rps) {
    double deadline = start + (double)(idx + 1) / rps;
    double now = now_sec();
    if (now < deadline) {
        double wait = deadline - now;
        struct timespec ts;
        ts.tv_sec  = (time_t)wait;
        ts.tv_nsec = (long)((wait - (time_t)wait) * 1e9);
        nanosleep(&ts, NULL);
    }
}

/* ------------------------------------------------------------------ */
/* logmoko                                                              */
/* ------------------------------------------------------------------ */

static void bench_logmoko(int nr_logs) {
    lmk_init();
    lmk_get_config()->ring_buffer_size = LMK_RING_SZ;
    struct lmk_logger *logger = lmk_get_logger("bench");
    struct lmk_log_handler *fh = lmk_get_file_log_handler("fh", "bench_logmoko.log");
    lmk_attach_log_handler(logger, fh);
    lmk_set_log_level(logger, LMK_LOG_LEVEL_INFO);

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++)
        LMK_LOG_INFO(logger, "msg %d " MSG_PAD, i);
    double enqueue_sec = now_sec() - t0;

    lmk_destroy();
    double total_sec = now_sec() - t0;
    remove("bench_logmoko.log");

    printf("logmoko : enqueue %7.3fms  total(+flush) %7.3fms  (%d logs)\n",
           enqueue_sec * 1000, total_sec * 1000, nr_logs);
}

static void bench_logmoko_full(int run, int nr_logs) {
    lmk_init();
    lmk_get_config()->ring_buffer_size = nr_logs;
    struct lmk_logger *logger = lmk_get_logger("bench");
    struct lmk_log_handler *fh = lmk_get_file_log_handler("fh", "bench_logmoko_full.log");
    lmk_attach_log_handler(logger, fh);
    lmk_set_log_level(logger, LMK_LOG_LEVEL_INFO);

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++)
        LMK_LOG_INFO(logger, "msg %d " MSG_PAD, i);
    lmk_destroy();

    printf("logmoko run %d: %7.3fms total  (%d logs, ring=%d)\n",
           run, (now_sec() - t0) * 1000, nr_logs, nr_logs);
    remove("bench_logmoko_full.log");
}

static void bench_logmoko_rate_limited(int nr_logs, int rps) {
    lmk_init();
    lmk_get_config()->ring_buffer_size = LMK_RING_SZ;
    struct lmk_logger *logger = lmk_get_logger("bench");
    struct lmk_log_handler *fh = lmk_get_file_log_handler("fh", "bench_logmoko_rl.log");
    lmk_attach_log_handler(logger, fh);
    lmk_set_log_level(logger, LMK_LOG_LEVEL_INFO);

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++) {
        LMK_LOG_INFO(logger, "msg %d " MSG_PAD, i);
        rate_limit(i, t0, rps);
    }
    double enqueue_sec = now_sec() - t0;

    lmk_destroy();
    double total_sec = now_sec() - t0;
    remove("bench_logmoko_rl.log");

    printf("logmoko : enqueue %7.3fms  total(+flush) %7.3fms  (%d logs @ %d/sec)\n",
           enqueue_sec * 1000, total_sec * 1000, nr_logs, rps);
}

/* ------------------------------------------------------------------ */
/* spdlog                                                               */
/* ------------------------------------------------------------------ */

static std::shared_ptr<spdlog::async_logger> make_spdlog(const char *file, int ring_sz,
                                                          spdlog::async_overflow_policy policy) {
    spdlog::init_thread_pool(ring_sz, 1);
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(file, true);
    auto logger = std::make_shared<spdlog::async_logger>(
        "bench", sink, spdlog::thread_pool(), policy);
    logger->set_level(spdlog::level::info);
    logger->set_pattern("%v");
    return logger;
}

static void bench_spdlog(int nr_logs) {
    auto logger = make_spdlog("bench_spdlog.log", LMK_RING_SZ,
                              spdlog::async_overflow_policy::discard_new);
    auto tp = spdlog::thread_pool();

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++)
        logger->info("msg {} " MSG_PAD, i);
    double enqueue_sec = now_sec() - t0;
    size_t dropped = tp->discard_counter();

    spdlog::shutdown();
    double total_sec = now_sec() - t0;
    remove("bench_spdlog.log");

    printf("spdlog  : enqueue %7.3fms  total(+flush) %7.3fms  (%d logs, dropped=%zu)\n",
           enqueue_sec * 1000, total_sec * 1000, nr_logs, dropped);
}

static void bench_spdlog_full(int run, int nr_logs) {
    auto logger = make_spdlog("bench_spdlog_full.log", nr_logs,
                              spdlog::async_overflow_policy::block);

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++)
        logger->info("msg {} " MSG_PAD, i);
    spdlog::shutdown();

    printf("spdlog  run %d: %7.3fms total  (%d logs, ring=%d)\n",
           run, (now_sec() - t0) * 1000, nr_logs, nr_logs);
    remove("bench_spdlog_full.log");
}

static void bench_spdlog_rate_limited(int nr_logs, int rps) {
    auto logger = make_spdlog("bench_spdlog_rl.log", LMK_RING_SZ,
                              spdlog::async_overflow_policy::discard_new);

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++) {
        logger->info("msg {} " MSG_PAD, i);
        rate_limit(i, t0, rps);
    }
    double enqueue_sec = now_sec() - t0;

    spdlog::shutdown();
    double total_sec = now_sec() - t0;
    remove("bench_spdlog_rl.log");

    printf("spdlog  : enqueue %7.3fms  total(+flush) %7.3fms  (%d logs @ %d/sec)\n",
           enqueue_sec * 1000, total_sec * 1000, nr_logs, rps);
}

/* ------------------------------------------------------------------ */
/* zlog                                                                 */
/* ------------------------------------------------------------------ */

static void bench_zlog(int nr_logs) {
    if (zlog_init("bench_zlog.conf")) { fprintf(stderr, "zlog_init failed\n"); return; }
    zlog_category_t *c = zlog_get_category("bench");
    if (!c) { zlog_fini(); return; }

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++)
        zlog_info(c, "msg %d " MSG_PAD, i);
    double enqueue_sec = now_sec() - t0;

    zlog_fini();
    double total_sec = now_sec() - t0;
    remove("bench_zlog.log");

    printf("zlog    : enqueue %7.3fms  total(+flush) %7.3fms  (%d logs)\n",
           enqueue_sec * 1000, total_sec * 1000, nr_logs);
}

static void bench_zlog_rate_limited(int nr_logs, int rps) {
    if (zlog_init("bench_zlog.conf")) { fprintf(stderr, "zlog_init failed\n"); return; }
    zlog_category_t *c = zlog_get_category("bench");
    if (!c) { zlog_fini(); return; }

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++) {
        zlog_info(c, "msg %d " MSG_PAD, i);
        rate_limit(i, t0, rps);
    }
    double enqueue_sec = now_sec() - t0;

    zlog_fini();
    double total_sec = now_sec() - t0;
    remove("bench_zlog.log");

    printf("zlog    : enqueue %7.3fms  total(+flush) %7.3fms  (%d logs @ %d/sec)\n",
           enqueue_sec * 1000, total_sec * 1000, nr_logs, rps);
}

/* ------------------------------------------------------------------ */
/* syslog                                                               */
/* ------------------------------------------------------------------ */

static void bench_syslog(int nr_logs) {
    openlog("bench", LOG_NDELAY, LOG_USER);

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++)
        syslog(LOG_INFO, "msg %d " MSG_PAD, i);
    double enqueue_sec = now_sec() - t0;

    closelog();
    double total_sec = now_sec() - t0;

    printf("syslog  : enqueue %7.3fms  total(+flush) %7.3fms  (%d logs)\n",
           enqueue_sec * 1000, total_sec * 1000, nr_logs);
}

static void bench_syslog_rate_limited(int nr_logs, int rps) {
    openlog("bench", LOG_NDELAY, LOG_USER);

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++) {
        syslog(LOG_INFO, "msg %d " MSG_PAD, i);
        rate_limit(i, t0, rps);
    }
    double enqueue_sec = now_sec() - t0;

    closelog();
    double total_sec = now_sec() - t0;

    printf("syslog  : enqueue %7.3fms  total(+flush) %7.3fms  (%d logs @ %d/sec)\n",
           enqueue_sec * 1000, total_sec * 1000, nr_logs, rps);
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */

int main() {
    printf("=== %d logs — enqueue latency (ring=%d, drops expected) ===\n\n",
           NR_LOGS, LMK_RING_SZ);
    for (int r = 1; r <= 3; r++) { printf("run %d: ", r); bench_logmoko(NR_LOGS); }
    printf("\n");
    for (int r = 1; r <= 3; r++) { printf("run %d: ", r); bench_spdlog(NR_LOGS); }
    printf("\n");
    for (int r = 1; r <= 3; r++) { printf("run %d: ", r); bench_zlog(NR_LOGS); }
    printf("\n");
    for (int r = 1; r <= 3; r++) { printf("run %d: ", r); bench_syslog(NR_LOGS); }

    printf("\n=== %d logs — total throughput including full drain to disk ===\n\n", NR_LOGS);
    for (int r = 1; r <= 3; r++) bench_logmoko_full(r, NR_LOGS);
    printf("\n");
    for (int r = 1; r <= 3; r++) bench_spdlog_full(r, NR_LOGS);
    printf("\n");
    for (int r = 1; r <= 3; r++) { printf("zlog    run %d: ", r); bench_zlog(NR_LOGS); }
    printf("\n");
    for (int r = 1; r <= 3; r++) { printf("syslog  run %d: ", r); bench_syslog(NR_LOGS); }

    int rl_logs = 200000;
    printf("\n=== %d logs — rate-limited at %d logs/sec (sustained) ===\n\n",
           rl_logs, RATE_LIMIT_RPS);
    for (int r = 1; r <= 3; r++) { printf("run %d: ", r); bench_logmoko_rate_limited(rl_logs, RATE_LIMIT_RPS); }
    printf("\n");
    for (int r = 1; r <= 3; r++) { printf("run %d: ", r); bench_spdlog_rate_limited(rl_logs, RATE_LIMIT_RPS); }
    printf("\n");
    for (int r = 1; r <= 3; r++) { printf("run %d: ", r); bench_zlog_rate_limited(rl_logs, RATE_LIMIT_RPS); }
    printf("\n");
    for (int r = 1; r <= 3; r++) { printf("run %d: ", r); bench_syslog_rate_limited(rl_logs, RATE_LIMIT_RPS); }

    return 0;
}
