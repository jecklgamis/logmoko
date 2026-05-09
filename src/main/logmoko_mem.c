#include "logmoko_mem.h"
#include <string.h>

void *lmk_malloc(size_t size) {
    return (void *) malloc(size);
}

void lmk_free(void *addr) {
    if (addr != NULL) {
        free(addr);
    }
}

char *lmk_strdup(const char *str) {
    if (str == NULL) {
        return NULL;
    }
    size_t len = strlen(str) + 1;
    char *new_str = (char *) lmk_malloc(len);
    if (new_str != NULL) {
        memcpy(new_str, str, len);
    }
    return new_str;
}


