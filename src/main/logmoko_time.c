#include "logmoko.h"

int lmk_get_timestamp(char *ts_buff, const int ts_buff_len) {
    if (ts_buff == NULL || ts_buff_len < LMK_TSTAMP_BUFF_SIZE) {
        return LMK_E_NG;
    }
    time_t t = time(NULL);
    struct tm tm_info;
    localtime_r(&t, &tm_info);
    strftime(ts_buff, ts_buff_len, "%Y-%m-%d %H:%M:%S", &tm_info);
    return LMK_E_OK;
}

