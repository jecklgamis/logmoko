#include "logmoko.h"
#include <ctype.h>



struct lmk_cfg_listener {
    char host[64];
    int port;
};

struct lmk_cfg_handler {
    char name[64];
    int type;
    int level;
    char filename[256];
    struct lmk_cfg_listener listeners[LMK_CFG_MAX_LISTENERS];
    int nr_listeners;
    long max_file_size;
    int  max_backup_files;
    char format[32];
    char ident[64];
    char facility[32];
    char psk[256];
};

struct lmk_cfg_logger {
    char name[64];
    int level;
    char handler_names[LMK_CFG_MAX_HANDLERS_PER_LOGGER][64];
    int nr_handlers;
};

struct lmk_cfg {
    int log_buffer_size;
    int ring_buffer_size;
    struct lmk_cfg_handler handlers[LMK_CFG_MAX_HANDLERS];
    int nr_handlers;
    struct lmk_cfg_logger loggers[LMK_CFG_MAX_LOGGERS];
    int nr_loggers;
};

static char *lmk_cfg_trim(char *s) {
    while (isspace((unsigned char) *s)) s++;
    if (!*s)
        return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char) *end)) end--;
    end[1] = '\0';
    return s;
}

static int lmk_cfg_parse_level(const char *s) {
    if (!strcasecmp(s, "trace")) return LMK_LOG_LEVEL_TRACE;
    if (!strcasecmp(s, "debug")) return LMK_LOG_LEVEL_DEBUG;
    if (!strcasecmp(s, "info"))  return LMK_LOG_LEVEL_INFO;
    if (!strcasecmp(s, "warn"))  return LMK_LOG_LEVEL_WARN;
    if (!strcasecmp(s, "error")) return LMK_LOG_LEVEL_ERROR;
    if (!strcasecmp(s, "off"))   return LMK_LOG_LEVEL_OFF;
    return -1;
}

enum { LMK_SEC_NONE, LMK_SEC_GLOBAL, LMK_SEC_HANDLER, LMK_SEC_LOGGER };

