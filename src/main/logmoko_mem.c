/*
 * RCUNIT - A unit testing framework for C.
 * Copyright (C) 2006 Jerrico L. Gamis
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA
 *
 */

/**
 */
#include "logmoko_mem.h"

/**
 *   @brief Internal memory allocation wrapper
 *   @param[in] size Number of bytes to allocate
 *   @return Pointer to the allocated memory 
 */

void *lmk_malloc(size_t size) {
    return (void*) malloc(size);
}

/**
 *   @brief Internal memory deallocation wrapper
 *   @param[in] addr Allocated memory
 */

void lmk_free(void *addr) {
    if (addr != NULL) {
        free(addr);
    }
}


