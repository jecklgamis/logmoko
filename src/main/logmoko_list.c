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

#include "logmoko_list.h"

/**
 *  @brief Initializes the given list or list entry
 *  @param[in] list  List or list entry
 */

void lmk_init_list(lmk_list *list) {
    if (list != NULL) {
        list->prev = list;
        list->next = list;
    }
}

/**
 *  @brief Returns true if the given list is empty
 *  @param[in] list List
 *  @return Boolean value (RCU_TRUE, 0)
 */

int lmk_is_list_empty(lmk_list *list) {
    int result = 0;
    if (list != NULL) {
        result = (list->next == list);
    }
    return result;
}

/**
 *  @brief Inserts a entry into a list
 *  @param[in] list List
 *  @param[in] entry List entry
 */

void lmk_insert_list(lmk_list *list, lmk_list *entry) {
    if (list != NULL && entry != NULL) {
        entry->prev = list->prev;
        entry->next = list;
        list->prev->next = entry;
        list->prev = entry;
    }
}

/**
 *  @brief Removes an entry from a list
 *  @param[in] entry List entry
 */

void lmk_remove_list(lmk_list *entry) {
    if (entry != NULL) {
        if (entry->next != entry) {
            entry->prev->next = entry->next;
            entry->next->prev = entry->prev;
        }
    }
}

/**
 *  @brief Returns the number of elements of the list
 *  @param[in] list List
 */
size_t lmk_get_list_size(lmk_list *list) {
    size_t size = 0L;
    lmk_list *cursor = NULL;
    if (list != NULL) {
        LMK_FOR_EACH_ENTRY(list, cursor) {
            size++;
        }
    }
    return size;
}
