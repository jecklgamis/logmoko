#include <stdlib.h>
#include <pthread.h>
#include "logmoko.h"
#include "testmoko.h"

/* Verify normal creation and destruction of logger 
 *  Verify default log level
 */
TMK_TEST(lmk_test_create_and_destroy_logger) {

    struct lmk_logger *logger = lmk_get_logger("logger");
    TMK_ASSERT_NOT_NULL(logger);
    TMK_ASSERT_EQUAL(LMK_LOG_LEVEL_INFO, lmk_get_log_level(logger));
    TMK_ASSERT_EQUAL(1, lmk_get_nr_loggers());
    TMK_ASSERT_EQUAL(0, lmk_get_nr_handlers());

    lmk_destroy_logger(&logger);
    TMK_ASSERT_NULL(logger);
    TMK_ASSERT_EQUAL(0, lmk_get_nr_loggers());
    lmk_destroy();
}

/* Verify that logging using a null logger is allowed (no exceptions of what kind)
 */
TMK_TEST(lmk_test_null_logger) {
    LMK_LOG_TRACE(NULL, "This is a trace log");
    lmk_destroy();
}

/* Verify that a null logger name is not allowed */
TMK_TEST(lmk_test_null_logger_name) {
    struct lmk_logger *logger = lmk_get_logger(NULL);
    TMK_ASSERT_NULL(logger);
    lmk_destroy();
}

/* Verify that the same logger is returned if names are similar */
TMK_TEST(lmk_test_get_existing_logger) {
    struct lmk_logger *logger = lmk_get_logger("logger");
    TMK_ASSERT_NOT_NULL(logger);

    struct lmk_logger *logger2 = lmk_get_logger("logger");
    TMK_ASSERT_NOT_NULL(logger2);
    TMK_ASSERT_EQUAL_PTRS(logger, logger2);
    lmk_destroy();
}

/* Verify that log requests are delegated to attached loggers */
TMK_TEST(lmk_test_logger_log_levels) {
    struct lmk_logger *logger = lmk_get_logger("logger");
    TMK_ASSERT_NOT_NULL(logger);
    lmk_set_log_level(logger, LMK_LOG_LEVEL_TRACE);

    struct lmk_log_handler *clh = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh);

    clh->nr_log_calls = 0;
    lmk_set_handler_log_level(clh, LMK_LOG_LEVEL_TRACE);

    lmk_attach_log_handler(logger, clh);

    LMK_LOG_TRACE(logger, "This is a trace log");
    LMK_LOG_INFO(logger, "This is an info log");
    LMK_LOG_DEBUG(logger, "This is a debug log");
    LMK_LOG_WARN(logger, "This is a warn log");
    LMK_LOG_ERROR(logger, "This is an error log");

    TMK_ASSERT_EQUAL(5, clh->nr_log_calls);

    clh->nr_log_calls = 0;
    lmk_set_handler_log_level(clh, LMK_LOG_LEVEL_DEBUG);

    LMK_LOG_TRACE(logger, "This is a trace log");
    LMK_LOG_INFO(logger, "This is an info log");
    LMK_LOG_DEBUG(logger, "This is a debug log");
    LMK_LOG_WARN(logger, "This is a warn log");
    LMK_LOG_ERROR(logger, "This is an error log");


    TMK_ASSERT_EQUAL(4, clh->nr_log_calls);

    clh->nr_log_calls = 0;
    lmk_set_handler_log_level(clh, LMK_LOG_LEVEL_INFO);

    LMK_LOG_TRACE(logger, "This is a trace log");
    LMK_LOG_INFO(logger, "This is an info log");
    LMK_LOG_DEBUG(logger, "This is a debug log");
    LMK_LOG_WARN(logger, "This is a warn log");
    LMK_LOG_ERROR(logger, "This is an error log");

    TMK_ASSERT_EQUAL(3, clh->nr_log_calls);


    clh->nr_log_calls = 0;
    lmk_set_handler_log_level(clh, LMK_LOG_LEVEL_WARN);

    LMK_LOG_TRACE(logger, "This is a trace log");
    LMK_LOG_INFO(logger, "This is an info log");
    LMK_LOG_DEBUG(logger, "This is a debug log");
    LMK_LOG_WARN(logger, "This is a warn log");
    LMK_LOG_ERROR(logger, "This is an error log");

    TMK_ASSERT_EQUAL(2, clh->nr_log_calls);

    clh->nr_log_calls = 0;
    lmk_set_handler_log_level(clh, LMK_LOG_LEVEL_ERROR);

    LMK_LOG_TRACE(logger, "This is a trace log");
    LMK_LOG_INFO(logger, "This is an info log");
    LMK_LOG_DEBUG(logger, "This is a debug log");
    LMK_LOG_WARN(logger, "This is a warn log");
    LMK_LOG_ERROR(logger, "This is an error log");

    TMK_ASSERT_EQUAL(1, clh->nr_log_calls);

    clh->nr_log_calls = 0;
    lmk_set_handler_log_level(clh, LMK_LOG_LEVEL_OFF);
    LMK_LOG_TRACE(logger, "This is a trace log");
    LMK_LOG_INFO(logger, "This is an info log");
    LMK_LOG_DEBUG(logger, "This is a debug log");
    LMK_LOG_WARN(logger, "This is a warn log");
    LMK_LOG_ERROR(logger, "This is an error log");

    TMK_ASSERT_EQUAL(0, clh->nr_log_calls);

    /* Verify that even if the log request is not filtered out by
     * the logger, still, it will not be logged if the log handler level
     * is more restrictive than the logger.
     */
    clh->nr_log_calls = 0;
    lmk_set_log_level(logger, LMK_LOG_LEVEL_WARN);
    lmk_set_handler_log_level(clh, LMK_LOG_LEVEL_ERROR);
    LMK_LOG_WARN(logger, "This is a warn log");

    TMK_ASSERT_EQUAL(0, clh->nr_log_calls);

    /*
     * Verify that if the handler log level is less restrictive than the logger
     * log level, requested log levels greater than that of the logger will
     * be logged.
     */
    clh->nr_log_calls = 0;

    lmk_set_log_level(logger, LMK_LOG_LEVEL_WARN);
    lmk_set_handler_log_level(clh, LMK_LOG_LEVEL_INFO);

    LMK_LOG_WARN(logger, "This is a warn log");

    TMK_ASSERT_EQUAL(1, clh->nr_log_calls);

    lmk_destroy();
}

