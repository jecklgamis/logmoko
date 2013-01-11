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

