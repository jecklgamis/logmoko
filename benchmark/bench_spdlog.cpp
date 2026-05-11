#include <cstdio>
#include <ctime>
#include <vector>
#include <string>

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>

#define NR_LOGS      100000
#define RING_SZ      8192

/* Match the 2KB payload used in bench.c */
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

/* Create a fresh async logger with discard-new overflow policy */
static std::shared_ptr<spdlog::async_logger> make_logger(const char *logfile) {
    spdlog::init_thread_pool(RING_SZ, 1);
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logfile, true);
    auto logger = std::make_shared<spdlog::async_logger>(
        "bench", sink, spdlog::thread_pool(),
        spdlog::async_overflow_policy::discard_new);
    logger->set_level(spdlog::level::info);
    logger->set_pattern("%v");
    return logger;
}

static void bench_enqueue(int run) {
    auto logger = make_logger("bench_spdlog.log");
    auto tp = spdlog::thread_pool();

    double t0 = now_sec();
    for (int i = 0; i < NR_LOGS; i++)
        logger->info("msg {} " MSG_PAD, i);
    double enqueue_sec = now_sec() - t0;

    size_t dropped = tp->discard_counter();

    spdlog::shutdown();
    double total_sec = now_sec() - t0;
    remove("bench_spdlog.log");

    printf("run %d: spdlog  : enqueue %.3fms  total(+flush) %.3fms  (%d logs, dropped=%zu)\n",
           run, enqueue_sec * 1000, total_sec * 1000, NR_LOGS, dropped);
}

static void bench_full_drain(int run) {
    /* Large enough ring so nothing drops — matches logmoko's full-drain test */
    spdlog::init_thread_pool(NR_LOGS, 1);
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("bench_spdlog_full.log", true);
    auto logger = std::make_shared<spdlog::async_logger>(
        "bench", sink, spdlog::thread_pool(),
        spdlog::async_overflow_policy::block);
    logger->set_level(spdlog::level::info);
    logger->set_pattern("%v");

    double t0 = now_sec();
    for (int i = 0; i < NR_LOGS; i++)
        logger->info("msg {} " MSG_PAD, i);
    spdlog::shutdown();
    double total_sec = now_sec() - t0;
    remove("bench_spdlog_full.log");

    printf("spdlog  run %d: %.3fms total  (%d logs, ring=%d)\n",
           run, total_sec * 1000, NR_LOGS, NR_LOGS);
}

static void bench_rate_limited(int run, int nr_logs, int rps) {
    auto logger = make_logger("bench_spdlog_rl.log");

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++) {
        logger->info("msg {} " MSG_PAD, i);
        rate_limit(i, t0, rps);
    }
    double enqueue_sec = now_sec() - t0;

    spdlog::shutdown();
    double total_sec = now_sec() - t0;
    remove("bench_spdlog_rl.log");

    printf("run %d: spdlog  : enqueue %.3fms  total(+flush) %.3fms  (%d logs @ %d/sec)\n",
           run, enqueue_sec * 1000, total_sec * 1000, nr_logs, rps);
}

static void bench_rate_sweep() {
    static const int rates[] = {
        100000, 200000, 300000, 400000, 500000, 600000, 700000,
        800000, 900000, 1000000, 1500000, 2000000, 3000000, 5000000
    };
    int nr_rates = sizeof(rates) / sizeof(rates[0]);
    int nr_logs  = 50000;

    printf("=== Drop-free threshold sweep — spdlog (ring=%d, %d logs each) ===\n\n",
           RING_SZ, nr_logs);
    printf("  %-12s  %-10s  %-10s  %s\n", "rate/sec", "dropped", "logged", "status");
    printf("  %-12s  %-10s  %-10s  %s\n", "--------", "-------", "------", "------");

    for (int i = 0; i < nr_rates; i++) {
        int rps = rates[i];
        auto logger = make_logger("bench_spdlog_sweep.log");
        auto tp = spdlog::thread_pool();

        double t0 = now_sec();
        for (int j = 0; j < nr_logs; j++) {
            logger->info("msg {} " MSG_PAD, j);
            rate_limit(j, t0, rps);
        }

        size_t dropped = tp->discard_counter();
        spdlog::shutdown();
        remove("bench_spdlog_sweep.log");

        size_t logged = nr_logs - dropped;
        printf("  %-12d  %-10zu  %-10zu  %s\n",
               rps, dropped, logged, dropped == 0 ? "OK (no drops)" : "DROPS");
    }
    printf("\n");
}

int main() {
    printf("=== %d logs — enqueue latency (ring=%d, drops expected) ===\n\n", NR_LOGS, RING_SZ);
    for (int r = 1; r <= 3; r++)
        bench_enqueue(r);

    printf("\n=== %d logs — total throughput including full drain to disk ===\n\n", NR_LOGS);
    for (int r = 1; r <= 3; r++)
        bench_full_drain(r);

    int rl_logs = 200000;
    int rl_rps  = 400000;
    printf("\n=== %d logs — rate-limited at %d logs/sec ===\n\n", rl_logs, rl_rps);
    for (int r = 1; r <= 3; r++)
        bench_rate_limited(r, rl_logs, rl_rps);

    printf("\n");
    bench_rate_sweep();

    return 0;
}