/*
 * Verify that we can successfully create different log handler types
 */
TMK_TEST(lmk_test_create_handlers) {
    struct lmk_log_handler *clh = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh);
    TMK_ASSERT_EQUAL(LMK_LOG_HANDLER_TYPE_CONSOLE, clh->type);
    TMK_ASSERT_TRUE(clh->initialized);
    struct lmk_log_handler *flh = lmk_get_file_log_handler("flh", "flh.log");
    TMK_ASSERT_NOT_NULL(flh);
    TMK_ASSERT_EQUAL(LMK_LOG_HANDLER_TYPE_FILE, flh->type);
    TMK_ASSERT_TRUE(flh->initialized);
    lmk_destroy();
}

/*
 * Verify we can only create one and only one console log handler.
 */
TMK_TEST(lmk_test_console_log_handler) {
    struct lmk_log_handler *clh1 = lmk_get_console_log_handler();
    struct lmk_log_handler *clh2 = lmk_get_console_log_handler();
    TMK_ASSERT_EQUAL_PTRS(clh1, clh2);
    lmk_destroy();
}

/*
 * Verify that we can successfully destroy handlers
 */
TMK_TEST(lmk_test_destroy_handlers) {
    struct lmk_log_handler *clh = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh);
    struct lmk_log_handler *flh = lmk_get_file_log_handler("flh", "flh.log");
    TMK_ASSERT_NOT_NULL(flh);
    lmk_destroy_log_handler(&clh);
    TMK_ASSERT_NULL(clh);
    lmk_destroy_log_handler(&flh);
    TMK_ASSERT_NULL(flh);
    TMK_ASSERT_EQUAL(0, lmk_get_nr_handlers());
    lmk_destroy();
}

/*
 * Verify that a handler cannot be destroyed if there are more than one
 * loggers referencing it.
 */
TMK_TEST(lmk_test_destroy_handler_with_logger_references) {
    struct lmk_logger *logger = lmk_get_logger("logger");
    TMK_ASSERT_NOT_NULL(logger);

    struct lmk_log_handler *clh = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh);

    lmk_dump_loggers();

    TMK_ASSERT_EQUAL(LMK_E_OK, lmk_attach_log_handler(logger, clh));
    TMK_ASSERT_EQUAL(1, lmk_get_nr_loggers());
    TMK_ASSERT_EQUAL(1, lmk_get_nr_handlers());

    TMK_ASSERT_EQUAL(LMK_E_NG, lmk_destroy_log_handler(&clh));
    TMK_ASSERT_EQUAL(1, lmk_get_nr_loggers());
    TMK_ASSERT_EQUAL(1, lmk_get_nr_handlers());

    lmk_destroy();
}

/*
 * Verify log handler attachment to multiple loggers.
 */
TMK_TEST(lmk_test_attach_handlers_to_multiple_loggers) {
    struct lmk_logger *logger1 = lmk_get_logger("logger1");
    TMK_ASSERT_NOT_NULL(logger1);

    struct lmk_logger *logger2 = lmk_get_logger("logger2");
    TMK_ASSERT_NOT_NULL(logger2);

    struct lmk_log_handler *clh = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh);

    struct lmk_log_handler *flh = lmk_get_file_log_handler("flh", "flh.log");
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

/*
 * Verify that we cannot attach the same handler (ignored) but we can attach
 * similarly-typed log handlers.
 */
TMK_TEST(lmk_test_attach_same_handler) {

    struct lmk_logger *logger = lmk_get_logger("logger");
    TMK_ASSERT_NOT_NULL(logger);

    struct lmk_log_handler *clh1 = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh1);

    struct lmk_log_handler *clh2 = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh2);

    lmk_attach_log_handler(logger, clh1);
    lmk_attach_log_handler(logger, clh1);
    lmk_attach_log_handler(logger, clh2);

    TMK_ASSERT_EQUAL(1, clh1->nr_logger_ref);
    TMK_ASSERT_EQUAL(1, clh2->nr_logger_ref);
    lmk_dump_loggers();
    lmk_destroy();
}

/* Verify that when we a destroy a logger, it will no longer be referencing
 *  any log handlers and will be detached from the list of all loggers.
 */
TMK_TEST(lmk_test_destroy_logger) {
    struct lmk_logger *logger = lmk_get_logger("logger");
    TMK_ASSERT_NOT_NULL(logger);

    struct lmk_log_handler *clh1 = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh1);

    struct lmk_log_handler *clh2 = lmk_get_console_log_handler();
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

/*
 * Verify that when the framework is destroyed, all loggers and log handlers
 * are destroyed as well.
 *
 */
