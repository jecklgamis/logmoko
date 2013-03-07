#include <stdlib.h>
#include <pthread.h>
#include "logmoko.h"
#include "testmoko.h"

/** Verify normal creation and destruction of logger 
 *  Verify default log level
 */
TMK_TEST(lmk_test_create_and_destroy_logger) {
    lmk_logger *logger = lmk_get_logger("logger");
    TMK_ASSERT_NOT_NULL(logger);
    TMK_ASSERT_EQUAL(LMK_LOG_LEVEL_TRACE, lmk_get_log_level(logger));
    lmk_destroy_logger(&logger);
    TMK_ASSERT_NULL(logger);
    TMK_ASSERT_EQUAL(0, lmk_get_nr_loggers());
    lmk_destroy();
}

/** Verify that logging using a null logger is allowed (no exceptions of what kind)
 */
TMK_TEST(lmk_test_null_logger) {
    LMK_LOG_TRACE(NULL, "This is a trace log");
    lmk_destroy();
}

/** Verify that a null logger name is not allowed */
TMK_TEST(lmk_test_null_logger_name) {
    lmk_logger *logger = lmk_get_logger(NULL);
    TMK_ASSERT_NULL(logger);
    lmk_destroy();
}

/** Verify that the same logger is returned if names are similar */
TMK_TEST(lmk_test_get_existing_logger) {
    lmk_logger *logger = lmk_get_logger("logger");
    TMK_ASSERT_NOT_NULL(logger);

    lmk_logger *logger2 = lmk_get_logger("logger");
    TMK_ASSERT_NOT_NULL(logger2);
    TMK_ASSERT_EQUAL(logger, logger2);
    lmk_destroy();
}

