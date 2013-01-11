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

