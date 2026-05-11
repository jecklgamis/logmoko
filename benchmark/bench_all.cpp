/**
 * Unified benchmark: logmoko vs spdlog vs quill vs g3log vs fmtlog vs zlog vs syslog vs log.c
 *
 * Build: make bench_all
 * Run:   ./bench_all
 */

/* ---- log.c (vendored, C): must come before syslog.h to avoid enum/macro conflicts ---- */
extern "C" {
#include "log.h"
}
/* Capture log.c's LOG_INFO (=2) before syslog.h redefines LOG_INFO as a macro */
static const int LOGC_LEVEL_INFO = LOG_INFO;

/* ---- C standard headers ---- */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <syslog.h>

/* ---- zlog (C) ---- */
#include <zlog.h>

/* ---- log4c (C) ---- */
#include <log4c.h>

/* ---- logmoko (C) — compiled separately to avoid C11/C++ atomic conflict ---- */
extern "C" {
#include "bench_logmoko.h"
}

/* ---- spdlog (C++) ---- */
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>

/* ---- quill (C++) ---- */
#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/LogMacros.h>
#include <quill/Logger.h>
#include <quill/sinks/FileSink.h>

/* ---- g3log (C++) ---- */
#include <g3log/g3log.hpp>
#include <g3log/logworker.hpp>

/* ---- fmtlog (C++, header-only, vendored) ---- */
#define FMTLOG_BLOCK 0
#include "fmtlog.h"
#include "fmtlog-inl.h"

/* ------------------------------------------------------------------ */

#define NR_LOGS        100000
#define LMK_RING_SZ    8192
#define RATE_LIMIT_RPS 400000
#define MSG_SHORT      "user=12345 action=login status=200 duration=42ms ip=192.168.1.1"

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
/* quill                                                                */
/* ------------------------------------------------------------------ */

static void bench_quill(int nr_logs) {
    quill::BackendOptions bo;
    quill::Backend::start(bo);
    auto sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
        "bench_quill.log",
        []() {
            quill::FileSinkConfig cfg;
            cfg.set_open_mode('w');
            return cfg;
        }(),
        quill::FileEventNotifier{});
    auto *logger = quill::Frontend::create_or_get_logger(
        "bench", std::move(sink),
        quill::PatternFormatterOptions{"%(message)"});
    logger->set_log_level(quill::LogLevel::Info);

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++)
        LOG_INFO(logger, "msg {} " MSG_PAD, i);
    double enqueue_sec = now_sec() - t0;

    quill::Backend::stop();
    double total_sec = now_sec() - t0;
    remove("bench_quill.log");

    printf("quill   : enqueue %7.3fms  total(+flush) %7.3fms  (%d logs)\n",
           enqueue_sec * 1000, total_sec * 1000, nr_logs);
}

static void bench_quill_full(int run, int nr_logs) {
    quill::BackendOptions bo;
    quill::Backend::start(bo);
    auto sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
        "bench_quill_full.log",
        []() {
            quill::FileSinkConfig cfg;
            cfg.set_open_mode('w');
            return cfg;
        }(),
        quill::FileEventNotifier{});
    auto *logger = quill::Frontend::create_or_get_logger(
        "bench_full", std::move(sink),
        quill::PatternFormatterOptions{"%(message)"});
    logger->set_log_level(quill::LogLevel::Info);

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++)
        LOG_INFO(logger, "msg {} " MSG_PAD, i);
    quill::Backend::stop();

    printf("quill   run %d: %7.3fms total  (%d logs)\n",
           run, (now_sec() - t0) * 1000, nr_logs);
    remove("bench_quill_full.log");
}

static void bench_quill_rate_limited(int nr_logs, int rps) {
    quill::BackendOptions bo;
    quill::Backend::start(bo);
    auto sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
        "bench_quill_rl.log",
        []() {
            quill::FileSinkConfig cfg;
            cfg.set_open_mode('w');
            return cfg;
        }(),
        quill::FileEventNotifier{});
    auto *logger = quill::Frontend::create_or_get_logger(
        "bench_rl", std::move(sink),
        quill::PatternFormatterOptions{"%(message)"});
    logger->set_log_level(quill::LogLevel::Info);

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++) {
        LOG_INFO(logger, "msg {} " MSG_PAD, i);
        rate_limit(i, t0, rps);
    }
    double enqueue_sec = now_sec() - t0;

    quill::Backend::stop();
    double total_sec = now_sec() - t0;
    remove("bench_quill_rl.log");

    printf("quill   : enqueue %7.3fms  total(+flush) %7.3fms  (%d logs @ %d/sec)\n",
           enqueue_sec * 1000, total_sec * 1000, nr_logs, rps);
}

/* ------------------------------------------------------------------ */
/* g3log                                                                */
/* ------------------------------------------------------------------ */

static void bench_g3log(int nr_logs) {
    auto logworker = g3::LogWorker::createLogWorker();
    auto handle = logworker->addDefaultLogger("bench_g3log", "./");
    g3::initializeLogging(logworker.get());

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++)
        LOGF(INFO, "msg %d " MSG_PAD, i);
    double enqueue_sec = now_sec() - t0;

    logworker.reset();  /* blocks until queue drained */
    double total_sec = now_sec() - t0;

    /* g3log appends a timestamp to the filename; clean up matching files */
    system("rm -f bench_g3log*.log 2>/dev/null");

    printf("g3log   : enqueue %7.3fms  total(+flush) %7.3fms  (%d logs)\n",
           enqueue_sec * 1000, total_sec * 1000, nr_logs);
}

