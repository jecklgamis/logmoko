#ifndef BENCH_LOGMOKO_H
#define BENCH_LOGMOKO_H

void bench_logmoko(int nr_logs);
void bench_logmoko_full(int run, int nr_logs);
void bench_logmoko_rate_limited(int nr_logs, int rps);
void bench_logmoko_mt(int nr_threads, int nr_logs);
void bench_logmoko_latency(int nr_logs);
void bench_logmoko_short(int run, int nr_logs);
void bench_logmoko_filtered(int nr_calls);

#endif /* BENCH_LOGMOKO_H */
