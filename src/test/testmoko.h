/*
 * Testmoko - A unit testing framework for C.
 * Copyright (C) 2012 Jerrico Gamis (jecklgamis@gmail.com)
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

#ifndef TESTMOKO_H
#define TESTMOKO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include <time.h>
#include <stdarg.h>

#include "testmoko_exception.h"

#define TMK_API
#define TMK_NULLABLE

#define TEST_RESULT_SUCCESSFUL 0
#define TEST_RESULT_FAILED 1

typedef void(*tmk_test_function)();

typedef struct tmk_test_function_entry {
    const char *name;
    tmk_test_function test;
    tmk_test_function setup;
    tmk_test_function teardown;
    int result;
} tmk_test_function_entry;

#define TMK_TEST(name) \
    void name(void)

#define TMK_TEST_FUNCTION_TABLE_START(name) \
    tmk_test_function_entry name[]={

#define TMK_INCLUDE_TEST(func) \
    {#func, func, NULL, NULL, 1},

#define TMK_INCLUDE_TEST_ST(func, setup, teardown) \
    {#func, func, setup, teardown},

#define TMK_TEST_FUNCTION_TABLE_END \
     { NULL, NULL }                  \
    };

#define TMK_EXTERN_TEST_FUNCTION_TABLE(name) \
   extern tmk_test_function_entry name[];

/** @brief Asserts a condition */
#define TMK_ASSERT(cond) \
 { tmk_assert_impl((cond), __FILE__, __LINE__, "%s", #cond); }

/** @brief Asserts a false condition */
#define TMK_ASSERT_TRUE(cond) \
 { tmk_assert_impl((cond), __FILE__,__LINE__, "%s is not true", #cond); }

/** @brief Asserts a false condition */
#define TMK_ASSERT_FALSE(cond) \
 { tmk_assert_impl(!(cond), __FILE__,__LINE__, "%s is true", #cond); }

/** @brief Asserts a null pointer */
#define TMK_ASSERT_NULL(ptr) \
 { tmk_assert_impl(((ptr) == NULL), __FILE__,__LINE__, "null", #ptr); }

/** @brief Asserts a non-null pointer */
#define TMK_ASSERT_NOT_NULL(ptr) \
 { tmk_assert_impl(((ptr) != NULL), __FILE__,__LINE__,"pointer is not null", #ptr); }

/** @brief Asserts that the two variables are equal */
#define TMK_ASSERT_EQUAL(expected, actual) \
    { tmk_assert_impl((expected == actual), __FILE__, __LINE__, "expected %d but got %d", expected, actual); }

/** @brief Asserts that the two strings are equal */
#define TMK_ASSERT_EQUAL_STRING(expected, actual) \
    { tmk_assert_impl( !(strcmp((expected), (actual))), __FILE__, __LINE__, "expected \"%s\" but got \"%s\"", expected, actual); }

/** @brief Asserts that the two byte arrays are equal */
#define TMK_ASSERT_EQUAL_BYTE_ARRAY(expected, actual, size) \
    { tmk_assert_impl( !(memcmp(expected, actual, size)), __FILE__, __LINE__, "unequal byte arrays"); }

/** @brief Records an explicitly failed condition described by msg */
#define TMK_FAIL(msg) \
 { tmk_assert_impl(0, __FILE__, __LINE__, "Failed explicitly : %s", msg); }

void tmk_log_impl(const char* filename, const int line_no,
        const char *format, ...);

#define TMK_LOG(format...) \
	tmk_log_impl(__FILE__, __LINE__, format);

extern TMK_API void tmk_run_tests(const tmk_test_function_entry *tbl, 
        TMK_NULLABLE void (*setup)(), void (*teardown)());

void tmk_assert_impl(int cond, const char *fname, int line, const char *format, ...);

#endif /* TESTMOKO_H */