TMK_TEST(lmk_test_destroy_loggers) {
    struct lmk_logger *logger1 = lmk_get_logger("logger1");
    TMK_ASSERT_NOT_NULL(logger1);

    struct lmk_logger *logger2 = lmk_get_logger("logger2");
    TMK_ASSERT_NOT_NULL(logger2);

    struct lmk_logger *logger3 = lmk_get_logger("logger3");
    TMK_ASSERT_NOT_NULL(logger3);

    struct lmk_log_handler *clh = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh);

    struct lmk_log_handler *flh = lmk_get_file_log_handler("flh", "flh.log");
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
    struct lmk_logger *logger = NULL;
    int id = 0;
    static int nr_log_calls = 0;
    id = *((int *) arg);
    logger = lmk_get_logger("logger1");
    while (++nr_log_calls < 1024) {
        LMK_LOG_TRACE(logger, "[id=%d, calls=%d] what you see is what you see", id, nr_log_calls);
    }
    LMK_LOG_TRACE(logger, "[id=%d] Exiting");
    return NULL;
}

void *log_user_thread_routine2(void *arg) {
    struct lmk_logger *logger = NULL;
    int id = 0;
    static int nr_log_calls = 0;
    id = *((int *) arg);
    logger = lmk_get_logger("logger1");
    while (++nr_log_calls < 1024) {
        LMK_LOG_TRACE(logger, "[id=%d, calls=%d] keep it simple, simple", id, nr_log_calls);
    }
    LMK_LOG_TRACE(logger, "[id=%d] Exiting");
    return NULL;
}

void *log_user_thread_routine3(void *arg) {
    struct lmk_logger *logger = NULL;
    int id = 0;
    static int nr_log_calls = 0;
    id = *((int *) arg);
    logger = lmk_get_logger("logger2");
    while (++nr_log_calls < 1024) {
        LMK_LOG_TRACE(logger, "[id=%d, calls=%d] write once, write again", id, nr_log_calls);
    }
    LMK_LOG_TRACE(logger, "[id=%d] Exiting");
    return NULL;
}

void *log_user_thread_routine4(void *arg) {
    struct lmk_logger *logger = NULL;
    int id = 0;
    static int nr_log_calls = 0;
    id = *((int *) arg);
    logger = lmk_get_logger("logger2");
    while (++nr_log_calls < 1024) {
        LMK_LOG_TRACE(logger, "[id=%d, calls=%d] the network is the network", id, nr_log_calls);
    }
    LMK_LOG_TRACE(logger, "[id=%d] Exiting");
    return NULL;
}

/*
 * Note:
 *   Threads : t1, t2, t3, t4
 *   logger1 has file log handler, flh
 *   logger2 has file log handler, flh
 *   t1 and t2 use the same logger, logger1
 *   t3 and t4 use the same logger, logger2
 *   logger1 and logger2 share the same file log handler, flh
 */

TMK_TEST(lmk_test_multiple_threads) {
    pthread_t t1;
    pthread_t t2;
    pthread_t t3;
    pthread_t t4;

    struct lmk_logger *logger1 = lmk_get_logger("logger1");
    TMK_ASSERT_NOT_NULL(logger1);

    struct lmk_logger *logger2 = lmk_get_logger("logger2");
    TMK_ASSERT_NOT_NULL(logger2);

    struct lmk_log_handler *flh = lmk_get_file_log_handler("flh", "flh.log");
    TMK_ASSERT_NOT_NULL(flh);
    lmk_set_handler_log_level(flh, LMK_LOG_LEVEL_TRACE);

//    struct lmk_log_handler *clh = lmk_get_console_log_handler();
//    TMK_ASSERT_NOT_NULL(clh);
//    lmk_set_handler_log_level(clh, LMK_LOG_LEVEL_TRACE);

    lmk_set_log_level(logger1, LMK_LOG_LEVEL_TRACE);
    lmk_set_log_level(logger2, LMK_LOG_LEVEL_TRACE);

    lmk_attach_log_handler(logger1, flh);
    lmk_attach_log_handler(logger2, flh);

    int p1 = 1;
    int p2 = 2;
    int p3 = 3;
    int p4 = 4;

    pthread_create(&t1, NULL, (void *) log_user_thread_routine1, (void *) &p1);
    pthread_create(&t2, NULL, (void *) log_user_thread_routine2, (void *) &p2);
    pthread_create(&t3, NULL, (void *) log_user_thread_routine3, (void *) &p3);
    pthread_create(&t4, NULL, (void *) log_user_thread_routine4, (void *) &p4);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    pthread_join(t4, NULL);

    TMK_ASSERT_TRUE(flh->nr_log_calls <= 4*1024);
    TMK_ASSERT_TRUE(flh->nr_log_calls > 0);
    lmk_destroy();
}

TMK_TEST(lmk_test_socket_log_handler) {
    struct lmk_logger *logger = lmk_get_logger("logger");
    TMK_ASSERT_NOT_NULL(logger);

    struct lmk_log_handler *slh = lmk_get_socket_log_handler("slh");
    TMK_ASSERT_NOT_NULL(slh);
    lmk_set_handler_log_level(slh, LMK_LOG_LEVEL_TRACE);
    lmk_set_log_level(logger, LMK_LOG_LEVEL_TRACE);

    lmk_attach_log_listener(slh, "127.0.0.1", 6000);

    TMK_ASSERT_EQUAL(LMK_E_OK, lmk_attach_log_handler(logger, slh));
    TMK_ASSERT_EQUAL(1, slh->nr_logger_ref);

    LMK_LOG_TRACE(logger, "This is a trace log");
    LMK_LOG_INFO(logger, "This is a debug log");
    LMK_LOG_DEBUG(logger, "This is an info log");
    LMK_LOG_WARN(logger, "This is a warn log");
    LMK_LOG_ERROR(logger, "This is an error log");

    TMK_ASSERT_EQUAL(5, slh->nr_log_calls);

    lmk_destroy();
}

