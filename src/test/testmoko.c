/*
 * Testmoko - A unit testing framework for C.
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

#include "testmoko.h"

unsigned int g_nr_failed_test;
unsigned int g_nr_succ_test;
static int g_tmk_initialized = 0;

#define TMK_LOG_BUFF_SIZE 2048
static char local_log_buff[TMK_LOG_BUFF_SIZE];
tmk_test_function *tmk_test_function_table;
char assert_msg_buff[TMK_LOG_BUFF_SIZE];

void tmk_log_impl(const char* filename, const int line_no, const char *format, ...) {
    va_list ap;
    memset(local_log_buff, 0, TMK_LOG_BUFF_SIZE);
    va_start(ap, format);
    vsprintf(local_log_buff, format, ap);
    va_end(ap);
    fprintf(stdout, "[Testmoko] %s\n", local_log_buff);
}

void tmk_assert_impl(int cond, const char *filename, int line, const char *format, ...) {
    va_list ap;
    memset(&assert_msg_buff[0], 0, TMK_LOG_BUFF_SIZE);
    va_start(ap, format);
    vsprintf(assert_msg_buff, format, ap);
    va_end(ap);
    if (!cond) {
        TMK_LOG("Assertion failure in  %s:%d : %s", filename, line, assert_msg_buff);
        TMK_THROW(TMK_GET_EXCP(TMK_EXCP_ASSERTIONFAILURE));
    }
}

TMK_API void tmk_run_tests(const tmk_test_function_entry *tbl,
        TMK_NULLABLE void (*setup)(), TMK_NULLABLE void (*teardown)()) {
    tmk_test_function_entry *test;
    g_nr_failed_test = 0;
    TMK_LOG("Test started");

    for (test = tbl; test->name != NULL; test++) {
        test->result = TEST_RESULT_SUCCESSFUL;
        TMK_TRY
        {
            if (test->setup != NULL) {
                test->setup();
            } else {
                if (setup != NULL) { 
                    setup();
                }
            }
            if (test->test != NULL) {
                test->test();
            }
            if (test->teardown != NULL) {
                test->teardown();
            } else {
                if (teardown != NULL) {
                    teardown();
                }
            }
            g_nr_succ_test++;
            TMK_LOG("Test : %s OK", test->name);
        }

        TMK_CATCH(e) {
            TMK_LOG("Test : %s NG", test->name);
            g_nr_failed_test++;
            continue;
            test->result = TEST_RESULT_FAILED;
        }
        TMK_END_CATCH
    }
    TMK_LOG("Test Results : Passed = %lu, Failed = %lu ", g_nr_succ_test, g_nr_failed_test);
    if (g_nr_failed_test != 0) {
        TMK_LOG("Test failed!");
    } else {
        TMK_LOG("Test successful");
    }
    TMK_LOG("Test finished");
}

void tmk_init() {
    if (!g_tmk_initialized) {
        tmk_init_exception();
        g_tmk_initialized = 1;
    }
}

void tmk_destroy() {
    if (g_tmk_initialized) {
        tmk_destroy_exception();
    }
}