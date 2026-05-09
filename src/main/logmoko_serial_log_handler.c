
#include "logmoko.h"

void lmk_serial_log_handler_init(struct lmk_log_handler *handler, void *param) {
    struct lmk_serial_log_handler *slh;
    char ts_buff[LMK_TSTAMP_BUFF_SIZE];
    LMK_LOCK_MUTEX(handler->lock);
    LMK_UNLOCK_MUTEX(handler->lock);

}

void lmk_serial_log_handler_log_impl(struct lmk_log_handler *handler, void *param) {
    const char *level_str = NULL;
    struct lmk_log_record *log_rec = NULL;
    struct lmk_serial_log_handler *slh = NULL;
    LMK_LOCK_MUTEX(handler->lock);
    LMK_UNLOCK_MUTEX(handler->lock);

}

void lmk_serial_log_handler_destroy(struct lmk_log_handler *handler, void *param) {
    char ts_buff[LMK_TSTAMP_BUFF_SIZE];
    struct lmk_serial_log_handler *slh = NULL;
    LMK_LOCK_MUTEX(handler->lock);
    LMK_UNLOCK_MUTEX(handler->lock);

}