TMK_TEST(lmk_test_detach_log_handler) {
    struct lmk_logger *logger = lmk_get_logger("logger");
    TMK_ASSERT_NOT_NULL(logger);

    struct lmk_log_handler *clh = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh);

    struct lmk_log_handler *flh = lmk_get_file_log_handler("flh", "flh.log");
    TMK_ASSERT_NOT_NULL(flh);

    TMK_ASSERT_EQUAL(LMK_E_OK, lmk_attach_log_handler(logger, clh));
    TMK_ASSERT_EQUAL(LMK_E_OK, lmk_attach_log_handler(logger, flh));
    TMK_ASSERT_EQUAL(2, lmk_get_list_size(&logger->handler_ref_list));
    TMK_ASSERT_EQUAL(1, clh->nr_logger_ref);
    TMK_ASSERT_EQUAL(1, flh->nr_logger_ref);

    TMK_ASSERT_EQUAL(LMK_E_OK, lmk_detach_log_handler(logger, clh));
    TMK_ASSERT_EQUAL(1, lmk_get_list_size(&logger->handler_ref_list));
    TMK_ASSERT_EQUAL(0, clh->nr_logger_ref);
    TMK_ASSERT_EQUAL(1, flh->nr_logger_ref);

    TMK_ASSERT_EQUAL(LMK_E_OK, lmk_detach_log_handler(logger, flh));
    TMK_ASSERT_EQUAL(0, lmk_get_list_size(&logger->handler_ref_list));
    TMK_ASSERT_EQUAL(0, flh->nr_logger_ref);

    lmk_destroy();
}

TMK_TEST(lmk_test_detach_unattached_handler) {
    struct lmk_logger *logger = lmk_get_logger("logger");
    TMK_ASSERT_NOT_NULL(logger);

    struct lmk_log_handler *clh = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh);

    TMK_ASSERT_EQUAL(LMK_E_NG, lmk_detach_log_handler(logger, clh));

    lmk_destroy();
}

TMK_TEST(lmk_test_detach_null_args) {
    struct lmk_logger *logger = lmk_get_logger("logger");
    struct lmk_log_handler *clh = lmk_get_console_log_handler();

    TMK_ASSERT_EQUAL(LMK_E_NG, lmk_detach_log_handler(NULL, clh));
    TMK_ASSERT_EQUAL(LMK_E_NG, lmk_detach_log_handler(logger, NULL));

    lmk_destroy();
}

TMK_TEST(lmk_test_get_handler_log_level) {
    struct lmk_log_handler *clh = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh);

    lmk_set_handler_log_level(clh, LMK_LOG_LEVEL_WARN);
    TMK_ASSERT_EQUAL(LMK_LOG_LEVEL_WARN, lmk_get_handler_log_level(clh));

    lmk_set_handler_log_level(clh, LMK_LOG_LEVEL_ERROR);
    TMK_ASSERT_EQUAL(LMK_LOG_LEVEL_ERROR, lmk_get_handler_log_level(clh));

    lmk_set_handler_log_level(clh, LMK_LOG_LEVEL_TRACE);
    TMK_ASSERT_EQUAL(LMK_LOG_LEVEL_TRACE, lmk_get_handler_log_level(clh));

    lmk_destroy();
}

TMK_TEST(lmk_test_get_log_level_str) {
    TMK_ASSERT_EQUAL_STRING("TRACE",   lmk_get_log_level_str(LMK_LOG_LEVEL_TRACE));
    TMK_ASSERT_EQUAL_STRING("DEBUG",   lmk_get_log_level_str(LMK_LOG_LEVEL_DEBUG));
    TMK_ASSERT_EQUAL_STRING("INFO",    lmk_get_log_level_str(LMK_LOG_LEVEL_INFO));
    TMK_ASSERT_EQUAL_STRING("WARN",    lmk_get_log_level_str(LMK_LOG_LEVEL_WARN));
    TMK_ASSERT_EQUAL_STRING("ERROR",   lmk_get_log_level_str(LMK_LOG_LEVEL_ERROR));
    TMK_ASSERT_EQUAL_STRING("OFF",     lmk_get_log_level_str(LMK_LOG_LEVEL_OFF));
    TMK_ASSERT_EQUAL_STRING("UNKNOWN", lmk_get_log_level_str(LMK_LOG_LEVEL_UNKNOWN));
    TMK_ASSERT_EQUAL_STRING("UNKNOWN", lmk_get_log_level_str(-1));
    lmk_destroy();
}

TMK_TEST(lmk_test_get_log_handler_type_str) {
    TMK_ASSERT_EQUAL_STRING("CONSOLE", lmk_get_log_handler_type_str(LMK_LOG_HANDLER_TYPE_CONSOLE));
    TMK_ASSERT_EQUAL_STRING("FILE",    lmk_get_log_handler_type_str(LMK_LOG_HANDLER_TYPE_FILE));
    TMK_ASSERT_EQUAL_STRING("SOCKET",  lmk_get_log_handler_type_str(LMK_LOG_HANDLER_TYPE_SOCKET));
    TMK_ASSERT_EQUAL_STRING("SYSLOG",  lmk_get_log_handler_type_str(LMK_LOG_HANDLER_TYPE_SYSLOG));
    lmk_destroy();
}

