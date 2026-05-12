#include "logmoko.h"
#include "logmoko_rate_limiter.h"

int main(int argc, char *argv[]) {
    lmk_init();

    struct lmk_logger      *logger  = lmk_get_logger("socket-logger");
    struct lmk_log_handler *handler = lmk_get_socket_log_handler("socket-handler");

    lmk_set_socket_psk(handler, "some-psk");
    lmk_set_log_level(logger, LMK_LOG_LEVEL_INFO);
    lmk_attach_log_listener(handler, "127.0.0.1", 9000);
    lmk_attach_log_handler(logger, handler);

    struct lmk_rate_limiter rl;
    lmk_rate_limiter_init(&rl, 500000);

    for (int i = 0; i < 10000; i++) {
        LMK_LOG_INFO(logger, "This is an info log %d", i);
        lmk_rate_limiter_wait(&rl);
    }

    lmk_destroy();
    return EXIT_SUCCESS;
}
