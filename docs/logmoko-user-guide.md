## Logmoko User Guide

## Installing

### From a release
Download the latest release artifact from the [releases page](https://github.com/jecklgamis/logmoko/releases),
then extract and install:
```
$ tar xzf logmoko-<version>.tar.gz
$ cd logmoko-<version>
$ ./autogen.sh
$ ./configure
$ make all
$ sudo make install
```

### From source (main branch)
```
$ git clone git@github.com:jecklgamis/logmoko.git
$ cd logmoko
$ ./autogen.sh
$ ./configure
$ make check
$ make all
$ sudo make install
```

This installs `liblogmoko.a` in `/usr/local/lib` and headers in `/usr/local/include`.

## Example

[src/examples/example_app.c](../src/examples/example_app.c)
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

Build and run:
```
gcc -o example_app example_app.c -llogmoko
./example_app
```

Output:
```
[TRACE Mon May 11 06:42:10 2026 (example_app.c:21)] : This is a trace log
[DEBUG Mon May 11 06:42:10 2026 (example_app.c:22)] : This is a debug log
[INFO  Mon May 11 06:42:10 2026 (example_app.c:23)] : This is an info log
[WARN  Mon May 11 06:42:10 2026 (example_app.c:24)] : This is a warn log
[ERROR Mon May 11 06:42:10 2026 (example_app.c:25)] : This is an error log
```

## API Reference

### Initializing

```c
void lmk_init();
void lmk_destroy();
int  lmk_init_from_file(const char *path);
```

`lmk_init_from_file` with `NULL` auto-detects the config: checks the `LMK_CONFIG_FILE` environment
variable first, then falls back to `logmoko.conf` in the current directory.

### Loggers

Loggers filter and route log requests to one or more handlers. Created with a default level of `INFO`.

```c
struct lmk_logger *lmk_get_logger(const char *name);
void lmk_set_log_level(struct lmk_logger *logger, int log_level);
int  lmk_get_log_level(struct lmk_logger *logger);
```

Log levels (lowest to highest): `LMK_LOG_LEVEL_TRACE`, `LMK_LOG_LEVEL_DEBUG`, `LMK_LOG_LEVEL_INFO`,
`LMK_LOG_LEVEL_WARN`, `LMK_LOG_LEVEL_ERROR`, `LMK_LOG_LEVEL_OFF`.

A request is logged if its level ≥ the logger's level.

Logging macros (accept `printf`-style format strings):
```c
LMK_LOG_TRACE(logger, format...)
LMK_LOG_DEBUG(logger, format...)
LMK_LOG_INFO(logger, format...)
LMK_LOG_WARN(logger, format...)
LMK_LOG_ERROR(logger, format...)
```

### Log Handlers

Handlers perform the actual I/O. Each runs a background consumer thread with a ring buffer; logs
that arrive when the buffer is full are discarded rather than blocking the caller.

```c
struct lmk_log_handler *lmk_get_console_log_handler();
struct lmk_log_handler *lmk_get_file_log_handler(const char *name, const char *filename);
struct lmk_log_handler *lmk_get_socket_log_handler(const char *name);
struct lmk_log_handler *lmk_get_syslog_log_handler(const char *name, const char *ident, int facility);
int  lmk_attach_log_handler(struct lmk_logger *logger, struct lmk_log_handler *handler);
int  lmk_detach_log_handler(struct lmk_logger *logger, struct lmk_log_handler *handler);
void lmk_set_handler_log_level(struct lmk_log_handler *handler, int log_level);
```

All factory functions return `NULL` on failure (NULL name, allocation failure, thread creation
failure). `lmk_get_file_log_handler` also returns `NULL` if `filename` is NULL. Always check the
return value before attaching a handler.

#### Console handler
```c
struct lmk_log_handler *console = lmk_get_console_log_handler();
lmk_attach_log_handler(logger, console);
```

#### File handler
```c
struct lmk_log_handler *file = lmk_get_file_log_handler("file", "app.log");
lmk_attach_log_handler(logger, file);
```

#### Socket handler (UDP)
```c
struct lmk_log_handler *sock = lmk_get_socket_log_handler("socket");
lmk_attach_log_listener(sock, "127.0.0.1", 9000);
lmk_attach_log_handler(logger, sock);
```

Listen with `nc`:
```
$ nc -l -u localhost 9000
```

#### Syslog handler
```c
struct lmk_log_handler *sl = lmk_get_syslog_log_handler("syslog", "myapp", LOG_DAEMON);
lmk_attach_log_handler(logger, sl);
```

Logmoko levels map to syslog priorities:

| Logmoko | syslog |
|---|---|
| TRACE / DEBUG | `LOG_DEBUG` |
| INFO | `LOG_INFO` |
| WARN | `LOG_WARNING` |
| ERROR | `LOG_ERR` |

View output on macOS:
```
log show --predicate 'eventMessage contains "myapp"' --last 5m
```

On Linux:
```
journalctl -t myapp
```

### Log Rotation

File handlers rotate when the log file reaches a configurable size. Defaults: 20 MB, 10 backups.
Backup files are named `app.log.1`, `app.log.2`, …

```c
void lmk_set_log_rotation(struct lmk_log_handler *handler, long max_file_size, int max_backup_files);
```

Example:
```c
struct lmk_log_handler *file = lmk_get_file_log_handler("file", "app.log");
lmk_set_log_rotation(file, 50 * 1024 * 1024, 5);  /* 50 MB, keep 5 backups */
```

### Log Format

Default format:
```
[LEVEL  timestamp (file:line) handler] : message
```

**Named presets:**

| Name | Output |
|---|---|
| `default` | `[LEVEL  timestamp (file:line) handler] : message` |
| `simple`  | `timestamp [LEVEL] message` |
| `json`    | `{"timestamp":"...","level":"...","file":"...","line":N,"handler":"...","message":"..."}` |

```c
lmk_set_log_format(handler, lmk_get_format_fn("simple"));
lmk_set_log_format(handler, NULL);   /* restore default */
```

**Custom format function:**

Supply any function matching `lmk_format_fn`:
```c
int my_format(char *out, size_t out_size,
              int log_level, const char *level_str,
              const char *timestamp,
              const char *file_name, int line_no,
              const char *handler_name,
              const char *message) {
    return snprintf(out, out_size, "%s [%s] %s\n", timestamp, level_str, message);
}

lmk_set_log_format(handler, my_format);
```

### Thread Safety

All `LMK_LOG_*` calls are thread-safe. Multiple threads can log concurrently on the same logger
with no locking — each thread formats into a thread-local buffer before handing off to the
handler's lock-free ring buffer.

**Safe to do concurrently:** logging from any number of threads; attaching/detaching handlers from
different loggers simultaneously.

**Must be done before spawning logging threads:** creating loggers and attaching handlers. The
framework does not lock logger or handler lists during log calls.

**Dropped logs:** when a handler's ring buffer is full, the calling thread increments a `dropped`
counter and returns immediately. The count is printed to stderr when the handler is destroyed.

## Config File

Logmoko can be fully initialised from an INI config file:

```c
lmk_init_from_file("logmoko.conf");
struct lmk_logger *logger = lmk_get_logger("app");
LMK_LOG_INFO(logger, "Hello from config");
lmk_destroy();
```

**Schema:**

```ini
[global]
log_buffer_size  = 2048    ; per-log message buffer size in bytes (default: 2048)
ring_buffer_size = 1024    ; async ring buffer depth per handler (default: 1024)

[handler:<name>]
type             = console | file | socket | syslog
level            = trace | debug | info | warn | error | off
format           = default | simple | json
filename         = <path>          ; file handlers only (required)
max_file_size    = <bytes>         ; file handlers only (default: 20971520)
max_backup_files = <count>         ; file handlers only (default: 10)
listener         = <host>:<port>   ; socket handlers, repeatable
ident            = <string>        ; syslog handlers only
facility         = user | daemon | local0..local7  ; syslog handlers only

[logger:<name>]
level    = trace | debug | info | warn | error | off
handlers = <handler-name>[, ...]
```

**Full example (`logmoko.conf`):**

```ini
[global]
log_buffer_size  = 2048
ring_buffer_size = 1024

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