TMK_TEST(lmk_test_search_logger_by_name) {
    struct lmk_logger *logger = lmk_get_logger("my-logger");
    TMK_ASSERT_NOT_NULL(logger);

    struct lmk_logger *found = lmk_search_logger_by_name("my-logger");
    TMK_ASSERT_NOT_NULL(found);
    TMK_ASSERT_EQUAL_PTRS(logger, found);

    TMK_ASSERT_NULL(lmk_search_logger_by_name("nonexistent"));
    TMK_ASSERT_NULL(lmk_search_logger_by_name(NULL));

    lmk_destroy();
}

TMK_TEST(lmk_test_search_log_handler_by_name) {
    struct lmk_log_handler *clh = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh);

    struct lmk_log_handler *flh = lmk_get_file_log_handler("my-file-handler", "test.log");
    TMK_ASSERT_NOT_NULL(flh);

    struct lmk_log_handler *found_clh = lmk_search_log_handler_by_name("console");
    TMK_ASSERT_NOT_NULL(found_clh);
    TMK_ASSERT_EQUAL_PTRS(clh, found_clh);

    struct lmk_log_handler *found_flh = lmk_search_log_handler_by_name("my-file-handler");
    TMK_ASSERT_NOT_NULL(found_flh);
    TMK_ASSERT_EQUAL_PTRS(flh, found_flh);

    TMK_ASSERT_NULL(lmk_search_log_handler_by_name("nonexistent"));
    TMK_ASSERT_NULL(lmk_search_log_handler_by_name(NULL));

    lmk_destroy();
}

TMK_TEST(lmk_test_find_handler) {
    struct lmk_logger *logger = lmk_get_logger("logger");
    TMK_ASSERT_NOT_NULL(logger);

    struct lmk_log_handler *clh = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh);

    struct lmk_log_handler *flh = lmk_get_file_log_handler("flh", "flh.log");
    TMK_ASSERT_NOT_NULL(flh);

    TMK_ASSERT_NULL(lmk_find_handler(logger, "console"));

    lmk_attach_log_handler(logger, clh);
    lmk_attach_log_handler(logger, flh);

    TMK_ASSERT_EQUAL_PTRS(clh, lmk_find_handler(logger, "console"));
    TMK_ASSERT_EQUAL_PTRS(flh, lmk_find_handler(logger, "flh"));
    TMK_ASSERT_NULL(lmk_find_handler(logger, "nonexistent"));
    TMK_ASSERT_NULL(lmk_find_handler(NULL, "console"));
    TMK_ASSERT_NULL(lmk_find_handler(logger, NULL));

    lmk_destroy();
}

TMK_TEST(lmk_test_syslog_log_handler) {
    struct lmk_log_handler *slh = lmk_get_syslog_log_handler("syslog-handler", "logmoko-test", 0);
    TMK_ASSERT_NOT_NULL(slh);
    TMK_ASSERT_EQUAL(LMK_LOG_HANDLER_TYPE_SYSLOG, slh->type);
    TMK_ASSERT_TRUE(slh->initialized);

    struct lmk_logger *logger = lmk_get_logger("logger");
    TMK_ASSERT_NOT_NULL(logger);
    lmk_set_log_level(logger, LMK_LOG_LEVEL_TRACE);
    lmk_set_handler_log_level(slh, LMK_LOG_LEVEL_TRACE);
    lmk_attach_log_handler(logger, slh);

    slh->nr_log_calls = 0;
    LMK_LOG_INFO(logger, "Test syslog message");
    LMK_LOG_WARN(logger, "Test syslog warning");

    TMK_ASSERT_EQUAL(2, slh->nr_log_calls);

    lmk_destroy();
}

TMK_TEST(lmk_test_get_log_level_null_logger) {
    TMK_ASSERT_EQUAL(LMK_LOG_LEVEL_UNKNOWN, lmk_get_log_level(NULL));
    lmk_destroy();
}

TMK_TEST(lmk_test_set_log_level_invalid) {
    struct lmk_logger *logger = lmk_get_logger("logger");
    TMK_ASSERT_NOT_NULL(logger);

    lmk_set_log_level(logger, LMK_LOG_LEVEL_INFO);
    lmk_set_log_level(logger, -1);
    TMK_ASSERT_EQUAL(LMK_LOG_LEVEL_INFO, lmk_get_log_level(logger));

    lmk_set_log_level(logger, 999);
    TMK_ASSERT_EQUAL(LMK_LOG_LEVEL_INFO, lmk_get_log_level(logger));

    lmk_destroy();
}

TMK_TEST(lmk_test_logger_filters_before_handler) {
    struct lmk_logger *logger = lmk_get_logger("logger");
    TMK_ASSERT_NOT_NULL(logger);

    struct lmk_log_handler *clh = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh);

    lmk_set_log_level(logger, LMK_LOG_LEVEL_ERROR);
    lmk_set_handler_log_level(clh, LMK_LOG_LEVEL_TRACE);
    lmk_attach_log_handler(logger, clh);

    clh->nr_log_calls = 0;
    LMK_LOG_TRACE(logger, "trace");
    LMK_LOG_DEBUG(logger, "debug");
    LMK_LOG_INFO(logger,  "info");
    LMK_LOG_WARN(logger,  "warn");
    LMK_LOG_ERROR(logger, "error");

    TMK_ASSERT_EQUAL(1, clh->nr_log_calls);

    lmk_destroy();
}

