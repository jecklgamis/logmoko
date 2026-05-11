#ifndef BENCH_LOGMOKO_H
#define BENCH_LOGMOKO_H

void bench_logmoko(int nr_logs);
void bench_logmoko_full(int run, int nr_logs);
void bench_logmoko_rate_limited(int nr_logs, int rps);

#endif /* BENCH_LOGMOKO_H */
