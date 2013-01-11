/*
 * Logmoko - A logging framework for C.
 * Copyright (C) 2012 Jerrico Gamis
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
 *  @brief Initializes the log handler
 *  @param[in] param  Arbitrary parameter set by framework
 */

void lmk_serial_log_handler_init(lmk_log_handler *handler, void *param) {
    lmk_serial_log_handler *slh;
    char ts_buff[LMK_TSTAMP_BUFF_SIZE];
    LMK_LOCK_MUTEX(handler->lock);
    LMK_UNLOCK_MUTEX(handler->lock);

}

/**
 *  @brief Log handler implementation
 *  @param[in] param  Log data (LMK_LOG_REC)
 */

void lmk_serial_log_handler_log_impl(lmk_log_handler *handler, void *param) {
    const char *level_str = NULL;
    lmk_log_record *log_rec = NULL;
    lmk_serial_log_handler *slh = NULL;
    LMK_LOCK_MUTEX(handler->lock);
    LMK_UNLOCK_MUTEX(handler->lock);

}

/**
 *  @brief Destroys the log handler
 *  @param[in] param  Set to NULL value
 */
void lmk_serial_log_handler_destroy(lmk_log_handler *handler, void *param) {
    char ts_buff[LMK_TSTAMP_BUFF_SIZE];
    lmk_serial_log_handler *slh = NULL;
    LMK_LOCK_MUTEX(handler->lock);
    LMK_UNLOCK_MUTEX(handler->lock);

}

