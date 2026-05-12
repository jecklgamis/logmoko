#ifndef LOGMOKO_RATE_LIMITER_H
#define LOGMOKO_RATE_LIMITER_H

/*
 * Token-bucket rate limiter. Call lmk_rate_limiter_wait() after each
 * operation; it sleeps just long enough to honour the target rate.
 *
 * Usage:
 *   struct lmk_rate_limiter rl;
 *   lmk_rate_limiter_init(&rl, 100000);   // 100K ops/sec
 *   for (...) {
 *       do_work();
 *       lmk_rate_limiter_wait(&rl);
 *   }
 */

struct lmk_rate_limiter {
    int    rps;    /* target operations per second */
    long   count;  /* number of operations so far  */
    double start;  /* wall-clock start time (set on first wait) */
};

/* Set the target rate. Does not start the clock until the first wait. */
void lmk_rate_limiter_init(struct lmk_rate_limiter *rl, int rps);

/* Sleep until the current operation's deadline. Call once per operation. */
void lmk_rate_limiter_wait(struct lmk_rate_limiter *rl);

/* Reset the limiter — next wait restarts the clock from scratch. */
void lmk_rate_limiter_reset(struct lmk_rate_limiter *rl);

#endif /* LOGMOKO_RATE_LIMITER_H */
