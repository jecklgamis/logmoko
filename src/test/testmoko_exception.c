/*
 * TestMoko - A unit testing framework for C.
 * Copyright (C) 2012 Jerrico Gamis
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

#include <testmoko_exception.h>

/** @brief Current exception stack frame */
tmk_exception_frame *g_tmk_curr_excp_frame = NULL;
int g_tmk_excp_initialized = 0;

/** @brief Defines an exception object table entry */
#define TMK_DEFINE_EXCEPTION(id, name) \
    { id, name },

/** @brief Global exception object table */
tmk_exception
g_tmk_excp_tbl[] = {
    TMK_DEFINE_EXCEPTION(TMK_EXCP_ID_START, "Reserved (TMK_EXCP_ID_START)")
    TMK_DEFINE_EXCEPTION(TMK_EXCP_ABORTTESTRUN, "test function aborted exception")
    TMK_DEFINE_EXCEPTION(TMK_EXCP_ABORTMODRUN, "test module aborted exception")
    TMK_DEFINE_EXCEPTION(TMK_EXCP_NULLPOINTER, "null pointer exception")
    TMK_DEFINE_EXCEPTION(TMK_EXCP_INVPARAM, "invalid parameter exception")
    TMK_DEFINE_EXCEPTION(TMK_EXCP_ASSERTIONFAILURE, "assertion failure exception")
    TMK_DEFINE_EXCEPTION(TMK_EXCP_ID_END, "Reserved (TMK_EXCP_ID_END)")
    TMK_DEFINE_EXCEPTION(TMK_EXCP_ID_UNKNOWN, "Unknown")
};

/**
 *  @brief Retrieves the exception object of the given exception id
 *  @param[in] id Exception identifier
 *  @return Exception object
 */

tmk_exception *tmk_lookup_excp_by_id(tmk_exception_id id) {
    tmk_exception *excp = &g_tmk_excp_tbl[TMK_EXCP_ID_UNKNOWN];
    if (id > TMK_EXCP_ID_START && id < TMK_EXCP_ID_END) {
        excp = &g_tmk_excp_tbl[id];
    }
    return excp;
}

/**
 *  @brief Initializes the exception mechanism
 *  @return Operation error code (TMK_E_OK, TMK_E_NG)
 */

void tmk_init_exception() {
    if (!g_tmk_excp_initialized) {
        g_tmk_curr_excp_frame = NULL;
    }
}

/**
 *  @brief Destroys the exception mechanism
 *  @return Operation error code (TMK_E_OK, TMK_E_NG)
 */

void tmk_destroy_exception() {
    if (g_tmk_excp_initialized) {
    }
}