#include <openssl/rand.h>
#include "logmoko.h"
#include "logmoko_encryption.h"

static void random_psk(char *buf, size_t len) {
    static const char alphabet[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    unsigned char rnd[64];
    size_t n = len - 1 < sizeof(rnd) ? len - 1 : sizeof(rnd);
    RAND_bytes(rnd, (int)n);
    for (size_t i = 0; i < n; i++)
        buf[i] = alphabet[rnd[i] % (sizeof(alphabet) - 1)];
    buf[n] = '\0';
}

int main(int argc, char *argv[]) {
    lmk_init();

    struct lmk_logger *logger = lmk_get_logger("socket-logger");
    struct lmk_log_handler *handler = lmk_get_socket_log_handler("socket-handler");

    lmk_set_socket_psk(handler, "some-psk");

    lmk_set_log_level(logger, LMK_LOG_LEVEL_INFO);
    lmk_attach_log_listener(handler, "127.0.0.1", 9000);
    lmk_attach_log_handler(logger, handler);

    for(int a=0;a < 10000;a++) {
        LMK_LOG_INFO(logger, "This is an info log %d", a);
    }

    lmk_destroy();
    return EXIT_SUCCESS;
}