/** Verify that log requests are delegated to attached loggers */
TMK_TEST(lmk_test_logger_log_levels) {
    lmk_logger *logger = lmk_get_logger("logger");
    TMK_ASSERT_NOT_NULL(logger);
    lmk_set_log_level(logger, LMK_LOG_LEVEL_TRACE);

    lmk_log_handler *clh = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh);
    lmk_set_handler_log_level(clh, LMK_LOG_LEVEL_TRACE);

    lmk_attach_log_handler(logger, clh);

    LMK_LOG_TRACE(logger, "This is a trace log");
    LMK_LOG_INFO(logger, "This is a debug log");
    LMK_LOG_DEBUG(logger, "This is an info log");
    LMK_LOG_WARN(logger, "This is a warn log");
    LMK_LOG_ERROR(logger, "This is an error log");
    LMK_LOG_FATAL(logger, "This is a fatal log");

    TMK_ASSERT_EQUAL(6, clh->nr_log_calls);

    clh->nr_log_calls = 0;
    lmk_set_handler_log_level(clh, LMK_LOG_LEVEL_DEBUG);

    LMK_LOG_TRACE(logger, "This is a trace log");
    LMK_LOG_INFO(logger, "This is a debug log");
    LMK_LOG_DEBUG(logger, "This is an info log");
    LMK_LOG_WARN(logger, "This is a warn log");
    LMK_LOG_ERROR(logger, "This is an error log");
    LMK_LOG_FATAL(logger, "This is a fatal log");

    TMK_ASSERT_EQUAL(5, clh->nr_log_calls);

    clh->nr_log_calls = 0;
    lmk_set_handler_log_level(clh, LMK_LOG_LEVEL_INFO);

    LMK_LOG_TRACE(logger, "This is a trace log");
    LMK_LOG_INFO(logger, "This is a debug log");
    LMK_LOG_DEBUG(logger, "This is an info log");
    LMK_LOG_WARN(logger, "This is a warn log");
    LMK_LOG_ERROR(logger, "This is an error log");
    LMK_LOG_FATAL(logger, "This is a fatal log");

    TMK_ASSERT_EQUAL(4, clh->nr_log_calls);

    clh->nr_log_calls = 0;
    lmk_set_handler_log_level(clh, LMK_LOG_LEVEL_WARN);

    LMK_LOG_TRACE(logger, "This is a trace log");
    LMK_LOG_INFO(logger, "This is a debug log");
    LMK_LOG_DEBUG(logger, "This is an info log");
    LMK_LOG_WARN(logger, "This is a warn log");
    LMK_LOG_ERROR(logger, "This is an error log");
    LMK_LOG_FATAL(logger, "This is a fatal log");

    TMK_ASSERT_EQUAL(3, clh->nr_log_calls);

    clh->nr_log_calls = 0;
    lmk_set_handler_log_level(clh, LMK_LOG_LEVEL_ERROR);

    LMK_LOG_TRACE(logger, "This is a trace log");
    LMK_LOG_INFO(logger, "This is a debug log");
    LMK_LOG_DEBUG(logger, "This is an info log");
    LMK_LOG_WARN(logger, "This is a warn log");
    LMK_LOG_ERROR(logger, "This is an error log");
    LMK_LOG_FATAL(logger, "This is a fatal log");

    TMK_ASSERT_EQUAL(2, clh->nr_log_calls);

    clh->nr_log_calls = 0;
    lmk_set_handler_log_level(clh, LMK_LOG_LEVEL_FATAL);

    LMK_LOG_TRACE(logger, "This is a trace log");
    LMK_LOG_INFO(logger, "This is a debug log");
    LMK_LOG_DEBUG(logger, "This is an info log");
    LMK_LOG_WARN(logger, "This is a warn log");
    LMK_LOG_ERROR(logger, "This is an error log");
    LMK_LOG_FATAL(logger, "This is a fatal log");

    TMK_ASSERT_EQUAL(1, clh->nr_log_calls);

    clh->nr_log_calls = 0;
    lmk_set_handler_log_level(clh, LMK_LOG_LEVEL_OFF);

    LMK_LOG_TRACE(logger, "This is a trace log");
    LMK_LOG_INFO(logger, "This is a debug log");
    LMK_LOG_DEBUG(logger, "This is an info log");
    LMK_LOG_WARN(logger, "This is a warn log");
    LMK_LOG_ERROR(logger, "This is an error log");
    LMK_LOG_FATAL(logger, "This is a fatal log");

    TMK_ASSERT_EQUAL(0, clh->nr_log_calls);

    /** Verify that even if the log request is not filtered out by
     * the logger, still, it will not be logged if the log handler level
     * is more restrictive than the logger.
     */
    clh->nr_log_calls = 0;

    lmk_set_log_level(logger, LMK_LOG_LEVEL_WARN);
    lmk_set_handler_log_level(clh, LMK_LOG_LEVEL_ERROR);

    LMK_LOG_WARN(logger, "This is a warn log");
    LMK_LOG_FATAL(logger, "This is a fatal log");

    TMK_ASSERT_EQUAL(1, clh->nr_log_calls);

    /**
     * Verify that if the handler log level is less restrictive than the logger
     * log level, requested log levels greater than that of the logger will
     * be logged.
     */
    clh->nr_log_calls = 0;

    lmk_set_log_level(logger, LMK_LOG_LEVEL_WARN);
    lmk_set_handler_log_level(clh, LMK_LOG_LEVEL_INFO);

    LMK_LOG_WARN(logger, "This is a warn log");
    LMK_LOG_FATAL(logger, "This is a fatal log");

    TMK_ASSERT_EQUAL(2, clh->nr_log_calls);

    lmk_destroy();
}

/**
 * Verify that we can successfully create different log handler types
 */
TMK_TEST(lmk_test_create_handlers) {
    lmk_log_handler *clh = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh);
    TMK_ASSERT_EQUAL(LMK_LOG_HANDLER_TYPE_CONSOLE, clh->type);
    TMK_ASSERT_TRUE(clh->initialized);
    lmk_log_handler *flh = lmk_get_file_log_handler("flh", "flh.log");
    TMK_ASSERT_NOT_NULL(flh);
    TMK_ASSERT_EQUAL(LMK_LOG_HANDLER_TYPE_FILE, flh->type);
    TMK_ASSERT_TRUE(flh->initialized);
    lmk_destroy();
}

/**
 * Verify we can only create one and only one console log handler.
 */
TMK_TEST(lmk_test_console_log_handler) {
    lmk_log_handler *clh1 = lmk_get_console_log_handler();
    lmk_log_handler *clh2 = lmk_get_console_log_handler();
    TMK_ASSERT_EQUAL(clh1, clh2);
    lmk_destroy();
}

/**
 * Verify that we can successfully destroy handlers
 */
