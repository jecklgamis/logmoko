#include "logmoko.h"

FILE *config_file;

typedef struct {
    lmk_list link;
    const char *key;
    const char *value;
} key_value_pair;

lmk_list key_value_pair_list;

void lmk_init_config(const char *path) {
}

static void parse_config_file(const char *filename) {
}

void die(const char *msg) {
}

static void *handle_section_entry(const char *name) {
    return NULL;
}

void lmk_destroy_config() {
}


