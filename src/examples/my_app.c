#include "logmoko.h"

int main(int argc, char **argv) {
    lmk_logger *logger = lmk_get_logger("mylogger");
    lmk_log_handler *clh = lmk_get_console_log_handler("mylogger-clh");
    lmk_log_handler *flh = lmk_get_file_log_handler("mylogger-flh", "my_app.log");

    lmk_attach_log_handler(logger, clh);
    lmk_attach_log_handler(logger, flh);

    LMK_LOG_TRACE(logger, "this is a trace log");
    LMK_LOG_DEBUG(logger, "this is a debug log");
    LMK_LOG_INFO(logger, "this is an info log");
    LMK_LOG_WARN(logger, "this is a warn log");
    LMK_LOG_ERROR(logger, "this is an error log");
    LMK_LOG_FATAL(logger, "this is a fatal log");

    lmk_destroy();
    return EXIT_SUCCESS;
}

