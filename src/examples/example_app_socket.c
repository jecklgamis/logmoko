#include "logmoko.h"

int main() {
    lmk_init();

    struct lmk_logger *logger = lmk_get_logger("socket-logger");
    struct lmk_log_handler *handler = lmk_get_socket_log_handler("socket-handler");

    lmk_set_log_level(logger, LMK_LOG_LEVEL_INFO);
    lmk_attach_log_listener(handler, "127.0.0.1", 9000);
    lmk_attach_log_handler(logger, handler);

    for(int a=0;a < 10000;a++) {
        LMK_LOG_INFO(logger, "This is an info log %d", a);
    }

    lmk_destroy();
    return 0;
}
