### Logmoko

[![build](https://github.com/jecklgamis/logmoko/actions/workflows/build.yaml/badge.svg)](https://github.com/jecklgamis/logmoko/actions/workflows/build.yaml)

Logmoko is a logging framework for C. It supports multiple log levels, named loggers, pluggable handlers,
asynchronous I/O, log rotation, syslog, and INI-based configuration.

Tested on:

| Distro | libc |
|---|---|
| Ubuntu 24.04 | glibc |
| Alpine 3.21 | musl |
| Fedora 41 | glibc |
| macOS 14 (Apple Silicon) | libSystem |

### Contents

- [Features](#features)
- [Getting Started](#getting-started)
- [Example App](#example-app)
- [Config File Example](#config-file-example)
- [Custom Log Format](#custom-log-format)
- [Syslog Handler](#syslog-handler)
- [Log Rotation](#log-rotation)
- [Thread Safety](#thread-safety)
- [Performance](#performance)

### Features

- **Log levels**: TRACE, DEBUG, INFO, WARN, ERROR, OFF — configurable per logger and per handler
- **Named loggers**: create and retrieve loggers by name; multiple loggers can share handlers
- **Handlers**:
  - Console — writes to stdout
  - File — writes to a file; supports log rotation
  - Socket — sends log records over UDP to one or more remote listeners
  - Syslog — forwards to the system logger via `syslog(3)`; configurable ident and facility
- **Log rotation**: file handlers rotate when the log file reaches a configurable size (default 20 MB), keeping up to a configurable number of backup files (default 10)
- **Async I/O**: each handler runs a background consumer thread with a ring buffer; logs that arrive when the buffer is full are discarded rather than blocking the caller
- **Custom log format**: supply a function pointer to override the default log line format per handler
- **INI config file**: initialise the entire logging setup from a config file via `lmk_init_from_file()` or auto-detected from `LMK_CONFIG_FILE` env var / `logmoko.conf`

### Getting Started

Installing from source:
```
$ git clone git@github.com:jecklgamis/logmoko.git
$ ./autogen.sh
$ ./configure
$ make check
$ make all
$ sudo make install
```
This installs `liblogmoko.a` in `/usr/local/lib` and the header files in `/usr/local/include`.

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

### Config File Example

[src/examples/logmoko.conf](./src/examples/logmoko.conf)
```ini
[global]
log_buffer_size  = 2048
ring_buffer_size = 256

[handler:console]
type   = console
level  = trace
format = json

[handler:file]
type             = file
level            = trace
filename         = app.log
max_file_size    = 104857600
max_backup_files = 10
format           = simple

[handler:remote]
type     = socket
level    = trace
listener = 127.0.0.1:9000
format   = json

[handler:syslog]
type     = syslog
level    = info
ident    = myapp
facility = daemon

[logger:app]
level    = info
handlers = console, file, remote, syslog
```

Initialise from a config file:
```c
lmk_init_from_file("logmoko.conf");
struct lmk_logger *logger = lmk_get_logger("app");
LMK_LOG_INFO(logger, "Hello from config");
lmk_destroy();
```

### Custom Log Format

By default logmoko formats log lines as:
```
[LEVEL timestamp (file:line) handler] : message
```

**Named presets** — select a built-in format by name via the API or INI config:

| Name | Output |
|---|---|
| `default` | `[LEVEL  timestamp (file:line) handler] : message` |
| `simple`  | `timestamp [LEVEL] message` |
| `json`    | `{"timestamp":"...","level":"...","file":"...","line":N,"handler":"...","message":"..."}` |

```c
struct lmk_log_handler *console = lmk_get_console_log_handler();
lmk_set_log_format(console, lmk_get_format_fn("simple"));
```

Or set it in the config file:
```ini
[handler:console]
type   = console
format = simple
```

Pass `NULL` to restore the default format:
```c
lmk_set_log_format(console, NULL);
```

**Custom format function** — supply any function matching `lmk_format_fn`:
```c
void my_format(char *out, size_t out_size,
               int log_level, const char *level_str,
               const char *timestamp,
               const char *file_name, int line_no,
               const char *handler_name,
               const char *message) {
    snprintf(out, out_size, "%s [%s] %s\n", timestamp, level_str, message);
}

lmk_set_log_format(console, my_format);
```

### Syslog Handler

Forward logs to the system logger:
```c
struct lmk_log_handler *sl = lmk_get_syslog_log_handler("syslog", "myapp", LOG_DAEMON);
lmk_attach_log_handler(logger, sl);
```

Logmoko levels map to syslog priorities as follows:

| Logmoko | syslog |
|---|---|
| TRACE / DEBUG | `LOG_DEBUG` |
| INFO | `LOG_INFO` |
| WARN | `LOG_WARNING` |
| ERROR | `LOG_ERR` |

On macOS, view output with:
```
log show --predicate 'eventMessage contains "myapp"' --last 5m
```

On Linux:
```
journalctl -t myapp
```

### Log Rotation

Log rotation is enabled by default for file handlers (20 MB / 10 backups). Override at runtime:
```c
struct lmk_log_handler *file = lmk_get_file_log_handler("file", "app.log");
lmk_set_log_rotation(file, 50 * 1024 * 1024, 5);  /* 50 MB, keep 5 backups */
```
Backup files are named `app.log.1`, `app.log.2`, …, `app.log.N`.

See the [Logmoko User Guide](docs/logmoko-user-guide.md) for more details.

### Thread Safety

All public API functions are thread-safe. Multiple threads can call `LMK_LOG_*` concurrently on
the same logger; each logger serialises access to its log buffer with a mutex. Handlers are
protected by their own independent mutex, so multiple loggers can fan out to the same handler
concurrently without data races.

**What is safe to do concurrently:**
- Logging via `LMK_LOG_*` on any logger from any number of threads.
- Attaching and detaching handlers from different loggers simultaneously.

**What must be done before spawning logging threads:**
- Create loggers and attach handlers — the framework does not lock the logger or handler lists
  during log calls, so structural changes (create/destroy logger or handler) should be done
  before threads start logging.

**Dropped logs:** each handler's ring buffer absorbs bursts; when full, the calling thread
increments the handler's `dropped` counter and returns immediately rather than blocking. The
dropped count is printed to stderr when the handler is destroyed.

### Performance

Logmoko is designed so that log calls never block the application. Each handler runs a dedicated background thread that drains a ring buffer and performs all I/O, so the caller's cost is bounded to a mutex lock, a `strncpy` into the ring buffer slot, a condition signal, and an unlock.

Benchmarked on Apple M-series (100,000 INFO logs to a file handler, `-O2`):

| Scenario | Time | Throughput |
|---|---|---|
| Enqueue latency (caller-side only) | ~7 ms | ~14M enqueue attempts/sec |
| Full throughput — all logs written and flushed to disk | ~66 ms | ~1.5M logs/sec |

The file handler flushes once per drained batch rather than per line, which is the primary driver of write throughput under load.

**Choosing a ring buffer size:**

The ring buffer size (`ring_buffer_size`, default 1024) controls how much burst each handler can absorb before dropping. Logs that arrive when the buffer is full are silently discarded rather than blocking the caller. Each slot holds one formatted message (2 KB), so memory per handler is `ring_buffer_size × 2 KB`.

A sweep across buffer sizes against a 100K-log burst shows the tradeoff clearly:

| `ring_buffer_size` | Memory/handler | Written (burst) |
|---|---|---|
| 256 | 512 KB | ~7% |
| 1024 *(default)* | 2 MB | ~9% |
| 4096 | 8 MB | ~9% |
| 32768 | 64 MB | ~39% |
| 100000 | 195 MB | 100% |

The low write percentages in that table reflect a pathological benchmark — 100K logs fired as fast as possible with no work between them. In a real application the producer is doing actual work between log calls, giving the consumer continuous idle time to drain. At a typical sustained rate the default buffer sees near-zero drops.

- **Default (1024)** is a good fit for most workloads.
- **If you know your burst size**, set `ring_buffer_size` to match it. The buffer will absorb the full burst and the background thread will drain it after the burst subsides.
- **If you need zero drops under any conditions**, increase the buffer to match your worst-case burst, or use synchronous I/O instead.

**Sustained throughput limits by ring buffer size:**

Benchmarked on Linux (`-O2`), sustained rate-limited workload (200,000 INFO logs, zero drops threshold):

| `ring_buffer_size` | Memory/handler | Max sustained rate (zero drops) |
|---|---|---|
| 1024 *(default)* | 2 MB | ~300k logs/sec |
| 2048 | 4 MB | ~500k logs/sec |
| 4096 | 8 MB | ~600k logs/sec |

Above ~600–700k logs/sec, drops occur regardless of ring buffer size — that ceiling is the flush thread's disk write throughput, not the buffer size. Use powers of 2 for `ring_buffer_size`; non-power-of-2 values incur integer division overhead in the hot path and perform worse.

Test machine:
- **CPU**: Intel Core i7-4770 @ 3.40GHz, 4 cores / 8 threads, 8 MiB L3 cache
- **Memory**: 16 GB
- **Disk**: SSD


### Contributing
Please see `CONTRIBUTING.md`