static int lmk_cfg_parse_file(const char *path, struct lmk_cfg *cfg) {
    FILE *fp = fopen(path, "r");
    if (!fp)
        return LMK_E_NG;

    char line[LMK_CFG_LINE_MAX];
    int section = LMK_SEC_NONE;
    struct lmk_cfg_handler *cur_handler = NULL;
    struct lmk_cfg_logger *cur_logger = NULL;

    while (fgets(line, sizeof(line), fp)) {
        char *s = lmk_cfg_trim(line);
        if (!*s || *s == '#' || *s == ';')
            continue;

        if (*s == '[') {
            char *end = strchr(s, ']');
            if (!end)
                continue;
            *end = '\0';
            char *sec_name = lmk_cfg_trim(s + 1);
            cur_handler = NULL;
            cur_logger = NULL;

            if (!strcasecmp(sec_name, "global")) {
                section = LMK_SEC_GLOBAL;
            } else if (!strncasecmp(sec_name, "handler:", 8)) {
                char *hname = lmk_cfg_trim(sec_name + 8);
                if (*hname && cfg->nr_handlers < LMK_CFG_MAX_HANDLERS) {
                    cur_handler = &cfg->handlers[cfg->nr_handlers++];
                    memset(cur_handler, 0, sizeof(*cur_handler));
                    strncpy(cur_handler->name, hname, sizeof(cur_handler->name) - 1);
                    cur_handler->level = -1;
                    cur_handler->type = -1;
                }
                section = LMK_SEC_HANDLER;
            } else if (!strncasecmp(sec_name, "logger:", 7)) {
                char *lname = lmk_cfg_trim(sec_name + 7);
                if (*lname && cfg->nr_loggers < LMK_CFG_MAX_LOGGERS) {
                    cur_logger = &cfg->loggers[cfg->nr_loggers++];
                    memset(cur_logger, 0, sizeof(*cur_logger));
                    strncpy(cur_logger->name, lname, sizeof(cur_logger->name) - 1);
                    cur_logger->level = -1;
                }
                section = LMK_SEC_LOGGER;
            } else {
                section = LMK_SEC_NONE;
            }
            continue;
        }

        char *eq = strchr(s, '=');
        if (!eq)
            continue;
        *eq = '\0';
        char *key = lmk_cfg_trim(s);
        char *val = lmk_cfg_trim(eq + 1);

        if (section == LMK_SEC_GLOBAL) {
            if (!strcasecmp(key, "log_buffer_size"))
                cfg->log_buffer_size = atoi(val);
            else if (!strcasecmp(key, "ring_buffer_size"))
                cfg->ring_buffer_size = atoi(val);
        } else if (section == LMK_SEC_HANDLER && cur_handler) {
            if (!strcasecmp(key, "type")) {
                if (!strcasecmp(val, "console"))
                    cur_handler->type = LMK_LOG_HANDLER_TYPE_CONSOLE;
                else if (!strcasecmp(val, "file"))
                    cur_handler->type = LMK_LOG_HANDLER_TYPE_FILE;
                else if (!strcasecmp(val, "socket"))
                    cur_handler->type = LMK_LOG_HANDLER_TYPE_SOCKET;
                else if (!strcasecmp(val, "syslog"))
                    cur_handler->type = LMK_LOG_HANDLER_TYPE_SYSLOG;
            } else if (!strcasecmp(key, "level")) {
                cur_handler->level = lmk_cfg_parse_level(val);
            } else if (!strcasecmp(key, "filename")) {
                strncpy(cur_handler->filename, val, sizeof(cur_handler->filename) - 1);
            } else if (!strcasecmp(key, "ident")) {
                strncpy(cur_handler->ident, val, sizeof(cur_handler->ident) - 1);
            } else if (!strcasecmp(key, "facility")) {
                strncpy(cur_handler->facility, val, sizeof(cur_handler->facility) - 1);
            } else if (!strcasecmp(key, "format")) {
                strncpy(cur_handler->format, val, sizeof(cur_handler->format) - 1);
            } else if (!strcasecmp(key, "max_file_size")) {
                cur_handler->max_file_size = atol(val);
            } else if (!strcasecmp(key, "max_backup_files")) {
                cur_handler->max_backup_files = atoi(val);
            } else if (!strcasecmp(key, "psk")) {
                strncpy(cur_handler->psk, val, sizeof(cur_handler->psk) - 1);
            } else if (!strcasecmp(key, "listener")) {
                if (cur_handler->nr_listeners < LMK_CFG_MAX_LISTENERS) {
                    char *colon = strrchr(val, ':');
                    if (colon) {
                        struct lmk_cfg_listener *l = &cur_handler->listeners[cur_handler->nr_listeners++];
                        int host_len = (int) (colon - val);
                        if (host_len >= (int) sizeof(l->host))
                            host_len = (int) sizeof(l->host) - 1;
                        strncpy(l->host, val, host_len);
                        l->host[host_len] = '\0';
                        l->port = atoi(colon + 1);
                    }
                }
            }
        } else if (section == LMK_SEC_LOGGER && cur_logger) {
            if (!strcasecmp(key, "level")) {
                cur_logger->level = lmk_cfg_parse_level(val);
            } else if (!strcasecmp(key, "handlers")) {
                char *saveptr = NULL;
                char *tok = strtok_r(val, ",", &saveptr);
                while (tok && cur_logger->nr_handlers < LMK_CFG_MAX_HANDLERS_PER_LOGGER) {
                    char *hname = lmk_cfg_trim(tok);
                    if (*hname)
                        strncpy(cur_logger->handler_names[cur_logger->nr_handlers++],
                                hname, sizeof(cur_logger->handler_names[0]) - 1);
                    tok = strtok_r(NULL, ",", &saveptr);
                }
            }
        }
    }

    fclose(fp);
    return LMK_E_OK;
}

