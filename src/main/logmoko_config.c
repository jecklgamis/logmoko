#include "logmoko.h"

struct lmk_config *lmk_create_config() {
    struct lmk_config *config = lmk_malloc(sizeof(struct lmk_config));
    if (!config) {
        fprintf(stderr, "Unable to allocate config");
        return NULL;
    }
    return config;
}