TMK_TEST(lmk_test_destroy_handlers) {
    lmk_log_handler *clh = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh);
    lmk_log_handler *flh = lmk_get_file_log_handler("flh", "flh.log");
    TMK_ASSERT_NOT_NULL(flh);
    lmk_destroy_log_handler(&clh);
    TMK_ASSERT_NULL(clh);
    lmk_destroy_log_handler(&flh);
    TMK_ASSERT_NULL(flh);
    TMK_ASSERT_EQUAL(0, lmk_get_nr_handlers());
    lmk_destroy();
}

/**
 * Verify that a handler cannot be destroyed if there are more than one
 * loggers referencing it.
 */
TMK_TEST(lmk_test_destroy_handler_with_logger_references) {
    lmk_logger *logger = lmk_get_logger("logger");
    TMK_ASSERT_NOT_NULL(logger);

    lmk_log_handler *clh = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh);

    TMK_ASSERT_EQUAL(LMK_E_OK, lmk_attach_log_handler(logger, clh));
    TMK_ASSERT_EQUAL(1, lmk_get_nr_loggers());
    TMK_ASSERT_EQUAL(1, lmk_get_nr_handlers());

    TMK_ASSERT_EQUAL(LMK_E_NG, lmk_destroy_log_handler(&clh));
    TMK_ASSERT_EQUAL(1, lmk_get_nr_loggers());
    TMK_ASSERT_EQUAL(1, lmk_get_nr_handlers());

    lmk_destroy();
}

/**
 * Verify log handler attachment to multiple loggers.
 */
TMK_TEST(lmk_test_attach_handlers_to_multiple_loggers) {
    lmk_logger *logger1 = lmk_get_logger("logger1");
    TMK_ASSERT_NOT_NULL(logger1);

    lmk_logger *logger2 = lmk_get_logger("logger2");
    TMK_ASSERT_NOT_NULL(logger2);

    lmk_log_handler *clh = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh);

    lmk_log_handler *flh = lmk_get_file_log_handler("flh", "flh.log");
    TMK_ASSERT_NOT_NULL(flh);

    TMK_ASSERT_EQUAL(LMK_E_OK, lmk_attach_log_handler(logger1, clh));
    TMK_ASSERT_EQUAL(LMK_E_OK, lmk_attach_log_handler(logger1, flh));
    TMK_ASSERT_EQUAL(LMK_E_OK, lmk_attach_log_handler(logger2, clh));
    TMK_ASSERT_EQUAL(LMK_E_OK, lmk_attach_log_handler(logger2, flh));

    TMK_ASSERT_EQUAL(2, lmk_get_list_size(&logger1->handler_ref_list));
    TMK_ASSERT_EQUAL(2, lmk_get_list_size(&logger2->handler_ref_list));

    TMK_ASSERT_EQUAL(2, clh->nr_logger_ref);
    TMK_ASSERT_EQUAL(2, flh->nr_logger_ref);

    TMK_ASSERT_EQUAL(2, lmk_get_nr_handlers());
    TMK_ASSERT_EQUAL(2, lmk_get_nr_loggers());

    lmk_destroy();

}

/**
 * Verify that we cannot attach the same handler (ignored) but we can attach
 * similarly-typed log handlers.
 */
TMK_TEST(lmk_test_attach_same_handler) {

    lmk_logger *logger = lmk_get_logger("logger");
    TMK_ASSERT_NOT_NULL(logger);

    lmk_log_handler *clh1 = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh1);

    lmk_log_handler *clh2 = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh2);

    lmk_attach_log_handler(logger, clh1);
    lmk_attach_log_handler(logger, clh1);
    lmk_attach_log_handler(logger, clh2);

    TMK_ASSERT_EQUAL(1, clh1->nr_logger_ref);
    TMK_ASSERT_EQUAL(1, clh2->nr_logger_ref);
    lmk_dump_loggers();
    lmk_destroy();
}

/** Verify that when we a destroy a logger, it will no longer be referencing
 *  any log handlers and will be detached from the list of all loggers.
 */
