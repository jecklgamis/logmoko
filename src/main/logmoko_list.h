/*
 * Logmoko - A logging framework for C.
 * Copyright (C) 2013 Jerrico Gamis
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA
 *
 */

#ifndef LOGMOKO_LIST_H
#define LOGMOKO_LIST_H

#include <stdio.h>

/** @brief A generic doubly linked list */
typedef struct lmk_list_tag {
    struct lmk_list_tag *next; /**< Link to the next entry in the list */
    struct lmk_list_tag *prev; /**< Link to the previous entry in the list */
} lmk_list;

/** @brief Iterates a given list */
#define LMK_FOR_EACH_ENTRY(list, cursor) \
    for((cursor) = (list)->next; (cursor) != (list); (cursor) = (cursor)->next)

/** @brief Saves the current cursor */
#define LMK_SAVE_CURSOR(cursor)                      \
    {                                                \
    lmk_list *__saved_node__ = (cursor)->prev;

/** @brief Restores the current cursor */
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


