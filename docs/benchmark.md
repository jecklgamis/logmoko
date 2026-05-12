# Logmoko Benchmark Results

All benchmarks run on Apple M-series (macOS 14, `-O2`).

## File Handler

100,000 INFO logs to a file handler.

**Frameworks benchmarked:**

| Library | Version | Language | Files | LoC | Basis |
|---|---|---|---|---|---|
| logmoko | 2.0.0-rc10 | C | 19 | 2,219 | full source |
| spdlog | 1.17.0 | C++ | 90 | 10,761 | distributed headers |
| quill | 11.1.0 | C++ | 96 | 34,942 | distributed headers |
| g3log | 2.6 | C++ | 19 | 1,588 | installed headers |
| fmtlog | (vendored) | C++ | 2 | 1,459 | vendored headers (full) |
| zlog | 1.2.18 | C | 1 | 288 | public header only |
| log.c | 0.1.0 | C | 2 | 217 | vendored (full) |
| log4c | 1.2.4 | C | 26 | 2,589 | installed headers |
| syslog | — | C (POSIX) | — | — | OS built-in |

*LoC counted from installed/vendored files only. Compiled libraries (g3log, zlog, log4c) have additional implementation source not reflected here.*

**Enqueue latency — 100K burst (ring=8192, drops expected):**

| Library | Type | Enqueue | Total (+flush) |
|---|---|---|---|
| logmoko | async C | ~64 ms | ~71 ms |
| fmtlog | async C++ | ~2 ms | ~100 ms |
| quill | async C++ | ~1 ms | ~280 ms |
| spdlog | async C++ | ~163 ms | ~163 ms |
| g3log | async C++ | ~166 ms | ~254 ms |
| syslog | kernel IPC | ~41 ms | ~41 ms |
| log.c | sync C | ~600 ms | ~600 ms |
| zlog | sync C | ~2,000 ms | ~2,000 ms |
| log4c | sync C | ~0.3 ms | ~0.3 ms (stderr, no disk) |

*quill and fmtlog have near-zero enqueue latency; the async backend is the bottleneck on flush.
spdlog's enqueue time includes internal overflow-policy overhead at this ring size.*

**Full throughput — 100K logs, ring=100000 (zero drops):**

| Library | Total | Throughput |
|---|---|---|
| logmoko | ~85 ms | ~1.2M logs/sec |
| fmtlog | ~108 ms | ~930K logs/sec |
| spdlog | ~176 ms | ~570K logs/sec |
| g3log | ~255 ms | ~390K logs/sec |
| quill | ~300 ms | ~330K logs/sec |

**Sustained rate-limited workload — 200K logs at 400K logs/sec:**

| Library | Total |
|---|---|
| logmoko | ~500 ms (on schedule) |
| spdlog | ~500 ms (on schedule) |
| syslog | ~500 ms (on schedule, kernel-buffered) |
| g3log | ~528 ms |
| fmtlog | ~560 ms |
| quill | ~757 ms (backend lagging) |
| log.c | ~3,100 ms (sync, cannot keep up) |
| zlog | ~7,300 ms (sync, cannot keep up) |

**Multi-threaded producers — 100K total logs, ring=100K (no drops):**

| Library | 1 thread | 2 threads | 4 threads | 8 threads |
|---|---|---|---|---|
| logmoko | ~1,280K/sec | ~1,150K/sec | ~1,340K/sec | ~1,260K/sec |
| fmtlog | ~860K/sec | ~380K/sec | ~370K/sec | ~330K/sec |
| spdlog | ~435K/sec | ~432K/sec | ~434K/sec | ~377K/sec |
| g3log | ~350K/sec | ~402K/sec | ~344K/sec | ~358K/sec |
| quill | ~290K/sec | ~315K/sec | ~299K/sec | ~282K/sec |

logmoko throughput stays flat across thread counts — the MPMC ring buffer scales with producers.
fmtlog uses per-thread queues and degrades under contention on a single consumer. spdlog, g3log,
and quill show similar saturation at the single backend thread.

**Filtered call overhead — 10M DEBUG calls, logger level INFO:**

| Library | Cost |
|---|---|
| quill | ~0.25 ns/call |
| fmtlog | ~0.27 ns/call |
| g3log | ~0.94 ns/call |
| spdlog | ~1.07 ns/call |
| logmoko | ~1.54 ns/call |

All libraries are well under 2 ns/call for a filtered log — safe to leave debug logging compiled
in for production builds. quill and fmtlog achieve ~0.25 ns because their macros expand to a
single branch on a thread-local level flag with no other work. logmoko's slightly higher cost
comes from the additional `logger->log_level` check through a pointer indirection.

**Per-call enqueue latency — 100K logs, ring=100K (no drops):**

