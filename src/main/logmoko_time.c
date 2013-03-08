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

int lmk_get_timestamp(char *ts_buff, const int ts_buff_len) {
    time_t t;
    int len;
    if (ts_buff == NULL || ts_buff_len < LMK_TSTAMP_BUFF_SIZE) {
        return LMK_E_NG;
    }
    memset(ts_buff, 0, ts_buff_len);
    time(&t);
    sprintf(ts_buff, "%s", ctime(&t));
    len = strlen(ts_buff);
    memset(&ts_buff[len - 1], 0, 1);
    return LMK_E_OK;
}

