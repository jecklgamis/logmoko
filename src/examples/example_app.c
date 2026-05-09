#include "logmoko.h"

int main() {
    lmk_init();

    struct lmk_logger *logger = lmk_get_logger("logger");
    lmk_set_log_level(logger, LMK_LOG_LEVEL_INFO);

    struct lmk_log_handler *console = lmk_get_console_log_handler();
    lmk_attach_log_handler(logger, console);

    struct lmk_log_handler *file = lmk_get_file_log_handler("file", "example_app.log");
    lmk_attach_log_handler(logger, file);

    LMK_LOG_INFO(logger, "Log message %d", 1);
    LMK_LOG_ERROR(logger, "Log message %d", 2);

    lmk_destroy();
    return EXIT_SUCCESS;
}
