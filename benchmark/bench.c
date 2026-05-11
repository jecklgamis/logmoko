#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <syslog.h>

/* ---- zlog ---- */
#include <zlog.h>

/* ---- logmoko ---- */
#include "../src/main/logmoko.h"

#define NR_LOGS          100000
#define LMK_RING_SZ      8192
#define RATE_LIMIT_RPS   400000  /* target log calls per second for rate-limited bench */

/* 2KB payload: fills the log message slot to its maximum size */
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
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"  /* total ~1900 chars + seq number */

static double now_sec(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

/* Sleep until the next slot for a given target rate (calls/sec). */
static void rate_limit(int call_index, double start_sec, int rps) {
    double deadline = start_sec + (double)(call_index + 1) / rps;
    double now = now_sec();
    if (now < deadline) {
        double wait = deadline - now;
        struct timespec ts = {
            .tv_sec  = (time_t)wait,
            .tv_nsec = (long)((wait - (time_t)wait) * 1e9)
        };
        nanosleep(&ts, NULL);
    }
}

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

    printf("logmoko : enqueue %.3fms  total(+flush) %.3fms  (%d logs)\n",
           enqueue_sec * 1000, total_sec * 1000, nr_logs);
}

static void bench_zlog(int nr_logs) {
    int rc = zlog_init("bench_zlog.conf");
    if (rc) { fprintf(stderr, "zlog_init failed\n"); return; }
    zlog_category_t *c = zlog_get_category("bench");
    if (!c) { fprintf(stderr, "zlog_get_category failed\n"); zlog_fini(); return; }

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++)
        zlog_info(c, "Benchmark log message number %d", i);
    double enqueue_sec = now_sec() - t0;

    zlog_fini();
    double total_sec = now_sec() - t0;

    printf("zlog    : enqueue %.3fms  total(+flush) %.3fms  (%d logs)\n",
           enqueue_sec * 1000, total_sec * 1000, nr_logs);
}

static void bench_syslog(int nr_logs) {
    openlog("bench", LOG_NDELAY, LOG_USER);

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++)
        syslog(LOG_INFO, "msg %d " MSG_PAD, i);
    double enqueue_sec = now_sec() - t0;

    closelog();
    double total_sec = now_sec() - t0;

    printf("syslog  : enqueue %.3fms  total(+flush) %.3fms  (%d logs)\n",
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

    printf("syslog  : enqueue %.3fms  total(+flush) %.3fms  (%d logs @ %d/sec)\n",
           enqueue_sec * 1000, total_sec * 1000, nr_logs, rps);
}

static void bench_zlog_rate_limited(int nr_logs, int rps) {
    int rc = zlog_init("bench_zlog.conf");
    if (rc) { fprintf(stderr, "zlog_init failed\n"); return; }
    zlog_category_t *c = zlog_get_category("bench");
    if (!c) { fprintf(stderr, "zlog_get_category failed\n"); zlog_fini(); return; }

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++) {
        zlog_info(c, "msg %d " MSG_PAD, i);
        rate_limit(i, t0, rps);
    }
    double enqueue_sec = now_sec() - t0;

    zlog_fini();
    double total_sec = now_sec() - t0;

    printf("zlog    : enqueue %.3fms  total(+flush) %.3fms  (%d logs @ %d/sec)\n",
           enqueue_sec * 1000, total_sec * 1000, nr_logs, rps);
    remove("bench_zlog.log");
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

    printf("logmoko : enqueue %.3fms  total(+flush) %.3fms  (%d logs @ %d/sec)\n",
           enqueue_sec * 1000, total_sec * 1000, nr_logs, rps);
    remove("bench_logmoko_rl.log");
}