static void lmk_cfg_apply(const struct lmk_cfg *cfg) {
    struct lmk_config *lmk_cfg = lmk_get_config();

    lmk_cfg->log_buffer_size  = cfg->log_buffer_size  > 0 ? (unsigned int) cfg->log_buffer_size
                                                           : LMK_CFG_DEFAULT_LOG_BUFFER_SIZE;
    lmk_cfg->ring_buffer_size = cfg->ring_buffer_size > 0 ? (unsigned int) cfg->ring_buffer_size
                                                           : LMK_CFG_DEFAULT_RING_BUFFER_SIZE;

    for (int i = 0; i < cfg->nr_handlers; i++) {
        const struct lmk_cfg_handler *ch = &cfg->handlers[i];
        if (ch->type < 0)
            continue;
        struct lmk_log_handler *handler = NULL;
        if (ch->type == LMK_LOG_HANDLER_TYPE_CONSOLE) {
            handler = lmk_get_console_log_handler();
        } else if (ch->type == LMK_LOG_HANDLER_TYPE_FILE) {
            handler = lmk_get_file_log_handler(ch->name, ch->filename[0] ? ch->filename : NULL);
            if (handler && (ch->max_file_size > 0 || ch->max_backup_files > 0)) {
                long sz = ch->max_file_size > 0 ? ch->max_file_size : LMK_DEFAULT_MAX_FILE_SIZE;
                int  nb = ch->max_backup_files > 0 ? ch->max_backup_files : LMK_DEFAULT_MAX_BACKUP_FILES;
                lmk_set_log_rotation(handler, sz, nb);
            }
        } else if (ch->type == LMK_LOG_HANDLER_TYPE_SYSLOG) {
            extern int lmk_parse_syslog_facility(const char *s);
            int facility = lmk_parse_syslog_facility(ch->facility[0] ? ch->facility : NULL);
            handler = lmk_get_syslog_log_handler(ch->name, ch->ident[0] ? ch->ident : NULL, facility);
        } else if (ch->type == LMK_LOG_HANDLER_TYPE_SOCKET) {
            handler = lmk_get_socket_log_handler(ch->name);
            if (handler) {
                if (ch->psk[0])
                    lmk_set_socket_psk(handler, ch->psk);
                for (int j = 0; j < ch->nr_listeners; j++)
                    lmk_attach_log_listener(handler, ch->listeners[j].host, ch->listeners[j].port);
            }
        }
        if (handler && ch->level != -1)
            lmk_set_handler_log_level(handler, ch->level);
        if (handler && ch->format[0])
            lmk_set_log_format(handler, lmk_get_format_fn(ch->format));
    }

    for (int i = 0; i < cfg->nr_loggers; i++) {
        const struct lmk_cfg_logger *cl = &cfg->loggers[i];
        struct lmk_logger *logger = lmk_get_logger(cl->name);
        if (!logger)
            continue;
        if (cl->level != -1)
            lmk_set_log_level(logger, cl->level);
        for (int j = 0; j < cl->nr_handlers; j++) {
            struct lmk_log_handler *handler = lmk_search_log_handler_by_name(cl->handler_names[j]);
            if (handler)
                lmk_attach_log_handler(logger, handler);
        }
    }

}

static int lmk_parse_and_apply(const char *path) {
    struct lmk_cfg cfg;
    memset(&cfg, 0, sizeof(cfg));
    if (lmk_cfg_parse_file(path, &cfg) != LMK_E_OK) {
        return LMK_E_NG;
    }
    lmk_cfg_apply(&cfg);
    return LMK_E_OK;
}

int lmk_apply_auto_config() {
    const char *env_path = getenv("LMK_CONFIG_FILE");
    if (env_path)
        return lmk_parse_and_apply(env_path);
    return lmk_parse_and_apply("logmoko.conf");
}

LMK_API int lmk_init_from_file(const char *path) {
    lmk_init();
    if (!path)
        return lmk_apply_auto_config();
    if (lmk_parse_and_apply(path) != LMK_E_OK) {
        fprintf(stderr, "logmoko: unable to open config file: %s\n", path);
        return LMK_E_NG;
    }
    return LMK_E_OK;
}
