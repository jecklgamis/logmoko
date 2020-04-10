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

