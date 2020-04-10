#ifndef LOGMOKO_SYNC_H
#define LOGMOKO_SYNC_H

typedef pthread_mutex_t lmk_mutex;

#define LMK_INIT_MUTEX(mutex) \
    pthread_mutex_init(&mutex, NULL);

#define LMK_LOCK_MUTEX(mutex) \
    pthread_mutex_lock(&mutex);

#define LMK_UNLOCK_MUTEX(mutex) \
    pthread_mutex_unlock(&mutex);

#endif /* LOGMOKO_SYNC_H */

