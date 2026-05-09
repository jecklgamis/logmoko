#ifndef LOGMOKO_SYNC_H
#define LOGMOKO_SYNC_H

typedef pthread_mutex_t lmk_mutex;
typedef pthread_cond_t lmk_cond;

#define LMK_INIT_MUTEX(mutex) \
    pthread_mutex_init(&mutex, NULL);

#define LMK_LOCK_MUTEX(mutex) \
    pthread_mutex_lock(&mutex);

#define LMK_UNLOCK_MUTEX(mutex) \
    pthread_mutex_unlock(&mutex);

#define LMK_DESTROY_MUTEX(mutex) \
    pthread_mutex_destroy(&mutex);

#define LMK_INIT_COND(cond) \
    pthread_cond_init(&cond, NULL);

#define LMK_SIGNAL_COND(cond) \
    pthread_cond_signal(&cond);

#define LMK_WAIT_COND(cond, mutex) \
    pthread_cond_wait(&cond, &mutex);

#define LMK_DESTROY_COND(cond) \
    pthread_cond_destroy(&cond);

#endif /* LOGMOKO_SYNC_H */

