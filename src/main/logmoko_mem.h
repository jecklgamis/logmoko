

#ifndef LMK_MEM_H
#define LMK_MEM_H

#include <stdlib.h>

extern void *lmk_malloc(size_t size);

extern void lmk_free(void *addr);

extern char *lmk_strdup(const char *str);

#endif /* LMK_MEM_H */


