### Logmoko

[![build](https://github.com/jecklgamis/logmoko/actions/workflows/build.yaml/badge.svg)](https://github.com/jecklgamis/logmoko/actions/workflows/build.yaml)

Logmoko is a fast, lightweight async logging framework for C. It supports multiple log levels, named
loggers, pluggable handlers, asynchronous I/O, log rotation, syslog, INI-based configuration, and
AES-256-GCM encrypted UDP transport.

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
- **Encrypted UDP transport**: AES-256-GCM encryption on the socket handler via a preshared key — enable with `lmk_set_socket_psk()` or the `psk` config key
- **Custom log format**: supply a function pointer to override the default log line format per handler
- **INI config file**: initialise the entire logging setup from a config file via `lmk_init_from_file()`
- **Rate limiter**: `logmoko_rate_limiter` utility for pacing log producers to a target rate

### Comparison with Other C/C++ Logging Frameworks

| Feature | logmoko | spdlog | quill | fmtlog | g3log | zlog | log.c | log4c | syslog |
|---|---|---|---|---|---|---|---|---|---|
| **Language** | C | C++ | C++ | C++ | C++ | C | C | C | C (POSIX) |
| **I/O model** | Async | Async | Async | Async | Async | Sync | Sync | Sync | Kernel IPC |
| **Named loggers** | Yes | Yes | Yes | No (global) | No (LogWorker) | Yes (categories) | No (global) | Yes (categories) | No |
| **Log levels** | TRACE–OFF (6) | trace–off (7) | trace–off (7) | DBG–OFF (5) | DEBUG/INFO/WARN/FATAL (4+) | DEBUG–FATAL (6) | TRACE–FATAL (6) | TRACE–FATAL (7) | emerg–debug (8) |
| **File output** | Yes | Yes | Yes | Yes | Yes | Yes | Yes | Yes | No |
| **Console output** | Yes | Yes | Yes | No | No | No | Yes (stderr) | Yes | No |
| **Syslog output** | Yes | Yes | No | No | No | Yes | No | Yes | Yes (is syslog) |
| **UDP/socket output** | Yes ¹ | No | No | No | No | No | No | No | No |
| **Log rotation** | Yes | Yes | No | No | No | Yes | No | Yes | No |
| **Custom format** | Yes (fn ptr) | Yes (pattern) | Yes (pattern) | No | No | Yes (config) | No | Yes (config) | No |
| **Config file** | Yes (INI) | No | No | No | No | Yes (zlog.conf) | No | Yes (XML/.props) | No |
| **Multiple sinks per logger** | Yes | Yes | Yes | No | No | Yes | No | Yes | No |
| **Drop counter** | Yes | Yes | No | No | No | N/A | N/A | N/A | N/A |
| **Header-only** | No | Optional | No | Yes (vendored) | No | No | Yes (vendored) | No | N/A |
| **Crash/fatal handler** | No | No | No | No | Yes | No | No | No | No |
| **Re-initializable** | Yes | Yes | Yes | Yes | Yes | Yes | Yes | No | Yes |
| **Transport encryption** | Yes (AES-256-GCM) | N/A | N/A | N/A | N/A | N/A | N/A | N/A | N/A |

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

See [docs/benchmark.md](docs/benchmark.md) for full benchmark results and framework comparisons.

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
