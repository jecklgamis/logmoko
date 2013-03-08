#include <stdlib.h>
#include <logmoko.h>

int main(int argc, char *argv[]) {
    lmk_logger *logger;
    lmk_log_handler *handler;

    logger = lmk_get_logger("logger");
    handler = lmk_get_socket_log_handler("socket");
    lmk_attach_log_listener(handler, "localhost", 9000);
    lmk_attach_log_handler(logger, handler);
    
    LMK_LOG_TRACE(logger, "this is a trace log");
    LMK_LOG_DEBUG(logger, "this is a debug log");
    LMK_LOG_INFO(logger, "this is an info log");
    LMK_LOG_WARN(logger, "this is a warn log");
    LMK_LOG_ERROR(logger, "this is an error log");
    LMK_LOG_FATAL(logger, "this is a fatal log");  
    
    lmk_destroy();
    return EXIT_SUCCESS;
}