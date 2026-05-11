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

## Example

example_app.c:
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

Build and run the app:
```
gcc -o example_app example_app.c -llogmoko
chmod +x ./example_app
./example_app
```
Example output:
```
[TRACE Mon May 11 06:42:10 2026 (example_app.c:21)] : This is a trace log
[DEBUG Mon May 11 06:42:10 2026 (example_app.c:22)] : This is a debug log
[INFO  Mon May 11 06:42:10 2026 (example_app.c:23)] : This is an info log
[WARN  Mon May 11 06:42:10 2026 (example_app.c:24)] : This is a warn log
[ERROR Mon May 11 06:42:10 2026 (example_app.c:25)] : This is an error log
```


## Logmoko APIs

### Initializing

```c
void lmk_init();
void lmk_destroy();
```

To initialise from a config file:
```c
int lmk_init_from_file(const char *path);
```

Passing `NULL` auto-detects the config: checks the `LMK_CONFIG_FILE` environment variable first,
then falls back to `logmoko.conf` in the current directory.


### Loggers
The logger is the main object used for logging requests. A logger object is passed to the logging macros (`LMK_LOG_XXX`)
when logging. Use `lmk_get_logger` to create or retrieve an existing logger. You can create multiple loggers with different names.
Loggers are created with a default log level of `LMK_LOG_LEVEL_INFO`.

API:
```c
struct lmk_logger *lmk_get_logger(const char *name);
void lmk_set_log_level(struct lmk_logger *logger, int log_level);
int  lmk_get_log_level(struct lmk_logger *logger);
```

`log_level` is one of:
```
LMK_LOG_LEVEL_TRACE
LMK_LOG_LEVEL_DEBUG
LMK_LOG_LEVEL_INFO
LMK_LOG_LEVEL_WARN
LMK_LOG_LEVEL_ERROR
LMK_LOG_LEVEL_OFF
```

A logger can be assigned a log level that is used to filter out log requests. Below is the hierarchy of the log levels.
```
TRACE < DEBUG < INFO < WARN < ERROR
```
If the logger has log level `l`, a request is logged if its level `u >= l`. For example, if the logger's
log level is `INFO`, requests with level `INFO`, `WARN`, and `ERROR` are logged while `DEBUG` and `TRACE`
are ignored.

To log a message at a specific level, use the following macros. They accept arguments similar to `fprintf`.

```c
LMK_LOG_TRACE(logger, format...)
LMK_LOG_DEBUG(logger, format...)
LMK_LOG_INFO(logger, format...)
LMK_LOG_WARN(logger, format...)
LMK_LOG_ERROR(logger, format...)
LMK_IS_LOG_ENABLED(logger_or_handler, level)
```

Example:
```c
LMK_LOG_INFO(logger, "This is an info log");
LMK_LOG_INFO(logger, "This is an info log with integer param : %d", 1024);
```

### Log Handlers
A logger uses one or more handlers to do the actual logging to a specific interface. Logmoko supports console,
file, and socket log handler types. Log handlers are created with a default log level of `LMK_LOG_LEVEL_TRACE`.

Each handler runs a background consumer thread with a fixed-size ring buffer. If the buffer is full when a log
request arrives, the request is discarded rather than blocking the caller.

API:
```c
struct lmk_log_handler *lmk_get_console_log_handler();
struct lmk_log_handler *lmk_get_file_log_handler(const char *name, const char *filename);
struct lmk_log_handler *lmk_get_socket_log_handler(const char *name);
int  lmk_attach_log_handler(struct lmk_logger *logger, struct lmk_log_handler *handler);
int  lmk_detach_log_handler(struct lmk_logger *logger, struct lmk_log_handler *handler);
void lmk_attach_log_listener(struct lmk_log_handler *handler, const char *host, int port);
void lmk_set_handler_log_level(struct lmk_log_handler *handler, int log_level);
int  lmk_get_handler_log_level(struct lmk_log_handler *handler);
```

