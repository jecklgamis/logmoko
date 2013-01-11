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
 *  @brief Initializes the log handler
 *  @param[in] param  Arbitrary parameter set by framework
 */

void lmk_file_log_handler_init(lmk_log_handler *handler, void *param) {
    lmk_file_log_handler *flh;
    char ts_buff[LMK_TSTAMP_BUFF_SIZE];

    LMK_LOCK_MUTEX(handler->lock);
    flh = (lmk_file_log_handler*) handler;
    if (flh->log_fp != NULL) {
        fflush(flh->log_fp);
        fclose(flh->log_fp);
        flh->log_fp = NULL;
    }
    if ((flh->log_fp = fopen(flh->filename, "wt")) != NULL) {
        lmk_get_timestamp(ts_buff, LMK_TSTAMP_BUFF_SIZE);
        fflush(flh->log_fp);
    } else {
        fprintf(stderr, "Unable to open file for writing : %s (%s:%d)\n",
                flh->filename, __FILE__, __LINE__);
        flh->log_fp = NULL;
    }
    LMK_UNLOCK_MUTEX(handler->lock);
}

/**
 *  @brief Log handler implementation
 *  @param[in] param  Log data (LMK_LOG_REC)
 */

void lmk_file_log_handler_log_impl(lmk_log_handler *handler, void *param) {
    const char *level_str = NULL;
    lmk_log_record *log_rec = NULL;
    lmk_file_log_handler *flh = NULL;
    char ts_buff[LMK_TSTAMP_BUFF_SIZE];

    LMK_LOCK_MUTEX(handler->lock);
    flh = (lmk_file_log_handler*) handler;
    if (flh->log_fp != NULL) {
        log_rec = (lmk_log_record*) param;
        level_str = lmk_get_log_level_str(log_rec->log_level);
        lmk_get_timestamp(ts_buff, LMK_TSTAMP_BUFF_SIZE);
        fprintf(flh->log_fp, "[%-5s %s (%s:%d) %s] : %s\n", level_str, ts_buff,
                log_rec->file_name, log_rec->line_no, handler->name,
                log_rec->data);
        fflush(flh->log_fp);

    }
    LMK_UNLOCK_MUTEX(handler->lock);
}

/**
 *  @brief Destroys the log handler
 *  @param[in] param  Set to NULL value
 */

void lmk_file_log_handler_destroy(lmk_log_handler *handler, void *param) {
    char ts_buff[LMK_TSTAMP_BUFF_SIZE];
    lmk_file_log_handler *flh;

    LMK_LOCK_MUTEX(handler->lock);
    flh = (lmk_file_log_handler*) handler;
    if (flh->log_fp != NULL) {
        lmk_get_timestamp(ts_buff, LMK_TSTAMP_BUFF_SIZE);
        fflush(flh->log_fp);
        fclose(flh->log_fp);
        flh->log_fp = NULL;
    }
    LMK_UNLOCK_MUTEX(handler->lock);
}

/**
 *  @brief Sets the log handler file names. This applies only to file log handler.
 *  @param[in] handler handler
 *  @param[in] filename log file name
 */
LMK_API void lmk_set_log_filename(lmk_log_handler *handler, const char *filename) {
    LMK_LOCK_MUTEX(handler->lock);
    if (handler != NULL && filename != NULL) {
        if (handler->type == LMK_LOG_HANDLER_TYPE_FILE) {
            lmk_file_log_handler *flh = (lmk_file_log_handler*) handler;
            flh->filename = filename;
        }
    }
    LMK_UNLOCK_MUTEX(handler->lock);
}

