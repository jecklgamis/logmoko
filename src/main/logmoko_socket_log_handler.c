#include "logmoko.h"
#include "logmoko_encryption.h"

static inline void lmk_socket_maybe_wake(struct lmk_socket_log_handler *slh) {
    atomic_thread_fence(memory_order_seq_cst);
    if (atomic_load_explicit(&slh->sleeping, memory_order_seq_cst)) {
        pthread_mutex_lock(&slh->wakeup_lock);
        pthread_cond_signal(&slh->wakeup_cond);
        pthread_mutex_unlock(&slh->wakeup_lock);
    }
}

static void *lmk_socket_log_handler_thread_routine(void *arg) {
    struct lmk_socket_log_handler *slh = (struct lmk_socket_log_handler *)arg;
    char out[LMK_LOG_MSG_MAX_SIZE + 512];
    unsigned char enc[LMK_LOG_MSG_MAX_SIZE + 512 + CRYPTO_OVERHEAD];

    while (1) {
        struct lmk_ring_slot *slot;
        while ((slot = lmk_ring_peek_slot(slh->ring_buffer, slh->tail, slh->ring_mask))) {
            int n = lmk_format_log_line(&slh->base, out, sizeof(out), &slot->req);
            if (n > 0) {
                struct lmk_list *cursor;
                int use_psk = atomic_load_explicit(&slh->psk_enabled, memory_order_acquire);
                /* Server list is append-only after init; safe to read without lock. */
                LMK_FOR_EACH_ENTRY(&slh->log_server_list, cursor) {
                    struct lmk_log_server *log_server = (struct lmk_log_server *)cursor;
                    if (use_psk) {
                        int enc_len = 0;
                        if (encrypt_packet((EVP_CIPHER_CTX *)slh->cipher_ctx, slh->psk_key,
                                           (unsigned char *)out, n, enc, &enc_len) == 0) {
                            struct lmk_buffer enc_buf = { .addr = enc, .size = (size_t)enc_len };
                            struct lmk_udp_packet enc_pkt = { .buffer = &enc_buf,
                                                              .socket_addr = &log_server->socket_addr };
                            lmk_send_udp_packet(&slh->socket_object, &enc_pkt);
                        }
                    } else {
                        struct lmk_buffer buffer = { .addr = (unsigned char *)out, .size = (size_t)n };
                        struct lmk_udp_packet packet = { .buffer = &buffer,
                                                         .socket_addr = &log_server->socket_addr };
                        lmk_send_udp_packet(&slh->socket_object, &packet);
                    }
                }
            }
            lmk_ring_consume(slh->ring_buffer, &slh->tail, slh->ring_mask);
        }

        if (!atomic_load_explicit(&slh->running, memory_order_acquire))
            break;

        pthread_mutex_lock(&slh->wakeup_lock);
        atomic_store_explicit(&slh->sleeping, 1, memory_order_seq_cst);
        while (!lmk_ring_peek_slot(slh->ring_buffer, slh->tail, slh->ring_mask) &&
               atomic_load_explicit(&slh->running, memory_order_relaxed))
            pthread_cond_wait(&slh->wakeup_cond, &slh->wakeup_lock);
        atomic_store_explicit(&slh->sleeping, 0, memory_order_relaxed);
        pthread_mutex_unlock(&slh->wakeup_lock);
    }
    return NULL;
}

void lmk_socket_log_handler_init(struct lmk_log_handler *handler, void *param) {
    struct lmk_socket_log_handler *slh = (struct lmk_socket_log_handler *)handler;
    pthread_mutex_lock(&handler->lock);
    lmk_init_list(&slh->log_server_list);
    lmk_open_udp_socket(&slh->socket_object);

    size_t ring_size = lmk_next_pow2((size_t)lmk_get_config()->ring_buffer_size);
    slh->ring_mask   = ring_size - 1;
    slh->ring_buffer = lmk_malloc(sizeof(struct lmk_ring_slot) * ring_size);
    if (!slh->ring_buffer) {
        fprintf(stderr, "logmoko: unable to allocate socket handler ring buffer\n");
        lmk_close_udp_socket(&slh->socket_object);
        pthread_mutex_unlock(&handler->lock);
        return;
    }
    lmk_ring_init(slh->ring_buffer, ring_size);
    atomic_init(&slh->head, 0);
    slh->tail = 0;
    atomic_init(&slh->running, 1);
    atomic_init(&slh->sleeping, 0);
    atomic_init(&slh->dropped, 0);
    atomic_init(&slh->psk_enabled, 0);
    slh->cipher_ctx = EVP_CIPHER_CTX_new();
    pthread_mutex_init(&slh->wakeup_lock, NULL);
    pthread_cond_init(&slh->wakeup_cond, NULL);

    if (pthread_create(&slh->thread, NULL,
                       lmk_socket_log_handler_thread_routine, slh) != 0) {
        fprintf(stderr, "logmoko: unable to create socket handler thread\n");
        lmk_free(slh->ring_buffer);
        slh->ring_buffer = NULL;
        atomic_store(&slh->running, 0);
        lmk_close_udp_socket(&slh->socket_object);
        EVP_CIPHER_CTX_free((EVP_CIPHER_CTX *)slh->cipher_ctx);
        slh->cipher_ctx = NULL;
        pthread_cond_destroy(&slh->wakeup_cond);
        pthread_mutex_destroy(&slh->wakeup_lock);
    }
    pthread_mutex_unlock(&handler->lock);
}

