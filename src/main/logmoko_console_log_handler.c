#include "logmoko.h"

void lmk_console_log_handler_log_impl(lmk_log_handler *handler, void *param) {
    lmk_log_record *log_rec = NULL;
    const char *level_str = NULL;
    char timestamp[LMK_TSTAMP_BUFF_SIZE];

    LMK_LOCK_MUTEX(handler->lock);
    log_rec = (lmk_log_record *) param;
    level_str = lmk_get_log_level_str(log_rec->log_level);
    lmk_get_timestamp(timestamp, LMK_TSTAMP_BUFF_SIZE);
    fprintf(stdout, "[%-5s %s (%s:%d)] : %s\n", level_str, timestamp, log_rec->file_name, log_rec->line_no,
            log_rec->data);
    LMK_UNLOCK_MUTEX(handler->lock);
}

