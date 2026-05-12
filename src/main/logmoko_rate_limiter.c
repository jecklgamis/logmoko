#include <time.h>
#include "logmoko_rate_limiter.h"

static double rl_now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

void lmk_rate_limiter_init(struct lmk_rate_limiter *rl, int rps) {
    rl->rps   = rps;
    rl->count = 0;
    rl->start = 0.0;
}

void lmk_rate_limiter_wait(struct lmk_rate_limiter *rl) {
    if (rl->rps <= 0)
        return;

    double now = rl_now();
    if (rl->start == 0.0)
        rl->start = now;

    rl->count++;
    double deadline = rl->start + (double)rl->count / rl->rps;
    double wait     = deadline - now;
    if (wait > 0.0) {
        struct timespec ts;
        ts.tv_sec  = (time_t)wait;
        ts.tv_nsec = (long)((wait - (double)ts.tv_sec) * 1e9);
        nanosleep(&ts, NULL);
    }
}

void lmk_rate_limiter_reset(struct lmk_rate_limiter *rl) {
    rl->count = 0;
    rl->start = 0.0;
}
