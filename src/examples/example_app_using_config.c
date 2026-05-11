#include "logmoko.h"

int main(int argc, char *argv[]) {
    const char *config_file = argc > 1 ? argv[1] : NULL;

    if (lmk_init_from_file(config_file) != LMK_E_OK) {
        fprintf(stderr, "Failed to load config\n");
        return EXIT_FAILURE;
    }

    struct lmk_logger *logger = lmk_get_logger("app");

    LMK_LOG_TRACE(logger, "This is a trace log");
    LMK_LOG_DEBUG(logger, "This is a debug log");
    LMK_LOG_INFO(logger, "This is an info log");
    LMK_LOG_WARN(logger, "This is a warn log");
    LMK_LOG_ERROR(logger, "This is an error log");

    lmk_destroy();
    return EXIT_SUCCESS;
}