TMK_TEST(lmk_test_destroy_logger) {
    lmk_logger *logger = lmk_get_logger("logger");
    TMK_ASSERT_NOT_NULL(logger);

    lmk_log_handler *clh1 = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh1);

    lmk_log_handler *clh2 = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh2);

    TMK_ASSERT(lmk_attach_log_handler(logger, clh1) == LMK_E_OK);
    TMK_ASSERT_EQUAL(1, clh1->nr_logger_ref);

    TMK_ASSERT(lmk_attach_log_handler(logger, clh2) == LMK_E_OK);

    TMK_ASSERT_EQUAL(1, clh1->nr_logger_ref);
    TMK_ASSERT_EQUAL(1, clh2->nr_logger_ref);

    lmk_destroy_logger(&logger);

    TMK_ASSERT_NULL(logger);
    TMK_ASSERT_EQUAL(0, clh1->nr_logger_ref);
    TMK_ASSERT_EQUAL(0, clh2->nr_logger_ref);
    TMK_ASSERT_EQUAL(0, lmk_get_nr_loggers());

    lmk_destroy();
}

/**
 * Verify that when the framework is destroyed, all loggers and log handlers
 * are destroyed as well.
 *
 */
TMK_TEST(lmk_test_destroy_loggers) {
    lmk_logger *logger1 = lmk_get_logger("logger1");
    TMK_ASSERT_NOT_NULL(logger1);

    lmk_logger *logger2 = lmk_get_logger("logger2");
    TMK_ASSERT_NOT_NULL(logger2);

    lmk_logger *logger3 = lmk_get_logger("logger3");
    TMK_ASSERT_NOT_NULL(logger3);

    lmk_log_handler *clh = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh);

    lmk_log_handler *flh = lmk_get_file_log_handler("flh", "flh.log");
    TMK_ASSERT_NOT_NULL(flh);

    TMK_ASSERT(lmk_attach_log_handler(logger1, clh) == LMK_E_OK);
    TMK_ASSERT(lmk_attach_log_handler(logger1, flh) == LMK_E_OK);
    TMK_ASSERT(lmk_attach_log_handler(logger2, clh) == LMK_E_OK);
    TMK_ASSERT(lmk_attach_log_handler(logger2, flh) == LMK_E_OK);

    TMK_ASSERT_EQUAL(2, lmk_get_list_size(&logger1->handler_ref_list));
    TMK_ASSERT_EQUAL(2, lmk_get_list_size(&logger2->handler_ref_list));

    TMK_ASSERT_EQUAL(2, clh->nr_logger_ref);
    TMK_ASSERT_EQUAL(2, flh->nr_logger_ref);

    TMK_ASSERT_EQUAL(2, lmk_get_nr_handlers());
    TMK_ASSERT_EQUAL(3, lmk_get_nr_loggers());

    lmk_destroy();

    TMK_ASSERT_EQUAL(0, lmk_get_nr_handlers());
    TMK_ASSERT_EQUAL(0, lmk_get_nr_loggers());

}

void *log_user_thread_routine1(void *arg) {
    lmk_logger *logger = NULL;
    int id = 0;
    static int nr_log_calls = 0;
    id = *((int*) arg);
    logger = lmk_get_logger("logger1");
    while (++nr_log_calls < 1024) {
        LMK_LOG_TRACE(logger, "[id=%d, calls=%d] what you see is what you see", id, nr_log_calls);
    }
    LMK_LOG_TRACE(logger, "[id=%d] Exiting");
}

void *log_user_thread_routine2(void *arg) {
    lmk_logger *logger = NULL;
    int id = 0;
    static int nr_log_calls = 0;
    id = *((int*) arg);
    logger = lmk_get_logger("logger1");
    while (++nr_log_calls < 1024) {
        LMK_LOG_TRACE(logger, "[id=%d, calls=%d] keep it simple, simple", id, nr_log_calls);
    }
    LMK_LOG_TRACE(logger, "[id=%d] Exiting");
}

void *log_user_thread_routine3(void *arg) {
    lmk_logger *logger = NULL;
    int id = 0;
    static int nr_log_calls = 0;
    id = *((int*) arg);
    logger = lmk_get_logger("logger2");
    while (++nr_log_calls < 1024) {
        LMK_LOG_TRACE(logger, "[id=%d, calls=%d] write once, write again", id, nr_log_calls);
    }
    LMK_LOG_TRACE(logger, "[id=%d] Exiting");
}

void *log_user_thread_routine4(void *arg) {
    lmk_logger *logger = NULL;
    int id = 0;
    static int nr_log_calls = 0;
    id = *((int*) arg);
    logger = lmk_get_logger("logger2");
    while (++nr_log_calls < 1024) {
        LMK_LOG_TRACE(logger, "[id=%d, calls=%d] the network is the network", id, nr_log_calls);
    }
    LMK_LOG_TRACE(logger, "[id=%d] Exiting");
}