static void bench_g3log_full(int run, int nr_logs) {
    auto logworker = g3::LogWorker::createLogWorker();
    logworker->addDefaultLogger("bench_g3log_full", "./");
    g3::initializeLogging(logworker.get());

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++)
        LOGF(INFO, "msg %d " MSG_PAD, i);
    logworker.reset();

    printf("g3log   run %d: %7.3fms total  (%d logs)\n",
           run, (now_sec() - t0) * 1000, nr_logs);
    system("rm -f bench_g3log_full*.log 2>/dev/null");
}

static void bench_g3log_rate_limited(int nr_logs, int rps) {
    auto logworker = g3::LogWorker::createLogWorker();
    logworker->addDefaultLogger("bench_g3log_rl", "./");
    g3::initializeLogging(logworker.get());

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++) {
        LOGF(INFO, "msg %d " MSG_PAD, i);
        rate_limit(i, t0, rps);
    }
    double enqueue_sec = now_sec() - t0;

    logworker.reset();
    double total_sec = now_sec() - t0;
    system("rm -f bench_g3log_rl*.log 2>/dev/null");

    printf("g3log   : enqueue %7.3fms  total(+flush) %7.3fms  (%d logs @ %d/sec)\n",
           enqueue_sec * 1000, total_sec * 1000, nr_logs, rps);
}

/* ------------------------------------------------------------------ */
/* fmtlog                                                               */
/* ------------------------------------------------------------------ */

static void bench_fmtlog(int nr_logs) {
    fmtlog::setLogFile("bench_fmtlog.log", true);
    fmtlog::setLogLevel(fmtlog::INF);
    fmtlog::startPollingThread(1000000);  /* 1 ms poll interval */

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++)
        logi("msg {} " MSG_PAD, i);
    double enqueue_sec = now_sec() - t0;

    fmtlog::stopPollingThread();
    fmtlog::poll(true);  /* final flush */
    double total_sec = now_sec() - t0;
    remove("bench_fmtlog.log");

    printf("fmtlog  : enqueue %7.3fms  total(+flush) %7.3fms  (%d logs)\n",
           enqueue_sec * 1000, total_sec * 1000, nr_logs);
}

static void bench_fmtlog_full(int run, int nr_logs) {
    fmtlog::setLogFile("bench_fmtlog_full.log", true);
    fmtlog::setLogLevel(fmtlog::INF);
    fmtlog::startPollingThread(1000000);

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++)
        logi("msg {} " MSG_PAD, i);
    fmtlog::stopPollingThread();
    fmtlog::poll(true);

    printf("fmtlog  run %d: %7.3fms total  (%d logs)\n",
           run, (now_sec() - t0) * 1000, nr_logs);
    remove("bench_fmtlog_full.log");
}

static void bench_fmtlog_rate_limited(int nr_logs, int rps) {
    fmtlog::setLogFile("bench_fmtlog_rl.log", true);
    fmtlog::setLogLevel(fmtlog::INF);
    fmtlog::startPollingThread(1000000);

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++) {
        logi("msg {} " MSG_PAD, i);
        rate_limit(i, t0, rps);
    }
    double enqueue_sec = now_sec() - t0;

    fmtlog::stopPollingThread();
    fmtlog::poll(true);
    double total_sec = now_sec() - t0;
    remove("bench_fmtlog_rl.log");

    printf("fmtlog  : enqueue %7.3fms  total(+flush) %7.3fms  (%d logs @ %d/sec)\n",
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
/* log.c (synchronous)                                                  */
/* ------------------------------------------------------------------ */

static void bench_logc(int nr_logs) {
    FILE *fp = fopen("bench_logc.log", "w");
    if (!fp) { fprintf(stderr, "log.c: cannot open file\n"); return; }
    log_set_quiet(true);
    log_add_fp(fp, LOGC_LEVEL_INFO);

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++)
        log_info("msg %d " MSG_PAD, i);
    double enqueue_sec = now_sec() - t0;

    fclose(fp);
    double total_sec = now_sec() - t0;
    remove("bench_logc.log");

    /* reset log.c state for next call */
    log_set_quiet(false);

    printf("log.c   : enqueue %7.3fms  total(+flush) %7.3fms  (%d logs)\n",
           enqueue_sec * 1000, total_sec * 1000, nr_logs);
}

static void bench_logc_rate_limited(int nr_logs, int rps) {
    FILE *fp = fopen("bench_logc_rl.log", "w");
    if (!fp) { fprintf(stderr, "log.c: cannot open file\n"); return; }
    log_set_quiet(true);
    log_add_fp(fp, LOGC_LEVEL_INFO);

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++) {
        log_info("msg %d " MSG_PAD, i);
        rate_limit(i, t0, rps);
    }
    double enqueue_sec = now_sec() - t0;

    fclose(fp);
    double total_sec = now_sec() - t0;
    remove("bench_logc_rl.log");
    log_set_quiet(false);

    printf("log.c   : enqueue %7.3fms  total(+flush) %7.3fms  (%d logs @ %d/sec)\n",
           enqueue_sec * 1000, total_sec * 1000, nr_logs, rps);
}

