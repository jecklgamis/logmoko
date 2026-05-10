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

my_app.c:
```c
#include "logmoko.h"

int main() {
    lmk_init();

    struct lmk_logger *logger = lmk_get_logger("logger");
    struct lmk_log_handler *console = lmk_get_console_log_handler();
    lmk_attach_log_handler(logger, console);

    struct lmk_log_handler *file = lmk_get_file_log_handler("file", "example_app.log");
    lmk_attach_log_handler(logger, file);

    lmk_set_log_level(logger, LMK_LOG_LEVEL_INFO);
    LMK_LOG_INFO(logger, "This is an info log");
    LMK_LOG_WARN(logger, "This is a warning log");
    LMK_LOG_ERROR(logger, "This is an error log");

    lmk_destroy();
    return EXIT_SUCCESS;
}
```

Link your program with the `logmoko` library
```
gcc -o my_app my_app.c -llogmoko
```
Example output:
```bash
[INFO Mon May 11 06:28:08 2026 (my_app.c:14)] : This is an info log
[WARN Mon May 11 06:28:08 2026 (my_app.c:15)] : This is a warning log
[ERROR Mon May 11 06:28:08 2026 (my_app.c:16)] : This is an error log
```


## Logmoko APIs

### Loggers
A logger is the main object used for logging requests. A logger object is passed to the logging macros (`LMK_LOG_XXX`)
when logging. Use `lmk_get_logger` to create or retrieve an existing logger. You can create multiple loggers with different names. 
 
API:
```
lmk_logger *lmk_get_logger(const char *name);
void lmk_set_log_level(lmk_logger *logger, int log_level);
int lmk_get_log_level(lmk_logger *logger);
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
As a general rule, if the logger has log level l, the request is logged , if u >= l. As an example, if the logger's 
log level is INFO, log requests with level INFO, WARN, and ERROR will be logged while DEBUG and TRACE  logs 
will be ignored.

To a log a message with specific log level, use the following API macros. These macros accept arguments similar to `fprintf`.

API
```
LMK_LOG_TRACE(logger, format...)
LMK_LOG_DEBUG(logger, format...)
LMK_LOG_INFO(logger, format...)
LMK_LOG_WARN(logger, format...)
LMK_LOG_INFO(logger, format...)
LMK_IS_LOG_ENABLED(logger_or_handler, level)
```

Example
```
LMK_LOG_INFO(logger, "This is an INFO log");
LMK_LOG_INFO(logger, "This is an INFO log with integer param : %d", 1024);
```

### Log Handlers
A logger uses one or more handlers to do the actual logging to a specific interface. Logmoko supports console
file, and socket log handler types. Log handlers are created with default log level set to `LMK_LOG_LEVEL_INFO`.

API
```
struct lmk_log_handler *lmk_get_console_log_handler();
struct lmk_log_handler *lmk_get_file_log_handler(const char *name, const char *filename);
struct lmk_log_handler *lmk_get_socket_log_handler(const char *name);
int lmk_attach_log_handler(lmk_logger *logger, lmk_log_handler *handler);
int lmk_detach_log_handler(lmk_logger *logger, lmk_log_handler *handler);
struct lmk_log_handler *lmk_get_socket_log_handler(const char* name);
void lmk_attach_log_listener(lmk_log_handler *handler, const char *host, int port);
void lmk_set_handler_log_level(lmk_log_handler *handler, int log_level);
int lmk_get_handler_log_level(lmk_log_handler *handler) ;
```

#### Console log handler example:
```
#include <stdlib.h>
#include <logmoko.h>

int main(int argc, char *argv[]) {
    lmk_logger *logger = lmk_get_logger("some-logger");
    lmk_attach_log_handler(logger, lmk_get_console_log_handler());
    LMK_LOG_INFO(logger, "this is an info log");
    lmk_destroy();
    return EXIT_SUCCESS;
}
```


#### File log handler example:
```
#include <stdlib.h>
#include <logmoko.h>

int main(int argc, char *argv[]) {
    lmk_logger *logger = lmk_get_logger("some-logger");
    lmk_log_handler *flh = lmk_get_file_log_handler("file-handler", "example_app.log");
    lmk_attach_log_handler(logger, flh);
    LMK_LOG_INFO(logger, "this is an info log");
    lmk_destroy();
    return EXIT_SUCCESS;
}
```

#### Socket log handler example:
```
#include <stdlib.h>
#include <logmoko.h>

int main(int argc, char *argv[]) {
    lmk_logger *logger;
    lmk_log_handler *handler;

    logger = lmk_get_logger("logger");
    handler = lmk_get_socket_log_handler("socket");
    lmk_attach_log_listener(handler, "127.0.0.1", 9000);
    lmk_attach_log_handler(logger, handler);

    LMK_LOG_INFO(logger, "this is an info log");

    lmk_destroy();
    return EXIT_SUCCESS;
}
```

A simple UDP socket listener can be setup using the `nc` utility . 
```
$ nc -l -u localhost 9000
```

### Log Format
The log output is fixed to the following format :
```
<log level> <timestamp> <filename>:<line number> <logger name> : <log message>
```

### Cleaning
Once you're done using the logger, you should call `lmk_destroy` to free up resources used by `logmoko`. 

API
```
void lmk_init();
void lmk_destroy();
void lmk_destroy_logger(lmk_logger **logger);
void lmk_destroy_handler(lmk_log_handler **handler);
void lmk_dump_loggers();
```

`lmk_init()` is implicitly called by API functions.