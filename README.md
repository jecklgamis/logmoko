### Logmoko

[![build](https://github.com/jecklgamis/logmoko/actions/workflows/build.yaml/badge.svg)](https://github.com/jecklgamis/logmoko/actions/workflows/build.yaml)

Logmoko is a fast, lightweight async logging framework for C. It supports multiple log levels, named
loggers, pluggable handlers, asynchronous I/O, log rotation, syslog, INI-based configuration, and
AES-256-GCM encrypted UDP transport.

Most async handlers are non-blocking: each handler runs a background thread draining a ring buffer,
so the caller's cost is bounded to a CAS, a memcpy into the ring slot, and a conditional wakeup
signal. The `sync_file` handler is different: it uses a bounded buffered queue and blocks callers
when the queue is full, preserving log records instead of dropping them.

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
- **Handlers**: Console, File (with log rotation), Socket (UDP), Syslog, Sync File
- **Async I/O**: console, file, socket, and syslog handlers use background consumer threads with ring buffers; logs that arrive when those buffers are full are discarded rather than blocking the caller
- **Buffered sync file I/O**: `sync_file` uses a background consumer thread with a bounded queue; callers block when the queue is full, so accepted records are not dropped
- **Encrypted UDP transport**: AES-256-GCM encryption on the socket handler via a preshared key — enable with `lmk_set_socket_psk()` or the `psk` config key
- **Custom log format**: supply a function pointer to override the default log line format per handler
- **INI config file**: initialise the entire logging setup from a config file via `lmk_init_from_file()`
- **Rate limiter**: `logmoko_rate_limiter` utility for pacing log producers to a target rate

See [docs/comparison.md](docs/comparison.md) for a feature comparison with spdlog, quill, fmtlog, g3log, zlog, log.c, log4c, and syslog.

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
    lmk_init();

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

logmoko delivers ~1.2M logs/sec sustained throughput, ~1 µs p99 enqueue latency, flat scaling
across 1–8 producer threads, and ~1.5 ns/call overhead for filtered (non-emitting) log calls.

Each handler runs a dedicated background thread draining a lock-free ring buffer. The caller's cost
is a CAS on the ring head, a `memcpy` into the claimed slot, and a conditional wakeup (only when
the consumer is sleeping).

The ring buffer size (`ring_buffer_size`, default 1024) controls burst absorption. Each slot holds
one formatted message (2 KB), so memory per handler is `ring_buffer_size × 2 KB`. Use powers of 2.

The `sync_file` handler also uses `ring_buffer_size` as its queue depth, but unlike the async
drop-on-full handlers it applies backpressure: producers wait when the queue is full. Its consumer
batches formatted log lines into a 256 KB write buffer and flushes/fsyncs on log rotation and
shutdown. This gives higher throughput than fsync-per-call logging, with the tradeoff that a process
crash can lose records that are still in memory or stdio buffers.

See [docs/benchmark.md](docs/benchmark.md) for full benchmark results and framework comparisons.

### Buffered Sync File Handler

Use `lmk_get_sync_file_log_handler()` when every accepted log record should be preserved and the
caller should apply backpressure instead of dropping on overflow:

```c
struct lmk_logger *logger = lmk_get_logger("audit");
struct lmk_log_handler *audit =
    lmk_get_sync_file_log_handler("audit-file", "audit.log");

lmk_attach_log_handler(logger, audit);
LMK_LOG_INFO(logger, "payment accepted id=%s", payment_id);
```

The queue depth is `ring_buffer_size` rounded up to the next power of two. The default is 1024
records. File rotation uses the same defaults as the async file handler: 20 MB per file and 10
backups unless changed with `lmk_set_log_rotation()` or config.

### Encrypted UDP Transport

The socket handler supports AES-256-GCM encryption via a preshared key. The PSK string is hashed
with SHA-256 to produce a 32-byte key. Each UDP packet carries a fresh 12-byte random IV and a
16-byte authentication tag (28 bytes total overhead per packet).

**Programmatic:**
```c
struct lmk_log_handler *handler = lmk_get_socket_log_handler("remote");
lmk_set_socket_psk(handler, "my-secret-passphrase");
lmk_attach_log_listener(handler, "192.168.1.10", 9000);
```

**Config file:**
```ini
[handler:remote]
type     = socket
level    = info
listener = 192.168.1.10:9000
psk      = my-secret-passphrase
```

**Receiver** (`logmoko_log_receiver`) decrypts and prints incoming packets:
```sh
./logmoko_log_receiver -p "my-secret-passphrase" -b 0.0.0.0 -l 9000
```

### Socket Handler Performance

The socket handler sustains ~310K msg/sec plain and ~250K msg/sec encrypted (AES-256-GCM) on
loopback UDP. With rate-limiting, both modes reach 500K+ msg/sec without drops — the ring absorbs
bursts and the consumer keeps up on average. Per-call enqueue latency is sub-microsecond (p99 ~1 µs).

See [docs/benchmark.md](docs/benchmark.md) for full socket handler benchmark results.

### Contributing
Please see `CONTRIBUTING.md`