/**
 * Note:
 *   Threads : t1, t2, t3, t4
 *   logger1 has file log handler, flh
 *   logger2 has file log handler, flh
 *   t1 and t2 use the same logger, logger1
 *   t3 and t4 use the same logger, logger2
 *   logger1 and logger2 share the same file log handler, flh
/*
 */

TMK_TEST(lmk_test_multiple_threads) {
    pthread_t t1;
    pthread_t t2;
    pthread_t t3;
    pthread_t t4;

    lmk_logger *logger1 = lmk_get_logger("logger1");
    TMK_ASSERT_NOT_NULL(logger1);

    lmk_logger *logger2 = lmk_get_logger("logger2");
    TMK_ASSERT_NOT_NULL(logger2);

    lmk_log_handler *flh = lmk_get_file_log_handler("flh", "flh.log");
    TMK_ASSERT_NOT_NULL(flh);

    lmk_log_handler *clh = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh);

    lmk_attach_log_handler(logger1, flh);
    lmk_attach_log_handler(logger2, flh);

    int p1 = 1;
    int p2 = 2;
    int p3 = 3;
    int p4 = 4;

    pthread_create(&t1, NULL, (void*) log_user_thread_routine1, (void*) &p1);
    pthread_create(&t2, NULL, (void*) log_user_thread_routine2, (void*) &p2);
    pthread_create(&t3, NULL, (void*) log_user_thread_routine3, (void*) &p3);
    pthread_create(&t4, NULL, (void*) log_user_thread_routine4, (void*) &p4);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    pthread_join(t4, NULL);

    TMK_ASSERT_EQUAL(1024 * 4, flh->nr_log_calls);
    lmk_destroy();

}

TMK_TEST(lmk_test_socket_log_handler) {
    lmk_logger *logger = lmk_get_logger("logger");
    TMK_ASSERT_NOT_NULL(logger);

    lmk_log_handler *slh = lmk_get_socket_log_handler("slh");
    TMK_ASSERT_NOT_NULL(slh);

    lmk_attach_log_listener(slh, "127.0.0.1", 6000);

    TMK_ASSERT_EQUAL(LMK_E_OK, lmk_attach_log_handler(logger, slh));
    TMK_ASSERT_EQUAL(1, slh->nr_logger_ref);

    LMK_LOG_TRACE(logger, "This is a trace log");
    LMK_LOG_INFO(logger, "This is a debug log");
    LMK_LOG_DEBUG(logger, "This is an info log");
    LMK_LOG_WARN(logger, "This is a warn log");
    LMK_LOG_ERROR(logger, "This is an error log");
    LMK_LOG_FATAL(logger, "This is a fatal log");
    
   TMK_ASSERT_EQUAL(6, slh->nr_log_calls);

    lmk_destroy();
}

TMK_TEST_FUNCTION_TABLE_START(test_function_table)
TMK_INCLUDE_TEST(lmk_test_create_and_destroy_logger) TMK_INCLUDE_TEST(lmk_test_null_logger)
TMK_INCLUDE_TEST(lmk_test_null_logger_name)
TMK_INCLUDE_TEST(lmk_test_get_existing_logger)
TMK_INCLUDE_TEST(lmk_test_logger_log_levels)
TMK_INCLUDE_TEST(lmk_test_create_handlers)
TMK_INCLUDE_TEST(lmk_test_destroy_handlers)
TMK_INCLUDE_TEST(lmk_test_destroy_handler_with_logger_references)
TMK_INCLUDE_TEST(lmk_test_attach_handlers_to_multiple_loggers)
TMK_INCLUDE_TEST(lmk_test_attach_same_handler)
TMK_INCLUDE_TEST(lmk_test_destroy_logger)
TMK_INCLUDE_TEST(lmk_test_destroy_loggers)
TMK_INCLUDE_TEST(lmk_test_multiple_threads)
TMK_INCLUDE_TEST(lmk_test_console_log_handler)
TMK_INCLUDE_TEST(lmk_test_socket_log_handler)
TMK_TEST_FUNCTION_TABLE_END

int main(int argc, char **argv) {
    tmk_run_tests(test_function_table, NULL, NULL);
    return EXIT_SUCCESS;
}