/* ------------------------------------------------------------------ */
/* log4c (synchronous)                                                  */
/* ------------------------------------------------------------------ */

/* log4c cannot be re-initialized after fini; single run only */
static void bench_log4c_once(int nr_logs) {
    if (log4c_init()) { fprintf(stderr, "log4c_init failed\n"); return; }
    log4c_category_t *cat = log4c_category_get("bench");
    log4c_category_set_priority(cat, LOG4C_PRIORITY_INFO);

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++)
        log4c_category_log(cat, LOG4C_PRIORITY_INFO, "msg %d " MSG_PAD, i);
    double enqueue_sec = now_sec() - t0;
    log4c_fini();
    double total_sec = now_sec() - t0;
    printf("run 1: log4c   : enqueue %7.3fms  total(+flush) %7.3fms  (%d logs, single run)\n",
           enqueue_sec * 1000, total_sec * 1000, nr_logs);
}

/* ------------------------------------------------------------------ */
/* memory footprint helper (macOS)                                      */
/* ------------------------------------------------------------------ */

#include <mach/mach.h>

static size_t phys_mem() {
    task_vm_info_data_t info;
    mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_VM_INFO,
                  (task_info_t)&info, &count) == KERN_SUCCESS)
        return (size_t)info.phys_footprint;
    return 0;
}

/* ------------------------------------------------------------------ */
/* drop behavior under overload                                         */
/* ------------------------------------------------------------------ */

static void bench_spdlog_drop(int ring_sz, int nr_logs) {
    auto logger = make_spdlog("bench_spdlog_drop.log", ring_sz,
                              spdlog::async_overflow_policy::discard_new);
    auto tp = spdlog::thread_pool();
    logger->info("warmup");

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++)
        logger->info(MSG_SHORT);
    double enqueue_sec = now_sec() - t0;
    size_t dropped = tp->discard_counter();

    spdlog::shutdown();
    remove("bench_spdlog_drop.log");

    double drop_pct = 100.0 * (double)dropped / nr_logs;
    printf("spdlog   ring=%-5d  %d sent  %zu dropped (%4.1f%%)  avg %4.1f ns/call\n",
           ring_sz, nr_logs, dropped, drop_pct, enqueue_sec * 1e9 / nr_logs);
}

static void bench_quill_drop(int ring_sz, int nr_logs) {
    quill::BackendOptions bo;
    quill::Backend::start(bo);
    auto sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
        "bench_quill_drop.log",
        []() { quill::FileSinkConfig c; c.set_open_mode('w'); return c; }(),
        quill::FileEventNotifier{});
    auto *logger = quill::Frontend::create_or_get_logger(
        "bench_drop", std::move(sink), quill::PatternFormatterOptions{"%(message)"});
    logger->set_log_level(quill::LogLevel::Info);
    LOG_INFO(logger, "warmup");

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++)
        LOG_INFO(logger, MSG_SHORT);
    double enqueue_sec = now_sec() - t0;

    quill::Backend::stop();
    remove("bench_quill_drop.log");

    /* quill doesn't expose a drop counter; report call time only */
    printf("quill    ring=%-5d  %d sent  drops=n/a              avg %4.1f ns/call\n",
           ring_sz, nr_logs, enqueue_sec * 1e9 / nr_logs);
}

static void bench_fmtlog_drop(int ring_sz, int nr_logs) {
    (void)ring_sz; /* fmtlog ring size not directly settable at runtime */
    fmtlog::setLogFile("bench_fmtlog_drop.log", true);
    fmtlog::setLogLevel(fmtlog::INF);
    fmtlog::startPollingThread(1000000);
    logi("warmup");

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++)
        logi(MSG_SHORT);
    double enqueue_sec = now_sec() - t0;

    fmtlog::stopPollingThread();
    fmtlog::poll(true);
    remove("bench_fmtlog_drop.log");

    printf("fmtlog   ring=n/a    %d sent  drops=n/a              avg %4.1f ns/call\n",
           nr_logs, enqueue_sec * 1e9 / nr_logs);
}

/* ------------------------------------------------------------------ */
/* memory footprint                                                     */
/* ------------------------------------------------------------------ */

static void bench_spdlog_memory(int ring_sz) {
    size_t before = phys_mem();
    spdlog::init_thread_pool((size_t)ring_sz, 1);
    auto sink   = std::make_shared<spdlog::sinks::basic_file_sink_mt>("bench_spdlog_mem.log", true);
    auto logger = std::make_shared<spdlog::async_logger>(
        "bench_mem", sink, spdlog::thread_pool(),
        spdlog::async_overflow_policy::discard_new);
    logger->info("touch");
    size_t after = phys_mem();
    spdlog::shutdown();
    remove("bench_spdlog_mem.log");
    long delta = (long)after - (long)before;
    printf("spdlog   ring=%-5d  %+.1f MB  (before=%zuMB after=%zuMB)\n",
           ring_sz, delta / 1048576.0, before / 1048576, after / 1048576);
}

