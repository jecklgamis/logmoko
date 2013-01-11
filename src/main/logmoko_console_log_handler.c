/*
 * Logmoko - A logging framework for C.
 * Copyright (C) 2013 Jerrico Gamis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "logmoko.h"

/**
 *  @brief Log handler implementation
 *  @param[in] param  Log data (LMK_LOG_REC)
 */

void lmk_console_log_handler_log_impl(lmk_log_handler *handler, void *param) {
    lmk_log_record *log_rec = NULL;
    const char *level_str = NULL;
    char timestamp[LMK_TSTAMP_BUFF_SIZE];
    LMK_LOCK_MUTEX(handler->lock);
    log_rec = (lmk_log_record*) param;
    level_str = lmk_get_log_level_str(log_rec->log_level);
    lmk_get_timestamp(timestamp, LMK_TSTAMP_BUFF_SIZE);
    fprintf(stdout, "[%-5s %s (%s:%d)] : %s\n", level_str, timestamp,
            log_rec->file_name, log_rec->line_no, log_rec->data);
    LMK_UNLOCK_MUTEX(handler->lock);
}

/**
 *  @brief Destroys the log handler
 *  @param[in] param  Set to null value
 */

void lmk_console_log_handler_destroy(lmk_log_handler *handler, void *param) {
    //nop
}

/**
 *  @brief Initializes log handler
 *  @param[in] param  Set to null value
 */

void lmk_console_log_handler_init(lmk_log_handler *handler, void *param) {
    //nop
}

