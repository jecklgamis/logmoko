#include "logmoko.h"
#include "logmoko_rate_limiter.h"

int main(int argc, char *argv[]) {
      lmk_init();

      struct lmk_logger *logger = lmk_get_logger("logger");

      struct lmk_log_handler *file = lmk_get_sync_file_log_handler("file", "example_app_sync_file.c.log");
      lmk_attach_log_handler(logger, file);


      struct lmk_rate_limiter rl;
      lmk_rate_limiter_init(&rl, 100000);

      int nr_logs = 2000000;
      for (int i = 0; i < nr_logs; i++) {
          LMK_LOG_INFO(logger, "This is an info log %d", i);
//          lmk_rate_limiter_wait(&rl);
      }

      lmk_destroy();
      return EXIT_SUCCESS;
}