static void bench_quill_memory(int ring_sz) {
    size_t before = phys_mem();
    quill::BackendOptions bo;
    quill::Backend::start(bo);
    auto sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
        "bench_quill_mem.log",
        []() { quill::FileSinkConfig c; c.set_open_mode('w'); return c; }(),
        quill::FileEventNotifier{});
    auto *logger = quill::Frontend::create_or_get_logger(
        "bench_mem", std::move(sink), quill::PatternFormatterOptions{"%(message)"});
    logger->set_log_level(quill::LogLevel::Info);
    LOG_INFO(logger, "touch");
    size_t after = phys_mem();
    quill::Backend::stop();
    remove("bench_quill_mem.log");
    long delta = (long)after - (long)before;
    printf("quill    ring=%-5d  %+.1f MB  (before=%zuMB after=%zuMB)\n",
           ring_sz, delta / 1048576.0, before / 1048576, after / 1048576);
}

static void bench_fmtlog_memory() {
    size_t before = phys_mem();
    fmtlog::setLogFile("bench_fmtlog_mem.log", true);
    fmtlog::setLogLevel(fmtlog::INF);
    fmtlog::startPollingThread(1000000);
    logi("touch");
    size_t after = phys_mem();
    fmtlog::stopPollingThread();
    fmtlog::poll(true);
    remove("bench_fmtlog_mem.log");
    long delta = (long)after - (long)before;
    printf("fmtlog   ring=n/a    %+.1f MB  (before=%zuMB after=%zuMB)\n",
           delta / 1048576.0, before / 1048576, after / 1048576);
}

static void bench_g3log_memory() {
    size_t before = phys_mem();
    auto logworker = g3::LogWorker::createLogWorker();
    logworker->addDefaultLogger("bench_g3log_mem", "./");
    g3::initializeLogging(logworker.get());
    LOGF(INFO, "touch");
    size_t after = phys_mem();
    logworker.reset();
    system("rm -f bench_g3log_mem*.log 2>/dev/null");
    long delta = (long)after - (long)before;
    printf("g3log    ring=n/a    %+.1f MB  (before=%zuMB after=%zuMB)\n",
           delta / 1048576.0, before / 1048576, after / 1048576);
}

/* ------------------------------------------------------------------ */
/* filtered call overhead                                               */
/* ------------------------------------------------------------------ */

static void bench_spdlog_filtered(int nr_calls) {
    spdlog::init_thread_pool(1024, 1);
    auto sink   = std::make_shared<spdlog::sinks::basic_file_sink_mt>("bench_spdlog_filt.log", true);
    auto logger = std::make_shared<spdlog::async_logger>(
        "bench_filt", sink, spdlog::thread_pool(),
        spdlog::async_overflow_policy::discard_new);
    logger->set_level(spdlog::level::info); /* debug calls filtered */

    double t0 = now_sec();
    for (int i = 0; i < nr_calls; i++)
        logger->debug(MSG_SHORT);
    double elapsed = now_sec() - t0;

    spdlog::shutdown();
    remove("bench_spdlog_filt.log");
    printf("spdlog   filtered: %5.2f ns/call  (%dM calls in %.1fms)\n",
           elapsed * 1e9 / nr_calls, nr_calls / 1000000, elapsed * 1000);
}

static void bench_quill_filtered(int nr_calls) {
    quill::BackendOptions bo;
    quill::Backend::start(bo);
    auto sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
        "bench_quill_filt.log",
        []() { quill::FileSinkConfig c; c.set_open_mode('w'); return c; }(),
        quill::FileEventNotifier{});
    auto *logger = quill::Frontend::create_or_get_logger(
        "bench_filt", std::move(sink), quill::PatternFormatterOptions{"%(message)"});
    logger->set_log_level(quill::LogLevel::Info); /* debug calls filtered */

    double t0 = now_sec();
    for (int i = 0; i < nr_calls; i++)
        LOG_DEBUG(logger, MSG_SHORT);
    double elapsed = now_sec() - t0;

    quill::Backend::stop();
    remove("bench_quill_filt.log");
    printf("quill    filtered: %5.2f ns/call  (%dM calls in %.1fms)\n",
           elapsed * 1e9 / nr_calls, nr_calls / 1000000, elapsed * 1000);
}

static void bench_fmtlog_filtered(int nr_calls) {
    fmtlog::setLogFile("bench_fmtlog_filt.log", true);
    fmtlog::setLogLevel(fmtlog::INF); /* debug calls filtered */
    fmtlog::startPollingThread(1000000);

    double t0 = now_sec();
    for (int i = 0; i < nr_calls; i++)
        logd(MSG_SHORT);
    double elapsed = now_sec() - t0;

    fmtlog::stopPollingThread();
    fmtlog::poll(true);
    remove("bench_fmtlog_filt.log");
    printf("fmtlog   filtered: %5.2f ns/call  (%dM calls in %.1fms)\n",
           elapsed * 1e9 / nr_calls, nr_calls / 1000000, elapsed * 1000);
}

static void bench_g3log_filtered(int nr_calls) {
    auto logworker = g3::LogWorker::createLogWorker();
    logworker->addDefaultLogger("bench_g3log_filt", "./");
    g3::initializeLogging(logworker.get());

    /* g3log has no simple runtime level setter; use a volatile guard to
       simulate the "condition checked, not taken" path without the compiler
       optimizing the call away entirely */
    static volatile bool enabled = false;
    double t0 = now_sec();
    for (int i = 0; i < nr_calls; i++)
        LOGF_IF(INFO, enabled, MSG_SHORT);
    double elapsed = now_sec() - t0;

    logworker.reset();
    system("rm -f bench_g3log_filt*.log 2>/dev/null");
    printf("g3log    filtered: %5.2f ns/call  (%dM calls in %.1fms, via volatile guard)\n",
           elapsed * 1e9 / nr_calls, nr_calls / 1000000, elapsed * 1000);
}

