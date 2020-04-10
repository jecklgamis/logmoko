#include <stdlib.h>
#include <logmoko.h>

int main(int argc, char *argv[]) {
    lmk_logger *logger;
    lmk_log_handler *handler;

    /* Create a logger */
    logger = lmk_get_logger("logger");

    /* Create a socket log handler */
    handler = lmk_get_socket_log_handler("socket");

    /* Attach a remote log listener */
    lmk_attach_log_listener(handler, "127.0.0.1", 9000);

    /* Attach log handler to logger */
    lmk_attach_log_handler(logger, handler);

    /* Start logging! */
    for (int a = 0; a < 16; a++) {
        LMK_LOG_INFO(logger, "%d This is an info log", a);
    }

    /* Clean up */
    lmk_destroy();
    return EXIT_SUCCESS;
}