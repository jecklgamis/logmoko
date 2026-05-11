### Logmoko

![build](https://github.com/jecklgamis/logmoko/actions/workflows/build.yaml/badge.svg)

Logmoko is a minimal logging framework for C. It supports multiple log levels, named loggers, pluggable handlers,
asynchronous I/O, log rotation, and INI-based configuration.

### Features

- **Log levels**: TRACE, DEBUG, INFO, WARN, ERROR, OFF — configurable per logger and per handler
- **Named loggers**: create and retrieve loggers by name; multiple loggers can share handlers
- **Handlers**:
  - Console — writes to stdout
  - File — writes to a file; supports log rotation
  - Socket — sends log records over UDP to one or more remote listeners
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
type  = console
level = trace

[handler:file]
type             = file
level            = trace
filename         = app.log
max_file_size    = 104857600
max_backup_files = 10

[handler:remote]
type     = socket
level    = trace
listener = 127.0.0.1:9000

[logger:app]
level    = info
handlers = console, file, remote
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

### Log Rotation

Log rotation is enabled by default for file handlers (20 MB / 10 backups). Override at runtime:
```c
struct lmk_log_handler *file = lmk_get_file_log_handler("file", "app.log");
lmk_set_log_rotation(file, 50 * 1024 * 1024, 5);  /* 50 MB, keep 5 backups */
```
Backup files are named `app.log.1`, `app.log.2`, …, `app.log.N`.

See the [Logmoko User Guide](docs/logmoko-user-guide.md) for more details.

### Contributing
Please see `CONTRIBUTING.md`