/* ------------------------------------------------------------------ */
/* latency + short-message helpers                                      */
/* ------------------------------------------------------------------ */

static void print_latency(const char *lib, const char *msg, std::vector<double> &s) {
    std::sort(s.begin(), s.end());
    size_t n = s.size();
    auto us = [&](double p) { return s[(size_t)(p * n)] * 1e6; };
    printf("%-8s %-6s  min=%5.2fus  p50=%5.2fus  p95=%5.2fus  p99=%5.2fus  p999=%6.2fus  max=%7.2fus\n",
           lib, msg, us(0), us(0.50), us(0.95), us(0.99), us(0.999), s.back() * 1e6);
}

/* ------------------------------------------------------------------ */
/* spdlog latency + short                                               */
/* ------------------------------------------------------------------ */

static void bench_spdlog_latency(int nr_logs) {
    std::vector<double> samples((size_t)nr_logs);

    for (const char *msg_label : {"long ", "short"}) {
        bool is_short = (msg_label[0] == 's');
        auto logger = make_spdlog("bench_spdlog_lat.log", nr_logs,
                                  spdlog::async_overflow_policy::block);
        for (int i = 0; i < nr_logs; i++) {
            double t0 = now_sec();
            if (is_short) logger->info(MSG_SHORT);
            else          logger->info("msg {} " MSG_PAD, i);
            samples[(size_t)i] = now_sec() - t0;
        }
        spdlog::shutdown();
        remove("bench_spdlog_lat.log");
        print_latency("spdlog", msg_label, samples);
    }
}

static void bench_spdlog_short(int run, int nr_logs) {
    auto logger = make_spdlog("bench_spdlog_short.log", nr_logs,
                              spdlog::async_overflow_policy::block);
    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++)
        logger->info(MSG_SHORT);
    spdlog::shutdown();
    double elapsed = now_sec() - t0;
    remove("bench_spdlog_short.log");
    printf("spdlog  short run %d: %7.3fms  (%d logs, ~%.0fK/sec)\n",
           run, elapsed * 1000, nr_logs, nr_logs / elapsed / 1000.0);
}

/* ------------------------------------------------------------------ */
/* quill latency + short                                                */
/* ------------------------------------------------------------------ */

static void bench_quill_latency(int nr_logs) {
    std::vector<double> samples((size_t)nr_logs);

    for (const char *msg_label : {"long ", "short"}) {
        bool is_short = (msg_label[0] == 's');
        quill::BackendOptions bo;
        quill::Backend::start(bo);
        std::string lname = std::string("bench_lat_") + msg_label;
        auto sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
            "bench_quill_lat.log",
            []() { quill::FileSinkConfig c; c.set_open_mode('w'); return c; }(),
            quill::FileEventNotifier{});
        auto *logger = quill::Frontend::create_or_get_logger(
            lname, std::move(sink), quill::PatternFormatterOptions{"%(message)"});
        logger->set_log_level(quill::LogLevel::Info);

        for (int i = 0; i < nr_logs; i++) {
            double t0 = now_sec();
            if (is_short) LOG_INFO(logger, MSG_SHORT);
            else          LOG_INFO(logger, "msg {} " MSG_PAD, i);
            samples[(size_t)i] = now_sec() - t0;
        }
        quill::Backend::stop();
        remove("bench_quill_lat.log");
        print_latency("quill", msg_label, samples);
    }
}

static void bench_quill_short(int run, int nr_logs) {
    quill::BackendOptions bo;
    quill::Backend::start(bo);
    auto sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
        "bench_quill_short.log",
        []() { quill::FileSinkConfig c; c.set_open_mode('w'); return c; }(),
        quill::FileEventNotifier{});
    auto *logger = quill::Frontend::create_or_get_logger(
        "bench_short", std::move(sink), quill::PatternFormatterOptions{"%(message)"});
    logger->set_log_level(quill::LogLevel::Info);

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++)
        LOG_INFO(logger, MSG_SHORT);
    quill::Backend::stop();
    double elapsed = now_sec() - t0;
    remove("bench_quill_short.log");
    printf("quill   short run %d: %7.3fms  (%d logs, ~%.0fK/sec)\n",
           run, elapsed * 1000, nr_logs, nr_logs / elapsed / 1000.0);
}

/* ------------------------------------------------------------------ */
/* fmtlog latency + short                                               */
/* ------------------------------------------------------------------ */

static void bench_fmtlog_latency(int nr_logs) {
    std::vector<double> samples((size_t)nr_logs);

    for (const char *msg_label : {"long ", "short"}) {
        bool is_short = (msg_label[0] == 's');
        fmtlog::setLogFile("bench_fmtlog_lat.log", true);
        fmtlog::setLogLevel(fmtlog::INF);
        fmtlog::startPollingThread(1000000);

        for (int i = 0; i < nr_logs; i++) {
            double t0 = now_sec();
            if (is_short) logi(MSG_SHORT);
            else          logi("msg {} " MSG_PAD, i);
            samples[(size_t)i] = now_sec() - t0;
        }
        fmtlog::stopPollingThread();
        fmtlog::poll(true);
        remove("bench_fmtlog_lat.log");
        print_latency("fmtlog", msg_label, samples);
    }
}

