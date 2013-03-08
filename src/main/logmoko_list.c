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

#include "logmoko_list.h"

void lmk_init_list(lmk_list *list) {
    if (list != NULL) {
        list->prev = list;
        list->next = list;
    }
}

int lmk_is_list_empty(lmk_list *list) {
    int result = 0;
    if (list != NULL) {
        result = (list->next == list);
    }
    return result;
}

void lmk_insert_list(lmk_list *list, lmk_list *entry) {
    if (list != NULL && entry != NULL) {
        entry->prev = list->prev;
        entry->next = list;
        list->prev->next = entry;
        list->prev = entry;
    }
}

void lmk_remove_list(lmk_list *entry) {
    if (entry != NULL) {
        if (entry->next != entry) {
            entry->prev->next = entry->next;
            entry->next->prev = entry->prev;
        }
    }
}

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