#### Console log handler example:
```c
#include "logmoko.h"

int main(int argc, char *argv[]) {
    lmk_init();
    struct lmk_logger *logger = lmk_get_logger("some-logger");
    lmk_attach_log_handler(logger, lmk_get_console_log_handler());
    LMK_LOG_INFO(logger, "this is an info log");
    lmk_destroy();
    return EXIT_SUCCESS;
}
```

#### File log handler example:
```c
#include "logmoko.h"

int main(int argc, char *argv[]) {
    lmk_init();
    struct lmk_logger *logger = lmk_get_logger("some-logger");
    struct lmk_log_handler *flh = lmk_get_file_log_handler("file-handler", "example_app.log");
    lmk_attach_log_handler(logger, flh);
    LMK_LOG_INFO(logger, "this is an info log");
    lmk_destroy();
    return EXIT_SUCCESS;
}
```

#### Socket log handler example:
```c
#include "logmoko.h"

int main(int argc, char *argv[]) {
    lmk_init();
    struct lmk_logger *logger  = lmk_get_logger("logger");
    struct lmk_log_handler *handler = lmk_get_socket_log_handler("socket");
    lmk_attach_log_listener(handler, "127.0.0.1", 9000);
    lmk_attach_log_handler(logger, handler);
    LMK_LOG_INFO(logger, "this is an info log");
    lmk_destroy();
    return EXIT_SUCCESS;
}
```

A simple UDP socket listener can be set up using the `nc` utility:
```
$ nc -l -u localhost 9000
```

### Log Rotation

File handlers rotate the log file when it reaches a configurable size. Backup files are named
`app.log.1`, `app.log.2`, … up to the configured maximum. Defaults are 20 MB and 10 backup files.

API:
```c
void lmk_set_log_rotation(struct lmk_log_handler *handler, long max_file_size, int max_backup_files);
```

Example:
```c
struct lmk_log_handler *file = lmk_get_file_log_handler("file", "app.log");
lmk_set_log_rotation(file, 50 * 1024 * 1024, 5);  /* 50 MB, keep 5 backups */
```

### Log Format

The default log line format is:
```
[LEVEL  timestamp (file:line) handler] : message
```

#### Named presets

| Name | Output |
|---|---|
| `default` | `[LEVEL  timestamp (file:line) handler] : message` |
| `simple`  | `timestamp [LEVEL] message` |
| `json`    | `{"timestamp":"...","level":"...","file":"...","line":N,"handler":"...","message":"..."}` |

Select a preset via the API:
```c
lmk_set_log_format(handler, lmk_get_format_fn("simple"));
```

Restore the default:
```c
lmk_set_log_format(handler, NULL);
```

#### Custom format function

Supply any function matching `lmk_format_fn`:
```c
void my_format(char *out, size_t out_size,
               int log_level, const char *level_str,
               const char *timestamp,
               const char *file_name, int line_no,
               const char *handler_name,
               const char *message) {
    snprintf(out, out_size, "%s [%s] %s\n", timestamp, level_str, message);
}

lmk_set_log_format(handler, my_format);
```

### Config File

Logmoko can be initialised entirely from an INI config file.

```c
lmk_init_from_file("logmoko.conf");
```

Config file format:
```ini
[global]
log_buffer_size  = 2048    ; per-log message buffer size in bytes
ring_buffer_size = 256     ; async ring buffer depth per handler

[handler:<name>]
type             = console | file | socket
level            = trace | debug | info | warn | error | off
format           = default | simple | json
filename         = <path>          ; file handlers only
max_file_size    = <bytes>         ; file handlers only (default: 20971520)
max_backup_files = <count>         ; file handlers only (default: 10)
listener         = <host>:<port>   ; socket handlers, repeatable

[logger:<name>]
level    = trace | debug | info | warn | error | off
handlers = <handler-name>[, ...]
```

Full example (`logmoko.conf`):
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

[logger:app]
level    = info
handlers = console, file, remote
```