static void bench_fmtlog_short(int run, int nr_logs) {
    fmtlog::setLogFile("bench_fmtlog_short.log", true);
    fmtlog::setLogLevel(fmtlog::INF);
    fmtlog::startPollingThread(1000000);

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++)
        logi(MSG_SHORT);
    fmtlog::stopPollingThread();
    fmtlog::poll(true);
    double elapsed = now_sec() - t0;
    remove("bench_fmtlog_short.log");
    printf("fmtlog  short run %d: %7.3fms  (%d logs, ~%.0fK/sec)\n",
           run, elapsed * 1000, nr_logs, nr_logs / elapsed / 1000.0);
}

/* ------------------------------------------------------------------ */
/* g3log latency + short                                                */
/* ------------------------------------------------------------------ */

static void bench_g3log_latency(int nr_logs) {
    std::vector<double> samples((size_t)nr_logs);

    for (const char *msg_label : {"long ", "short"}) {
        bool is_short = (msg_label[0] == 's');
        auto logworker = g3::LogWorker::createLogWorker();
        logworker->addDefaultLogger("bench_g3log_lat", "./");
        g3::initializeLogging(logworker.get());

        for (int i = 0; i < nr_logs; i++) {
            double t0 = now_sec();
            if (is_short) LOGF(INFO, MSG_SHORT);
            else          LOGF(INFO, "msg %d " MSG_PAD, i);
            samples[(size_t)i] = now_sec() - t0;
        }
        logworker.reset();
        system("rm -f bench_g3log_lat*.log 2>/dev/null");
        print_latency("g3log", msg_label, samples);
    }
}

static void bench_g3log_short(int run, int nr_logs) {
    auto logworker = g3::LogWorker::createLogWorker();
    logworker->addDefaultLogger("bench_g3log_short", "./");
    g3::initializeLogging(logworker.get());

    double t0 = now_sec();
    for (int i = 0; i < nr_logs; i++)
        LOGF(INFO, MSG_SHORT);
    logworker.reset();
    double elapsed = now_sec() - t0;
    system("rm -f bench_g3log_short*.log 2>/dev/null");
    printf("g3log   short run %d: %7.3fms  (%d logs, ~%.0fK/sec)\n",
           run, elapsed * 1000, nr_logs, nr_logs / elapsed / 1000.0);
}

/* ------------------------------------------------------------------ */
/* syslog latency (sync, included for contrast)                         */
/* ------------------------------------------------------------------ */

static void bench_syslog_latency(int nr_logs) {
    std::vector<double> samples((size_t)nr_logs);
    openlog("bench", LOG_NDELAY, LOG_USER);
    for (int i = 0; i < nr_logs; i++) {
        double t0 = now_sec();
        syslog(LOG_INFO, MSG_SHORT);
        samples[(size_t)i] = now_sec() - t0;
    }
    closelog();
    print_latency("syslog", "short", samples);
}

/* ------------------------------------------------------------------ */
/* multi-threaded producers                                             */
/* ------------------------------------------------------------------ */

static void bench_spdlog_mt(int nr_threads, int nr_logs) {
    int per_thread = nr_logs / nr_threads;
    int total      = per_thread * nr_threads;

    auto logger = make_spdlog("bench_spdlog_mt.log", total,
                              spdlog::async_overflow_policy::block);

    std::vector<std::thread> threads;
    double t0 = now_sec();
    for (int i = 0; i < nr_threads; i++) {
        threads.emplace_back([&logger, per_thread]() {
            for (int j = 0; j < per_thread; j++)
                logger->info("msg {} " MSG_PAD, j);
        });
    }
    for (auto &t : threads) t.join();
    double enqueue_sec = now_sec() - t0;

    spdlog::shutdown();
    double total_sec = now_sec() - t0;
    remove("bench_spdlog_mt.log");

    printf("spdlog  (%2d threads): enqueue %7.3fms  total %7.3fms  throughput ~%.0fK logs/sec\n",
           nr_threads, enqueue_sec * 1000, total_sec * 1000,
           total / total_sec / 1000.0);
}

static void bench_quill_mt(int nr_threads, int nr_logs) {
    int per_thread = nr_logs / nr_threads;
    int total      = per_thread * nr_threads;

    quill::BackendOptions bo;
    quill::Backend::start(bo);
    auto sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
        "bench_quill_mt.log",
        []() { quill::FileSinkConfig cfg; cfg.set_open_mode('w'); return cfg; }(),
        quill::FileEventNotifier{});
    auto *logger = quill::Frontend::create_or_get_logger(
        "bench_mt", std::move(sink),
        quill::PatternFormatterOptions{"%(message)"});
    logger->set_log_level(quill::LogLevel::Info);

    std::vector<std::thread> threads;
    double t0 = now_sec();
    for (int i = 0; i < nr_threads; i++) {
        threads.emplace_back([logger, per_thread]() {
            for (int j = 0; j < per_thread; j++)
                LOG_INFO(logger, "msg {} " MSG_PAD, j);
        });
    }
    for (auto &t : threads) t.join();
    double enqueue_sec = now_sec() - t0;

    quill::Backend::stop();
    double total_sec = now_sec() - t0;
    remove("bench_quill_mt.log");

    printf("quill   (%2d threads): enqueue %7.3fms  total %7.3fms  throughput ~%.0fK logs/sec\n",
           nr_threads, enqueue_sec * 1000, total_sec * 1000,
           total / total_sec / 1000.0);
}