| Library | msg | p50 | p99 | p999 | max |
|---|---|---|---|---|---|
| logmoko | long | ~1 µs | ~1 µs | ~1 µs | ~19 µs |
| logmoko | short | <1 µs | ~1 µs | ~1 µs | ~3 µs |
| quill | both | <1 µs | <1 µs | ~1 µs | ~3 µs |
| fmtlog | both | <1 µs | <1 µs | ~1 µs | ~4 µs |
| spdlog | long | ~2 µs | ~3 µs | ~5 µs | ~37 µs |
| spdlog | short | <1 µs | ~1 µs | ~3 µs | ~11 µs |
| g3log | long | ~2 µs | ~4 µs | ~10 µs | ~54 µs |
| syslog | short | <1 µs | ~1 µs | ~3 µs | ~3,722 µs |

*Values below 1 µs appear as 0 (macOS `CLOCK_MONOTONIC` resolution). The syslog max of 3.7 ms
is a kernel scheduling outlier, not typical cost.*

**Short (~62 B) vs long (~2 KB) message throughput:**

| Library | Long | Short | Speedup |
|---|---|---|---|
| logmoko | ~1.4M/sec | ~3.3M/sec | 2.4× |
| fmtlog | ~700K/sec | ~4.4M/sec | 6.3× |
| spdlog | ~560K/sec | ~5.3M/sec | 9.5× |
| quill | ~370K/sec | ~4.6M/sec | 12× |
| g3log | ~245K/sec | ~750K/sec | 3.1× |

logmoko formats on the producer thread (vsnprintf + memcpy into the ring slot), so its throughput
scales with message length. quill and fmtlog defer formatting to the background thread; their
enqueue cost is nearly message-size-independent, but the backend serialization shows up in total
time. The result: logmoko has the smallest long-vs-short gap because its bottleneck is the
formatting step, not I/O.

**Drop behavior — 100K short-message burst:**

| Library | ring=256 | ring=1024 | ring=8192 | avg ns/call |
|---|---|---|---|---|
| logmoko | 63.5% dropped | 59.1% dropped | 55.4% dropped | ~81 ns |
| spdlog | 75.5% dropped | 75.3% dropped | 54.6% dropped | ~120 ns |
| quill | n/a (no counter) | n/a | n/a | ~4 ns |
| fmtlog | n/a (no counter) | n/a | n/a | ~16 ns |

quill and fmtlog enqueue in nanoseconds because they defer formatting to the background thread — the producer is unlikely to overflow the ring at all. logmoko and spdlog format on the producer thread, so a tight burst outpaces the consumer. To avoid drops, size the ring to absorb the burst peak or rate-limit producers.

**Drop-free threshold (ring=8192):** zero drops up to ~1M logs/sec; drops begin at ~1.5M logs/sec.

**Memory footprint — 1 file handler, ring=1024:**

| Library | Delta | Notes |
|---|---|---|
| logmoko | +2.2 MB | pre-allocates 2 KB × ring_sz slots at init |
| fmtlog | +0.7 MB | per-thread queues allocated on first log |
| spdlog | +0.5 MB | |
| quill | +0.3 MB | |
| g3log | +0.1 MB | |

logmoko's memory cost is predictable and scales linearly: `ring_buffer_size × 2 KB` per handler. At the default ring=1024 that is 2 MB; at ring=8192 it is 16 MB.

**Choosing a ring buffer size:**

The ring buffer size (`ring_buffer_size`, default 1024) controls burst absorption. Each slot holds
one formatted message (2 KB), so memory per handler is `ring_buffer_size × 2 KB`. Use powers of 2;
non-power-of-2 values are rounded up internally.

## Socket Handler

Loopback UDP, 50,000 logs per run.

**Throughput (unthrottled, consumer sendto-bound):**

| Mode | Message | Throughput |
|---|---|---|
| Plain | short (~62 B) | ~310K msg/sec |
| Plain | long (~580 B) | ~302K msg/sec |
| Encrypted (AES-256-GCM) | short | ~250K msg/sec |
| Encrypted (AES-256-GCM) | long | ~241K msg/sec |

Encryption adds ~20% overhead (~60K msg/sec). The bottleneck is the `sendto` syscall rate on the
consumer thread, not the AES-NI crypto. Message size has minimal impact because I/O dominates.

**Drop-free throughput ceiling (rate-limited, ring=65536):**

Both plain and encrypted handlers sustain up to **500K+ msg/sec** without drops when the producer
is rate-limited. The ring absorbs bursts; the consumer keeps up on average. At unthrottled burst
speeds (~310K plain) the ring fills faster than it drains, so drops occur.

**Drop behaviour — 50K short-message burst:**

| Ring size | Dropped | Drop % |
|---|---|---|
| 64 | ~49,200 | 98.4% |
| 256 | ~49,100 | 98.2% |
| 1,024 | ~48,300 | 96.5% |
| 8,192 | ~41,000 | 82.0% |

At unthrottled burst rates the single consumer thread cannot drain fast enough. Size the ring to
cover your burst window, or use `lmk_rate_limiter` to pace producers below the sendto ceiling.

**Per-call enqueue latency (ring=50K, no drops):**

| p50 | p95 | p99 | p999 | max |
|---|---|---|---|---|
| <1 µs | ~1 µs | ~1 µs | ~2 µs | ~15 µs |

The enqueue path (CAS + memcpy into ring slot) is sub-microsecond. The `sendto` syscall and any
encryption happen entirely on the background consumer thread.
