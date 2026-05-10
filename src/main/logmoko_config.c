#include "logmoko.h"

int get_int_key_from_env(const char *name, int fallback) {
    char *value = getenv(name);
    if (!value) {
        return fallback;
    }
    return atoi(value);
}

struct lmk_config *lmk_load_config() {
    struct lmk_config *config = lmk_malloc(sizeof(struct lmk_config));
    if (!config) {
        fprintf(stderr, "Unable to allocate config");
        return NULL;
    }
    config->log_buffer_size = get_int_key_from_env("LMK_LOG_BUFFER_SIZE", 2048);
    if (config->log_buffer_size <=0) {
        config->log_buffer_size = 2048;
    }
    config->ring_buffer_size = get_int_key_from_env("LMK_RING_BUFFER_SIZE", 128);
    if (config->ring_buffer_size <=0) {
       config->ring_buffer_size = 128;
    }
    config->default_level = LMK_LOG_LEVEL_INFO;
#if LMK_DEBUG
    fprintf(stdout, "Using log_buffer_size = %d\n", config->log_buffer_size);
    fprintf(stdout, "Using ring_buffer_size = %d\n", config->ring_buffer_size);
    fprintf(stdout, "Using default_level = %d\n", config->default_level);
#endif
    return config;
}


