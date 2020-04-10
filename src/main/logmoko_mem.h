

#ifndef LMK_MEM_H
#define LMK_MEM_H

#include <stdlib.h>

extern void *lmk_malloc(size_t size);

extern void lmk_free(void *addr);

#endif /* LMK_MEM_H */