/* --- Socket handler -------------------------------------------------------- */

TMK_TEST(lmk_test_socket_log_handler_create) {
    struct lmk_log_handler *slh = lmk_get_socket_log_handler("slh");
    TMK_ASSERT_NOT_NULL(slh);
    TMK_ASSERT_EQUAL(LMK_LOG_HANDLER_TYPE_SOCKET, slh->type);
    TMK_ASSERT_TRUE(slh->initialized);
    TMK_ASSERT_EQUAL(1, lmk_get_nr_handlers());
    lmk_destroy();
}

TMK_TEST(lmk_test_socket_log_handler_singleton) {
    struct lmk_log_handler *slh1 = lmk_get_socket_log_handler("slh");
    struct lmk_log_handler *slh2 = lmk_get_socket_log_handler("slh");
    TMK_ASSERT_NOT_NULL(slh1);
    TMK_ASSERT_EQUAL_PTRS(slh1, slh2);
    TMK_ASSERT_EQUAL(1, lmk_get_nr_handlers());

    struct lmk_log_handler *slh3 = lmk_get_socket_log_handler("slh-other");
    TMK_ASSERT_NOT_NULL(slh3);
    TMK_ASSERT_EQUAL(2, lmk_get_nr_handlers());

    lmk_destroy();
}

TMK_TEST(lmk_test_socket_log_handler_null_name) {
    struct lmk_log_handler *slh = lmk_get_socket_log_handler(NULL);
    TMK_ASSERT_NULL(slh);
    TMK_ASSERT_EQUAL(0, lmk_get_nr_handlers());
    lmk_destroy();
}

TMK_TEST(lmk_test_socket_log_handler_no_listener) {
    struct lmk_logger *logger = lmk_get_logger("logger");
    TMK_ASSERT_NOT_NULL(logger);

    struct lmk_log_handler *slh = lmk_get_socket_log_handler("slh");
    TMK_ASSERT_NOT_NULL(slh);
    lmk_set_log_level(logger, LMK_LOG_LEVEL_TRACE);
    lmk_set_handler_log_level(slh, LMK_LOG_LEVEL_TRACE);
    lmk_attach_log_handler(logger, slh);

    slh->nr_log_calls = 0;
    LMK_LOG_INFO(logger, "message without a listener");
    LMK_LOG_WARN(logger, "another message without a listener");

    TMK_ASSERT_EQUAL(2, slh->nr_log_calls);
    lmk_destroy();
}

TMK_TEST(lmk_test_socket_log_handler_level_filtering) {
    struct lmk_logger *logger = lmk_get_logger("logger");
    TMK_ASSERT_NOT_NULL(logger);

    struct lmk_log_handler *slh = lmk_get_socket_log_handler("slh");
    TMK_ASSERT_NOT_NULL(slh);
    lmk_set_log_level(logger, LMK_LOG_LEVEL_TRACE);
    lmk_set_handler_log_level(slh, LMK_LOG_LEVEL_WARN);
    lmk_attach_log_handler(logger, slh);

    slh->nr_log_calls = 0;
    LMK_LOG_TRACE(logger, "trace");
    LMK_LOG_DEBUG(logger, "debug");
    LMK_LOG_INFO(logger, "info");
    LMK_LOG_WARN(logger, "warn");
    LMK_LOG_ERROR(logger, "error");

    TMK_ASSERT_EQUAL(2, slh->nr_log_calls);
    lmk_destroy();
}

TMK_TEST(lmk_test_socket_log_handler_multiple_listeners) {
    struct lmk_logger *logger = lmk_get_logger("logger");
    TMK_ASSERT_NOT_NULL(logger);

    struct lmk_log_handler *slh = lmk_get_socket_log_handler("slh");
    TMK_ASSERT_NOT_NULL(slh);
    lmk_set_log_level(logger, LMK_LOG_LEVEL_TRACE);
    lmk_set_handler_log_level(slh, LMK_LOG_LEVEL_TRACE);

    lmk_attach_log_listener(slh, "127.0.0.1", 6000);
    lmk_attach_log_listener(slh, "127.0.0.1", 6001);
    lmk_attach_log_handler(logger, slh);

    slh->nr_log_calls = 0;
    LMK_LOG_INFO(logger, "multicast message");
    TMK_ASSERT_EQUAL(1, slh->nr_log_calls);

    lmk_destroy();
}

TMK_TEST(lmk_test_socket_log_handler_destroy) {
    struct lmk_log_handler *slh = lmk_get_socket_log_handler("slh");
    TMK_ASSERT_NOT_NULL(slh);
    TMK_ASSERT_EQUAL(1, lmk_get_nr_handlers());

    TMK_ASSERT_EQUAL(LMK_E_OK, lmk_destroy_log_handler(&slh));
    TMK_ASSERT_NULL(slh);
    TMK_ASSERT_EQUAL(0, lmk_get_nr_handlers());

    lmk_destroy();
}

/* --- File handler ---------------------------------------------------------- */

TMK_TEST(lmk_test_file_log_handler_singleton) {
    struct lmk_log_handler *flh1 = lmk_get_file_log_handler("flh", "flh.log");
    struct lmk_log_handler *flh2 = lmk_get_file_log_handler("flh", "flh.log");
    TMK_ASSERT_NOT_NULL(flh1);
    TMK_ASSERT_EQUAL_PTRS(flh1, flh2);
    TMK_ASSERT_EQUAL(1, lmk_get_nr_handlers());

    struct lmk_log_handler *flh3 = lmk_get_file_log_handler("flh-other", "other.log");
    TMK_ASSERT_NOT_NULL(flh3);
    TMK_ASSERT_EQUAL(2, lmk_get_nr_handlers());

    lmk_destroy();
}

