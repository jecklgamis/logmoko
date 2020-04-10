
#include "logmoko.h"

void lmk_serial_log_handler_init(lmk_log_handler *handler, void *param) {
    lmk_serial_log_handler *slh;
    char ts_buff[LMK_TSTAMP_BUFF_SIZE];
    LMK_LOCK_MUTEX(handler->lock);
    LMK_UNLOCK_MUTEX(handler->lock);

}

void lmk_serial_log_handler_log_impl(lmk_log_handler *handler, void *param) {
    const char *level_str = NULL;
    lmk_log_record *log_rec = NULL;
    lmk_serial_log_handler *slh = NULL;
    LMK_LOCK_MUTEX(handler->lock);
    LMK_UNLOCK_MUTEX(handler->lock);

}

void lmk_serial_log_handler_destroy(lmk_log_handler *handler, void *param) {
    char ts_buff[LMK_TSTAMP_BUFF_SIZE];
    lmk_serial_log_handler *slh = NULL;
    LMK_LOCK_MUTEX(handler->lock);
    LMK_UNLOCK_MUTEX(handler->lock);

}

