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