TMK_TEST(lmk_test_file_log_handler_null_name) {
    struct lmk_log_handler *flh = lmk_get_file_log_handler(NULL, "flh.log");
    TMK_ASSERT_NULL(flh);
    TMK_ASSERT_EQUAL(0, lmk_get_nr_handlers());
    lmk_destroy();
}

TMK_TEST(lmk_test_file_log_handler_defaults) {
    struct lmk_log_handler *handler = lmk_get_file_log_handler("flh", "flh.log");
    TMK_ASSERT_NOT_NULL(handler);

    struct lmk_file_log_handler *flh = (struct lmk_file_log_handler *) handler;
    TMK_ASSERT_EQUAL(LMK_DEFAULT_MAX_FILE_SIZE, flh->max_file_size);
    TMK_ASSERT_EQUAL(LMK_DEFAULT_MAX_BACKUP_FILES, flh->max_backup_files);

    lmk_destroy();
}

TMK_TEST(lmk_test_set_log_rotation) {
    struct lmk_log_handler *handler = lmk_get_file_log_handler("flh", "flh.log");
    TMK_ASSERT_NOT_NULL(handler);

    lmk_set_log_rotation(handler, 5L * 1024 * 1024, 3);

    struct lmk_file_log_handler *flh = (struct lmk_file_log_handler *) handler;
    TMK_ASSERT_EQUAL(5L * 1024 * 1024, flh->max_file_size);
    TMK_ASSERT_EQUAL(3, flh->max_backup_files);

    lmk_destroy();
}

TMK_TEST(lmk_test_set_log_rotation_ignored_for_non_file_handler) {
    struct lmk_log_handler *clh = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh);

    lmk_set_log_rotation(clh, 1024, 2);

    lmk_destroy();
}

TMK_TEST(lmk_test_set_log_filename) {
    struct lmk_log_handler *handler = lmk_get_file_log_handler("flh", "original.log");
    TMK_ASSERT_NOT_NULL(handler);

    lmk_set_log_filename(handler, "renamed.log");

    struct lmk_file_log_handler *flh = (struct lmk_file_log_handler *) handler;
    TMK_ASSERT_EQUAL_STRING("renamed.log", flh->filename);

    lmk_destroy();
}

/* --- Handler API edge cases ------------------------------------------------ */

TMK_TEST(lmk_test_destroy_log_handler_null) {
    TMK_ASSERT_EQUAL(LMK_E_NG, lmk_destroy_log_handler(NULL));
    struct lmk_log_handler *null_handler = NULL;
    TMK_ASSERT_EQUAL(LMK_E_NG, lmk_destroy_log_handler(&null_handler));
    lmk_destroy();
}

TMK_TEST(lmk_test_get_handler_log_level_null) {
    TMK_ASSERT_EQUAL(LMK_LOG_LEVEL_UNKNOWN, lmk_get_handler_log_level(NULL));
    lmk_destroy();
}

TMK_TEST(lmk_test_set_handler_log_level_invalid) {
    struct lmk_log_handler *clh = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh);

    lmk_set_handler_log_level(clh, LMK_LOG_LEVEL_WARN);
    lmk_set_handler_log_level(clh, -1);
    TMK_ASSERT_EQUAL(LMK_LOG_LEVEL_WARN, lmk_get_handler_log_level(clh));

    lmk_set_handler_log_level(clh, 999);
    TMK_ASSERT_EQUAL(LMK_LOG_LEVEL_WARN, lmk_get_handler_log_level(clh));

    lmk_destroy();
}

/* --- Format ---------------------------------------------------------------- */

TMK_TEST(lmk_test_get_format_fn) {
    TMK_ASSERT_NULL(lmk_get_format_fn(NULL));
    TMK_ASSERT_NULL(lmk_get_format_fn("default"));
    TMK_ASSERT_NULL(lmk_get_format_fn("DEFAULT"));
    TMK_ASSERT_NOT_NULL(lmk_get_format_fn("simple"));
    TMK_ASSERT_NOT_NULL(lmk_get_format_fn("SIMPLE"));
    TMK_ASSERT_NOT_NULL(lmk_get_format_fn("json"));
    TMK_ASSERT_NOT_NULL(lmk_get_format_fn("JSON"));
    TMK_ASSERT_NULL(lmk_get_format_fn("nonexistent"));
    lmk_destroy();
}

static int g_custom_format_calls = 0;
static void custom_format_fn(char *out, size_t out_size,
                              int log_level, const char *level_str,
                              const char *timestamp,
                              const char *file_name, int line_no,
                              const char *handler_name,
                              const char *message) {
    g_custom_format_calls++;
    snprintf(out, out_size, "CUSTOM: %s\n", message);
}

TMK_TEST(lmk_test_set_log_format) {
    struct lmk_logger *logger = lmk_get_logger("logger");
    TMK_ASSERT_NOT_NULL(logger);

    struct lmk_log_handler *flh = lmk_get_file_log_handler("flh", "flh.log");
    TMK_ASSERT_NOT_NULL(flh);
    lmk_set_log_level(logger, LMK_LOG_LEVEL_TRACE);
    lmk_set_handler_log_level(flh, LMK_LOG_LEVEL_TRACE);
    lmk_attach_log_handler(logger, flh);

    g_custom_format_calls = 0;
    lmk_set_log_format(flh, custom_format_fn);
    TMK_ASSERT_EQUAL_PTRS(custom_format_fn, flh->format_fn);

    LMK_LOG_INFO(logger, "formatted message");
    LMK_LOG_WARN(logger, "another formatted message");

    lmk_destroy();
    TMK_ASSERT_EQUAL(2, g_custom_format_calls);
}

