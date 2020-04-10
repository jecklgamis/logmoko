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

