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


