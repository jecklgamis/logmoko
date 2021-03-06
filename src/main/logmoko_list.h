
#ifndef LOGMOKO_LIST_H
#define LOGMOKO_LIST_H

#include <stdio.h>

/* A generic doubly linked list */
typedef struct lmk_list_tag {
    struct lmk_list_tag *next; /**< Link to the next entry in the list */
    struct lmk_list_tag *prev; /**< Link to the previous entry in the list */
} lmk_list;

/* Iterates a given list */
#define LMK_FOR_EACH_ENTRY(list, cursor) \
    for((cursor) = (list)->next; (cursor) != (list); (cursor) = (cursor)->next)

/* Saves the current cursor */
#define LMK_SAVE_CURSOR(cursor)                      \
    {                                                \
    lmk_list *__saved_node__ = (cursor)->prev;

/* Restores the current cursor */
#define LMK_RESTORE_CURSOR(cursor) \
    (cursor) = __saved_node__; \
    }

/** List operations function prototypes */
void lmk_init_list(lmk_list *list);

int lmk_is_list_empty(lmk_list *list);

void lmk_insert_list(lmk_list *list, lmk_list *entry);

void lmk_remove_list(lmk_list *entry);

size_t lmk_get_list_size(lmk_list *entry);

#endif /* LOGMOKO_LIST_H */


