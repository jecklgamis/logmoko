#include "logmoko.h"

int main(int argc, char *argv[]) {
    lmk_init();
    struct lmk_logger *logger = lmk_get_logger("app");
    lmk_dump_loggers();
    LMK_LOG_INFO(logger, "This is an info log")
    lmk_destroy();
    return EXIT_SUCCESS;
}