static void bench_rate_sweep(void) {
    static const int rates[] = {
        100000, 200000, 300000, 400000, 500000, 600000, 700000,
        800000, 900000, 1000000, 1500000, 2000000, 3000000, 5000000
    };
    int nr_rates = sizeof(rates) / sizeof(rates[0]);
    int nr_logs  = 50000;

    printf("=== Drop-free threshold sweep (ring=%d, %d logs each) ===\n\n", LMK_RING_SZ, nr_logs);
    printf("  %-12s  %-10s  %-10s  %s\n", "rate/sec", "dropped", "logged", "status");
    printf("  %-12s  %-10s  %-10s  %s\n", "--------", "-------", "------", "------");

    for (int i = 0; i < nr_rates; i++) {
        int rps = rates[i];

        lmk_init();
        lmk_get_config()->ring_buffer_size = LMK_RING_SZ;
        struct lmk_logger *logger = lmk_get_logger("sweep");
        struct lmk_file_log_handler *fh =
            (struct lmk_file_log_handler *)lmk_get_file_log_handler("fh", "bench_sweep.log");
        lmk_attach_log_handler(logger, (struct lmk_log_handler *)fh);
        lmk_set_log_level(logger, LMK_LOG_LEVEL_INFO);

        double t0 = now_sec();
        for (int j = 0; j < nr_logs; j++) {
            LMK_LOG_INFO(logger, "msg %d " MSG_PAD, j);
            rate_limit(j, t0, rps);
        }

        unsigned long dropped = fh->dropped;
        lmk_destroy();
        remove("bench_sweep.log");

        unsigned long logged = nr_logs - dropped;
        printf("  %-12d  %-10lu  %-10lu  %s\n",
               rps, dropped, logged, dropped == 0 ? "OK (no drops)" : "DROPS");
    }
    printf("\n");
}

int main(void) {
    printf("=== %d logs — enqueue latency (small ring buffer, drops expected) ===\n\n", NR_LOGS);
    for (int r = 1; r <= 3; r++) {
        printf("run %d: ", r);
        bench_logmoko(NR_LOGS);
    }
    printf("\n");
    for (int r = 1; r <= 3; r++) {
        printf("run %d: ", r);
        bench_zlog(NR_LOGS);
    }
    printf("\n");
    for (int r = 1; r <= 3; r++) {
        printf("run %d: ", r);
        bench_syslog(NR_LOGS);
    }

    remove("bench_logmoko.log");
    remove("bench_zlog.log");

    printf("\n=== %d logs — total throughput including full drain to disk ===\n\n", NR_LOGS);
    /* Give logmoko a ring buffer large enough to hold everything so nothing drops */
    for (int r = 1; r <= 3; r++) {
        lmk_init();
        lmk_get_config()->ring_buffer_size = NR_LOGS;
        struct lmk_logger *logger = lmk_get_logger("bench");
        struct lmk_log_handler *fh = lmk_get_file_log_handler("fh", "bench_logmoko_full.log");
        lmk_attach_log_handler(logger, fh);
        lmk_set_log_level(logger, LMK_LOG_LEVEL_INFO);
        double t0 = now_sec();
        for (int i = 0; i < NR_LOGS; i++)
            LMK_LOG_INFO(logger, "msg %d " MSG_PAD, i);
        lmk_destroy();
        printf("logmoko run %d: %.3fms total  (%d logs, ring=%d)\n",
               r, (now_sec() - t0) * 1000, NR_LOGS, NR_LOGS);
        remove("bench_logmoko_full.log");
    }
    printf("\n");
    for (int r = 1; r <= 3; r++) {
        printf("zlog    run %d: ", r);
        bench_zlog(NR_LOGS);
        remove("bench_zlog.log");
    }
    printf("\n");
    for (int r = 1; r <= 3; r++) {
        printf("syslog  run %d: ", r);
        bench_syslog(NR_LOGS);
    }

    /* Rate-limited: simulate a sustained 10k logs/sec workload */
    int rl_logs = 200000; /* 200000 logs at 400k/sec = 0.5 sec elapsed */
    printf("\n=== %d logs — rate-limited at %d logs/sec (sustained, realistic workload) ===\n\n",
           rl_logs, RATE_LIMIT_RPS);
    for (int r = 1; r <= 3; r++) {
        printf("run %d: ", r);
        bench_logmoko_rate_limited(rl_logs, RATE_LIMIT_RPS);
    }
    printf("\n");
    for (int r = 1; r <= 3; r++) {
        printf("run %d: ", r);
        bench_zlog_rate_limited(rl_logs, RATE_LIMIT_RPS);
    }
    printf("\n");
    for (int r = 1; r <= 3; r++) {
        printf("run %d: ", r);
        bench_syslog_rate_limited(rl_logs, RATE_LIMIT_RPS);
    }

    bench_rate_sweep();

    return 0;
}
