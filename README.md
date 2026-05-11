### Logmoko

[![build](https://github.com/jecklgamis/logmoko/actions/workflows/build.yaml/badge.svg)](https://github.com/jecklgamis/logmoko/actions/workflows/build.yaml)

Logmoko is a fast, lightweight async logging framework for C. It supports multiple log levels, named
loggers, pluggable handlers, asynchronous I/O, log rotation, syslog, and INI-based configuration.

Log calls never block — each handler runs a background thread draining a ring buffer, so the caller's
cost is bounded to a CAS, a memcpy into the ring slot, and a conditional wakeup signal.

Tested on:

| Distro | libc |
|---|---|
| Ubuntu 24.04 | glibc |
| Alpine 3.21 | musl |
| Fedora 41 | glibc |
| macOS 14 (Apple Silicon) | libSystem |

### Features

- **Log levels**: TRACE, DEBUG, INFO, WARN, ERROR, OFF — configurable per logger and per handler
- **Named loggers**: create and retrieve loggers by name; multiple loggers can share handlers
- **Handlers**: Console, File (with log rotation), Socket (UDP), Syslog
- **Async I/O**: each handler runs a background consumer thread with a ring buffer; logs that arrive when the buffer is full are discarded rather than blocking the caller
- **Custom log format**: supply a function pointer to override the default log line format per handler
- **INI config file**: initialise the entire logging setup from a config file via `lmk_init_from_file()`

### Getting Started

```
$ git clone git@github.com:jecklgamis/logmoko.git
$ ./autogen.sh
$ ./configure
$ make check
$ make all
$ sudo make install
```

See the [Logmoko User Guide](docs/logmoko-user-guide.md) for full API reference, config file format, and handler examples.

### Example App

[src/examples/example_app.c](./src/examples/example_app.c)
```c
#include "logmoko.h"

int main(int argc, char *argv[]) {
    struct lmk_logger *logger = lmk_get_logger("logger");

    struct lmk_log_handler *console = lmk_get_console_log_handler();
    struct lmk_log_handler *file    = lmk_get_file_log_handler("file", "example_app.log");

    lmk_attach_log_handler(logger, console);
    lmk_attach_log_handler(logger, file);

    lmk_set_log_level(logger, LMK_LOG_LEVEL_TRACE);

    LMK_LOG_TRACE(logger, "This is a trace log");
    LMK_LOG_DEBUG(logger, "This is a debug log");
    LMK_LOG_INFO(logger, "This is an info log");
    LMK_LOG_WARN(logger, "This is a warn log");
    LMK_LOG_ERROR(logger, "This is an error log");

    lmk_destroy();
    return EXIT_SUCCESS;
}
```

### Performance

Each handler runs a dedicated background thread draining a lock-free ring buffer. The caller's cost
is a CAS on the ring head, a `memcpy` into the claimed slot, and a conditional wakeup (only when
the consumer is sleeping). The file handler batches formatted lines into a staging buffer and issues
a single `fwrite` per drain pass.

Benchmarked on Apple M-series (macOS 14, `-O2`), 100,000 INFO logs to a file handler.

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

**Drop-free threshold (ring=8192):** zero drops up to ~1M logs/sec; drops begin at ~1.5M logs/sec.

**Choosing a ring buffer size:**

The ring buffer size (`ring_buffer_size`, default 1024) controls burst absorption. Each slot holds
one formatted message (2 KB), so memory per handler is `ring_buffer_size × 2 KB`. Use powers of 2;
non-power-of-2 values are rounded up internally.

Test machine: Apple M-series, macOS 14, `-O2`.

### Contributing
Please see `CONTRIBUTING.md`
