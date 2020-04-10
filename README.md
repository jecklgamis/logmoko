### Logmoko

Logmoko is a minimal logging framework for C.  It allows user to control which log statements are logged based on a 
configured log level and to which interface (e.g. console, file, remote connection).

### Getting Started

Installing `logmoko` from source:
```
$ git clone git@github.com:jecklgamis/logmoko.git
$ ./autogen.sh
$ ./configure
$ make check
$ make all
$ sudo make install
```
By default, this will install `lomogko.a` library in `/usr/local/lib` and relevant header files in `/usr/local/include`
directory.

### Example App   

[src/examples/example_app.c](./src/examples/example_app.c)
```
#include <stdlib.h>
#include <logmoko.h>

int main(int argc, char *argv[]) {
    /* Create a logger */
    lmk_logger *logger = lmk_get_logger("logger");

    /* Create a console handler */
    lmk_log_handler *console = lmk_get_console_log_handler();

    /* Create a file handler */
    lmk_log_handler *file = lmk_get_file_log_handler("file", "example_app.log");

    /* Attach handlers to logger */
    lmk_attach_log_handler(logger, console);
    lmk_attach_log_handler(logger, file);

    /* Start logging! */
    LMK_LOG_TRACE(logger, "This is a trace log");
    LMK_LOG_DEBUG(logger, "This is a debug log");
    LMK_LOG_INFO(logger, "This is an info log");
    LMK_LOG_WARN(logger, "This is a warn log");
    LMK_LOG_ERROR(logger, "This is an error log");
    LMK_LOG_FATAL(logger, "This is a fatal log");

    /* Clean up */
    lmk_destroy();
    return EXIT_SUCCESS;
}
```

Compiling the example app:
```
$ gcc -o example_app example_app.c -llogmoko
$ chmod +x ./example_app
```

Running the example app:
```
$ ./example_app
[TRACE Fri Apr 10 14:23:08 2020 (example_app.c:19)] : This is a trace log
[DEBUG Fri Apr 10 14:23:08 2020 (example_app.c:20)] : This is a debug log
[INFO  Fri Apr 10 14:23:08 2020 (example_app.c:21)] : This is an info log
[WARN  Fri Apr 10 14:23:08 2020 (example_app.c:22)] : This is a warn log
[ERROR Fri Apr 10 14:23:08 2020 (example_app.c:23)] : This is an error log
[FATAL Fri Apr 10 14:23:08 2020 (example_app.c:24)] : This is a fatal log
```

See the [Logmoko User Guide](docs/logmoko-user-guide.md) for the details on using `logmoko`.

### Contributing
Please raise issue or pull request. CONTRIBUTING.md coming up soon!





