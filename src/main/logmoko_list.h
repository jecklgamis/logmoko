/*
 * The MIT License (MIT)
 *
 * Logmoko - A logging framework for C
 * Copyright 2013 Jerrico Gamis <jecklgamis@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

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