void lmk_socket_log_handler_log_impl(struct lmk_log_handler *handler, void *param) {
    struct lmk_socket_log_handler *slh = (struct lmk_socket_log_handler *)handler;
    struct lmk_log_record *rec = (struct lmk_log_record *)param;

    if (!atomic_load_explicit(&slh->running, memory_order_relaxed))
        return;
    if (!lmk_ring_enqueue(slh->ring_buffer, &slh->head, slh->ring_mask,
                          rec, handler->name)) {
        atomic_fetch_add_explicit(&slh->dropped, 1, memory_order_relaxed);
        return;
    }
    atomic_fetch_add_explicit(&handler->nr_log_calls, 1, memory_order_relaxed);
    lmk_socket_maybe_wake(slh);
}

void lmk_socket_log_handler_destroy(struct lmk_log_handler *handler, void *param) {
    struct lmk_socket_log_handler *slh = (struct lmk_socket_log_handler *)handler;

    atomic_store_explicit(&slh->running, 0, memory_order_release);
    pthread_mutex_lock(&slh->wakeup_lock);
    pthread_cond_broadcast(&slh->wakeup_cond);
    pthread_mutex_unlock(&slh->wakeup_lock);
    pthread_join(slh->thread, NULL);
    pthread_cond_destroy(&slh->wakeup_cond);
    pthread_mutex_destroy(&slh->wakeup_lock);

    unsigned long dropped = atomic_load(&slh->dropped);
    if (dropped)
        fprintf(stderr, "logmoko: socket handler '%s' dropped %lu log(s), logged %lu\n",
                handler->name, dropped, atomic_load(&handler->nr_log_calls));
    lmk_free(slh->ring_buffer);
    slh->ring_buffer = NULL;
    EVP_CIPHER_CTX_free((EVP_CIPHER_CTX *)slh->cipher_ctx);
    slh->cipher_ctx = NULL;

    pthread_mutex_lock(&handler->lock);
    struct lmk_list *cursor;
    LMK_FOR_EACH_ENTRY(&slh->log_server_list, cursor) {
        struct lmk_log_server *log_server = (struct lmk_log_server *)cursor;
        LMK_SAVE_CURSOR(cursor);
            lmk_remove_list(&log_server->link);
            lmk_free(log_server);
        LMK_RESTORE_CURSOR(cursor);
    }
    lmk_close_udp_socket(&slh->socket_object);
    pthread_mutex_unlock(&handler->lock);
}

LMK_API void lmk_attach_log_listener(struct lmk_log_handler *handler,
                                      const char *host, int port) {
    if (!handler || !host || port <= 0 ||
        handler->type != LMK_LOG_HANDLER_TYPE_SOCKET)
        return;
    pthread_mutex_lock(&handler->lock);
    {
        struct lmk_socket_log_handler *slh = (struct lmk_socket_log_handler *)handler;
        struct lmk_log_server *log_server =
            (struct lmk_log_server *)lmk_malloc(sizeof(struct lmk_log_server));
        if (log_server) {
            memset(log_server, 0, sizeof(struct lmk_log_server));
            lmk_init_list(&log_server->link);
            log_server->socket_addr.sin_family = PF_INET;
            log_server->socket_addr.sin_port   = htons(port);
            inet_pton(PF_INET, host, &log_server->socket_addr.sin_addr);
            lmk_insert_list(&slh->log_server_list, &log_server->link);
        }
    }
    pthread_mutex_unlock(&handler->lock);
}

LMK_API void lmk_set_socket_psk(struct lmk_log_handler *handler, const char *psk) {
    if (!handler || !psk || handler->type != LMK_LOG_HANDLER_TYPE_SOCKET)
        return;
    struct lmk_socket_log_handler *slh = (struct lmk_socket_log_handler *)handler;
    /* Derive the 32-byte key via SHA-256(psk), then atomically enable encryption. */
    derive_key(psk, slh->psk_key);
    atomic_store_explicit(&slh->psk_enabled, 1, memory_order_release);
}
