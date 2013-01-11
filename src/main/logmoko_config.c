/*
 * Logmoko - A logging framework for C.
 * Copyright (C) 2013 Jerrico Gamis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "logmoko.h"

FILE *config_file;

typedef struct {
    lmk_list link;
    const char *key;
    const char *value;
} key_value_pair;

lmk_list key_value_pair_list;

void lmk_init_config(const char *path) {
    if (path != NULL) {
    }
}

static void parse_config_file(const char *filename) {
    for (;;) {
    }
}

void die(const char *msg) {
    puts(msg);
    exit(-1);
}

static void *handle_section_entry(const char *name) {
    if (!strcmp(name, "logger")) {
        key_value_pair *pair = lmk_malloc(sizeof (key_value_pair));
        if (!pair) {
            die("Unable to allocate memory");
        }
    } else if (!strcmp(name, "handler")) {
    }
}

void lmk_destroy_config() {
}


