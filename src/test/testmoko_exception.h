/*
 * TestMoko - A unit testing framework for C.
 * Copyright (C) 2011 Jerrico L. Gamis
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

#ifndef TESTMOKO_EXCEPTION_H
#define TESTMOKO_EXCEPTION_H

#include <stdio.h>
#include <setjmp.h>

/** @brief Exception identifier */
typedef enum tmk_exception_id {
    TMK_EXCP_ID_START = 0 /**< Reserved exception */
    , TMK_EXCP_ABORTTESTRUN /**< Test function abort */
    , TMK_EXCP_ABORTMODRUN /**< Test module abort */
    , TMK_EXCP_NULLPOINTER /**< Null pointer */
    , TMK_EXCP_INVPARAM /**< Invalid parameter */
    , TMK_EXCP_ASSERTIONFAILURE /** <Assertion failure */
    /** Start of user-defined exceptions here */
    /** End of user-defined exceptions here */
    , TMK_EXCP_ID_END /**< Reserved exception */
    , TMK_EXCP_ID_UNKNOWN
    /**< Reserved exception */
} tmk_exception_id;

/** @brief Exception runtime data structure */
typedef struct tmk_exception {
    tmk_exception_id id; /**< Exception id */
    const char *name; /**< Exception name */
} tmk_exception;

/** @brief Exception frame runtime data structure */
typedef struct tmk_exception_frame {
    tmk_exception *excp; /**< Exception */
    sigjmp_buf *env; /**< native exception context */
    struct tmk_exception_frame *outer; /**< outer exception frame */
} tmk_exception_frame;

typedef struct tmk_exception_context {
    tmk_exception_frame *excp_root;
} tmk_exception_context;

#define TMK_EXCEPTION_TYPE_GET() (__ret_code)
#define TMK_EXCEPTION_TYPE_NONE  (0)
#define TMK_EXCEPTION_TYPE_THROW (1)
#define TMK_EXCEPTION_TYPE_CORE  (10)

/** @brief Saves an exception context */
#define TMK_TRY                               \
    {                                         \
        int __retcode = -1;                    \
        sigjmp_buf __env;                     \
        tmk_exception_frame  __frame;          \
        __frame.outer = g_tmk_curr_excp_frame;    \
        __frame.excp = NULL;              \
		g_tmk_curr_excp_frame = &__frame;         \
        g_tmk_curr_excp_frame->env = &__env;      \
        __retcode = sigsetjmp(__env, 1);       \
        if (__retcode == 0) {                 \

/** @brief Catches an exception and store in the given variable */
#define TMK_CATCH(__caught_excp)             \
        }                                     \
        g_tmk_curr_excp_frame = __frame.outer;    \
        if (__retcode != TMK_EXCEPTION_TYPE_NONE ) {            \
            tmk_exception *__caught_excp = __frame.excp;       \
            int __ret_code = __retcode;                     \

/** @brief Terminates TMK_CATCH */
#define TMK_END_CATCH                                          \
       }                                                       \
    }

/** @brief Throws the given exception object */
#define TMK_THROW(__thrown_excp__)                \
    {                                           \
        g_tmk_curr_excp_frame->excp = __thrown_excp__;\
        siglongjmp(*(g_tmk_curr_excp_frame->env), TMK_EXCEPTION_TYPE_THROW);\
    }

/** @brief Retrieves the exception object of the given exception id*/
#define TMK_GET_EXCP(id)  \
    (tmk_lookup_excp_by_id((id)))

/** @brief Exception mechanism function prototypes */
void tmk_init_exception();
void tmk_destroy_exception();
tmk_exception *tmk_lookup_excp_by_id(tmk_exception_id id);

/** External variable declarations */
extern tmk_exception_frame *g_tmk_curr_excp_frame;

#endif /* TESTMOKO_EXCEPTION_H */