static void bench_fmtlog_mt(int nr_threads, int nr_logs) {
    int per_thread = nr_logs / nr_threads;
    int total      = per_thread * nr_threads;

    fmtlog::setLogFile("bench_fmtlog_mt.log", true);
    fmtlog::setLogLevel(fmtlog::INF);
    fmtlog::startPollingThread(1000000);

    std::vector<std::thread> threads;
    double t0 = now_sec();
    for (int i = 0; i < nr_threads; i++) {
        threads.emplace_back([per_thread]() {
            for (int j = 0; j < per_thread; j++)
                logi("msg {} " MSG_PAD, j);
        });
    }
    for (auto &t : threads) t.join();
    double enqueue_sec = now_sec() - t0;

    fmtlog::stopPollingThread();
    fmtlog::poll(true);
    double total_sec = now_sec() - t0;
    remove("bench_fmtlog_mt.log");

    printf("fmtlog  (%2d threads): enqueue %7.3fms  total %7.3fms  throughput ~%.0fK logs/sec\n",
           nr_threads, enqueue_sec * 1000, total_sec * 1000,
           total / total_sec / 1000.0);
}

static void bench_g3log_mt(int nr_threads, int nr_logs) {
    int per_thread = nr_logs / nr_threads;
    int total      = per_thread * nr_threads;

    auto logworker = g3::LogWorker::createLogWorker();
    logworker->addDefaultLogger("bench_g3log_mt", "./");
    g3::initializeLogging(logworker.get());

    std::vector<std::thread> threads;
    double t0 = now_sec();
    for (int i = 0; i < nr_threads; i++) {
        threads.emplace_back([per_thread]() {
            for (int j = 0; j < per_thread; j++)
                LOGF(INFO, "msg %d " MSG_PAD, j);
        });
    }
    for (auto &t : threads) t.join();
    double enqueue_sec = now_sec() - t0;

    logworker.reset();
    double total_sec = now_sec() - t0;
    system("rm -f bench_g3log_mt*.log 2>/dev/null");

    printf("g3log   (%2d threads): enqueue %7.3fms  total %7.3fms  throughput ~%.0fK logs/sec\n",
           nr_threads, enqueue_sec * 1000, total_sec * 1000,
           total / total_sec / 1000.0);
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */

int main() {
    /* ---- memory footprint (run first on a fresh process) ---- */
    printf("=== Memory footprint — 1 file handler, ring=1024 ===\n\n");
    bench_logmoko_memory(1024);
    bench_spdlog_memory(1024);
    bench_quill_memory(1024);
    bench_fmtlog_memory();
    bench_g3log_memory();
    printf("\n");

    /* ---- enqueue latency ---- */
    printf("=== %d logs — enqueue latency (async ring=%d, drops expected) ===\n\n",
           NR_LOGS, LMK_RING_SZ);

    for (int r = 1; r <= 3; r++) { printf("run %d: ", r); bench_logmoko(NR_LOGS); }
    printf("\n");
    for (int r = 1; r <= 3; r++) { printf("run %d: ", r); bench_spdlog(NR_LOGS); }
    printf("\n");
    for (int r = 1; r <= 3; r++) { printf("run %d: ", r); bench_quill(NR_LOGS); }
    printf("\n");
    for (int r = 1; r <= 3; r++) { printf("run %d: ", r); bench_g3log(NR_LOGS); }
    printf("\n");
    for (int r = 1; r <= 3; r++) { printf("run %d: ", r); bench_fmtlog(NR_LOGS); }
    printf("\n");
    for (int r = 1; r <= 3; r++) { printf("run %d: ", r); bench_zlog(NR_LOGS); }
    printf("\n");
    for (int r = 1; r <= 3; r++) { printf("run %d: ", r); bench_syslog(NR_LOGS); }
    printf("\n");
    for (int r = 1; r <= 3; r++) { printf("run %d: ", r); bench_logc(NR_LOGS); }
    printf("\n");
    printf("--- log4c (sync, stderr suppressed, single run only) ---\n");
    freopen("/dev/null", "w", stderr);
    bench_log4c_once(NR_LOGS);
    freopen("/dev/tty", "w", stderr);
    printf("\n");

    /* ---- full drain (async only) ---- */
    printf("=== %d logs — total throughput including full drain to disk ===\n\n", NR_LOGS);
    for (int r = 1; r <= 3; r++) bench_logmoko_full(r, NR_LOGS);
    printf("\n");
    for (int r = 1; r <= 3; r++) bench_spdlog_full(r, NR_LOGS);
    printf("\n");
    for (int r = 1; r <= 3; r++) bench_quill_full(r, NR_LOGS);
    printf("\n");
    for (int r = 1; r <= 3; r++) bench_g3log_full(r, NR_LOGS);
    printf("\n");
    for (int r = 1; r <= 3; r++) bench_fmtlog_full(r, NR_LOGS);
    printf("\n");

    /* ---- rate-limited ---- */
    int rl_logs = 200000;
    printf("=== %d logs — rate-limited at %d logs/sec (sustained) ===\n\n",
           rl_logs, RATE_LIMIT_RPS);
    for (int r = 1; r <= 3; r++) { printf("run %d: ", r); bench_logmoko_rate_limited(rl_logs, RATE_LIMIT_RPS); }
    printf("\n");
    for (int r = 1; r <= 3; r++) { printf("run %d: ", r); bench_spdlog_rate_limited(rl_logs, RATE_LIMIT_RPS); }
    printf("\n");
    for (int r = 1; r <= 3; r++) { printf("run %d: ", r); bench_quill_rate_limited(rl_logs, RATE_LIMIT_RPS); }
    printf("\n");
    for (int r = 1; r <= 3; r++) { printf("run %d: ", r); bench_g3log_rate_limited(rl_logs, RATE_LIMIT_RPS); }
    printf("\n");
    for (int r = 1; r <= 3; r++) { printf("run %d: ", r); bench_fmtlog_rate_limited(rl_logs, RATE_LIMIT_RPS); }
    printf("\n");
    for (int r = 1; r <= 3; r++) { printf("run %d: ", r); bench_zlog_rate_limited(rl_logs, RATE_LIMIT_RPS); }
    printf("\n");
    for (int r = 1; r <= 3; r++) { printf("run %d: ", r); bench_syslog_rate_limited(rl_logs, RATE_LIMIT_RPS); }
    printf("\n");
    for (int r = 1; r <= 3; r++) { printf("run %d: ", r); bench_logc_rate_limited(rl_logs, RATE_LIMIT_RPS); }
    printf("\n");

    /* ---- filtered call overhead ---- */
    int nr_filtered = 10000000; /* 10M calls */
    printf("=== Filtered call overhead — %dM DEBUG calls, logger level INFO ===\n\n",
           nr_filtered / 1000000);
    bench_logmoko_filtered(nr_filtered);
    bench_spdlog_filtered(nr_filtered);
    bench_quill_filtered(nr_filtered);
    bench_fmtlog_filtered(nr_filtered);
    bench_g3log_filtered(nr_filtered);
    printf("\n");

    /* ---- per-call tail latency ---- */
    printf("=== Per-call enqueue latency — %d logs, ring=%d (no drops) ===\n\n", NR_LOGS, NR_LOGS);
    bench_logmoko_latency(NR_LOGS);
    printf("\n");
    bench_spdlog_latency(NR_LOGS);
    printf("\n");
    bench_quill_latency(NR_LOGS);
    printf("\n");
    bench_fmtlog_latency(NR_LOGS);
    printf("\n");
    bench_g3log_latency(NR_LOGS);
    printf("\n");
    bench_syslog_latency(NR_LOGS);
    printf("\n");

    /* ---- short vs long message throughput ---- */
    printf("=== Short vs long message — %d logs, ring=%d (no drops) ===\n\n", NR_LOGS, NR_LOGS);
    printf("--- long message (~2KB) ---\n");
    for (int r = 1; r <= 3; r++) bench_logmoko_full(r, NR_LOGS);
    printf("\n");
    for (int r = 1; r <= 3; r++) bench_spdlog_full(r, NR_LOGS);
    printf("\n");
    for (int r = 1; r <= 3; r++) bench_quill_full(r, NR_LOGS);
    printf("\n");
    for (int r = 1; r <= 3; r++) bench_fmtlog_full(r, NR_LOGS);
    printf("\n");
    for (int r = 1; r <= 3; r++) bench_g3log_full(r, NR_LOGS);
    printf("\n--- short message (~62 bytes) ---\n");
    for (int r = 1; r <= 3; r++) bench_logmoko_short(r, NR_LOGS);
    printf("\n");
    for (int r = 1; r <= 3; r++) bench_spdlog_short(r, NR_LOGS);
    printf("\n");
    for (int r = 1; r <= 3; r++) bench_quill_short(r, NR_LOGS);
    printf("\n");
    for (int r = 1; r <= 3; r++) bench_fmtlog_short(r, NR_LOGS);
    printf("\n");
    for (int r = 1; r <= 3; r++) bench_g3log_short(r, NR_LOGS);
    printf("\n");

    /* ---- multi-threaded producers ---- */
    int mt_logs = 100000;
    int thread_counts[] = {1, 2, 4, 8};
    printf("=== Multi-threaded producers — %d total logs, ring=%d (no drops) ===\n\n",
           mt_logs, mt_logs);
    for (int tc : thread_counts) bench_logmoko_mt(tc, mt_logs);
    printf("\n");
    for (int tc : thread_counts) bench_spdlog_mt(tc, mt_logs);
    printf("\n");
    for (int tc : thread_counts) bench_quill_mt(tc, mt_logs);
    printf("\n");
    for (int tc : thread_counts) bench_fmtlog_mt(tc, mt_logs);
    printf("\n");
    for (int tc : thread_counts) bench_g3log_mt(tc, mt_logs);
    printf("\n");

    /* ---- drop behavior under overload ---- */
    int drop_logs = 100000;
    int drop_rings[] = {256, 1024, 8192};
    printf("=== Drop behavior — %d logs burst, varying ring sizes ===\n\n", drop_logs);
    for (int rs : drop_rings) bench_logmoko_drop(rs, drop_logs);
    printf("\n");
    for (int rs : drop_rings) bench_spdlog_drop(rs, drop_logs);
    printf("\n");
    bench_quill_drop(1024, drop_logs);
    printf("\n");
    bench_fmtlog_drop(1024, drop_logs);
    printf("\n");

    return 0;
}
