/*
 * The MIT License (MIT)
 *
 * Logmoko - A logging framework for C
 * Copyright 2013 Jerrico Gamis <jecklgamis@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "logmoko.h"

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

void lmk_console_log_handler_destroy(lmk_log_handler *handler, void *param) {
}

void lmk_console_log_handler_init(lmk_log_handler *handler, void *param) {
}