TMK_TEST(lmk_test_set_log_format_null_clears_format) {
    struct lmk_log_handler *clh = lmk_get_console_log_handler();
    TMK_ASSERT_NOT_NULL(clh);

    lmk_set_log_format(clh, custom_format_fn);
    TMK_ASSERT_EQUAL_PTRS(custom_format_fn, clh->format_fn);

    lmk_set_log_format(clh, NULL);
    TMK_ASSERT_NULL(clh->format_fn);

    lmk_destroy();
}

/* --- Config ---------------------------------------------------------------- */

TMK_TEST(lmk_test_get_config) {
    struct lmk_config *cfg = lmk_get_config();
    TMK_ASSERT_NOT_NULL(cfg);
    TMK_ASSERT_TRUE(cfg->log_buffer_size > 0);
    TMK_ASSERT_TRUE(cfg->ring_buffer_size > 0);
    lmk_destroy();
}

TMK_TEST(lmk_test_file_write_perf) {
    struct lmk_logger *logger = lmk_get_logger("logger");
    struct lmk_log_handler *flh = lmk_get_file_log_handler("file-handler", "perf-test.log");
    lmk_attach_log_handler(logger, flh);
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    int nr_iterations = 10*10000;
    for(int a=0;a <nr_iterations;a++) {
        LMK_LOG_INFO(logger, "This is an info log %d", a);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) +  (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Elapsed time: %.6f seconds writing %d logs\n", elapsed,nr_iterations);
}

TMK_TEST_FUNCTION_TABLE_START(test_function_table)
    TMK_INCLUDE_TEST(lmk_test_create_and_destroy_logger)
    TMK_INCLUDE_TEST(lmk_test_null_logger)
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
    TMK_INCLUDE_TEST(lmk_test_detach_log_handler)
    TMK_INCLUDE_TEST(lmk_test_detach_unattached_handler)
    TMK_INCLUDE_TEST(lmk_test_detach_null_args)
    TMK_INCLUDE_TEST(lmk_test_get_handler_log_level)
    TMK_INCLUDE_TEST(lmk_test_get_log_level_str)
    TMK_INCLUDE_TEST(lmk_test_get_log_handler_type_str)
    TMK_INCLUDE_TEST(lmk_test_search_logger_by_name)
    TMK_INCLUDE_TEST(lmk_test_search_log_handler_by_name)
    TMK_INCLUDE_TEST(lmk_test_find_handler)
    TMK_INCLUDE_TEST(lmk_test_syslog_log_handler)
    TMK_INCLUDE_TEST(lmk_test_get_log_level_null_logger)
    TMK_INCLUDE_TEST(lmk_test_set_log_level_invalid)
    TMK_INCLUDE_TEST(lmk_test_logger_filters_before_handler)
    TMK_INCLUDE_TEST(lmk_test_socket_log_handler_create)
    TMK_INCLUDE_TEST(lmk_test_socket_log_handler_singleton)
    TMK_INCLUDE_TEST(lmk_test_socket_log_handler_null_name)
    TMK_INCLUDE_TEST(lmk_test_socket_log_handler_no_listener)
    TMK_INCLUDE_TEST(lmk_test_socket_log_handler_level_filtering)
    TMK_INCLUDE_TEST(lmk_test_socket_log_handler_multiple_listeners)
    TMK_INCLUDE_TEST(lmk_test_socket_log_handler_destroy)
    TMK_INCLUDE_TEST(lmk_test_file_log_handler_singleton)
    TMK_INCLUDE_TEST(lmk_test_file_log_handler_null_name)
    TMK_INCLUDE_TEST(lmk_test_file_log_handler_defaults)
    TMK_INCLUDE_TEST(lmk_test_set_log_rotation)
    TMK_INCLUDE_TEST(lmk_test_set_log_rotation_ignored_for_non_file_handler)
    TMK_INCLUDE_TEST(lmk_test_set_log_filename)
    TMK_INCLUDE_TEST(lmk_test_destroy_log_handler_null)
    TMK_INCLUDE_TEST(lmk_test_get_handler_log_level_null)
    TMK_INCLUDE_TEST(lmk_test_set_handler_log_level_invalid)
    TMK_INCLUDE_TEST(lmk_test_get_format_fn)
    TMK_INCLUDE_TEST(lmk_test_set_log_format)
    TMK_INCLUDE_TEST(lmk_test_set_log_format_null_clears_format)
    TMK_INCLUDE_TEST(lmk_test_get_config)
    TMK_INCLUDE_TEST(lmk_test_file_write_perf)
TMK_TEST_FUNCTION_TABLE_END

void tmk_setup() {
    lmk_init();
    TMK_ASSERT_EQUAL(0, lmk_get_nr_loggers());
    TMK_ASSERT_EQUAL(0, lmk_get_nr_handlers());
}

void tmk_teardown() {
    lmk_destroy();
    TMK_ASSERT_EQUAL(0, lmk_get_nr_loggers());
    TMK_ASSERT_EQUAL(0, lmk_get_nr_handlers());
}

int main(int argc, char **argv) {
    tmk_run_tests(test_function_table, tmk_setup, tmk_teardown);
    return EXIT_SUCCESS;
}

