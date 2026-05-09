
#ifndef LOGMOKO_LIST_H
#define LOGMOKO_LIST_H

#include <stdio.h>

/* A generic doubly linked list */
struct lmk_list {
    struct lmk_list *next; /**< Link to the next entry in the list */
    struct lmk_list *prev; /**< Link to the previous entry in the list */
};

/* Iterates a given list */
#define LMK_FOR_EACH_ENTRY(list, cursor) \
    for((cursor) = (list)->next; (cursor) != (list); (cursor) = (cursor)->next)

/* Saves the current cursor */
#define LMK_SAVE_CURSOR(cursor)                      \
    {                                                \
    struct lmk_list *__saved_node__ = (cursor)->prev;

/* Restores the current cursor */
#define LMK_RESTORE_CURSOR(cursor) \
    (cursor) = __saved_node__; \
    }

/** List operations function prototypes */
void lmk_init_list(struct lmk_list *list);

int lmk_is_list_empty(struct lmk_list *list);

void lmk_insert_list(struct lmk_list *list, struct lmk_list *entry);

void lmk_remove_list(struct lmk_list *entry);

size_t lmk_get_list_size(struct lmk_list *entry);

#endif /* LOGMOKO_LIST_H */


