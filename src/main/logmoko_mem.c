#include "logmoko_mem.h"

void *lmk_malloc(size_t size) {
    return (void *) malloc(size);
}

void lmk_free(void *addr) {
    if (addr != NULL) {
        free(addr);
    }
}


