#include "logmoko.h"

FILE *config_file;

struct key_value_pair {
    struct lmk_list link;
    const char *key;
    const char *value;
};

