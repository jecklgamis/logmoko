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
