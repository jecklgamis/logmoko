#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../src/main/logmoko.h"

#define NR_LOGS 100000

static double now_sec(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

static int count_lines(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    int n = 0;
    char buf[256];
    while (fgets(buf, sizeof(buf), f)) n++;
    fclose(f);
    return n;
}

static void run(int ring_sz) {
    remove("bench_rb.log");

    lmk_init();
    lmk_get_config()->ring_buffer_size = ring_sz;
    struct lmk_logger *logger = lmk_get_logger("bench");
    struct lmk_log_handler *fh = lmk_get_file_log_handler("fh", "bench_rb.log");
    lmk_attach_log_handler(logger, fh);
    lmk_set_log_level(logger, LMK_LOG_LEVEL_INFO);

    double t0 = now_sec();
    for (int i = 0; i < NR_LOGS; i++)
        LMK_LOG_INFO(logger, "Benchmark log message number %d", i);
    double enqueue = now_sec() - t0;

    lmk_destroy();
    double total = now_sec() - t0;

    int written = count_lines("bench_rb.log");
    printf("ring=%-6d  enqueue=%6.1fms  total=%6.1fms  written=%6d/%d  (%.0f%%)\n",
           ring_sz, enqueue * 1000, total * 1000, written, NR_LOGS,
           100.0 * written / NR_LOGS);
    remove("bench_rb.log");
}

int main(void) {
    printf("=== Ring buffer size sweep (%d logs) ===\n\n", NR_LOGS);
    int sizes[] = {256, 512, 1024, 2048, 4096, 8192, 16384, 32768, NR_LOGS};
    for (int i = 0; i < (int)(sizeof(sizes)/sizeof(sizes[0])); i++)
        run(sizes[i]);
    return 0;
}
