## Process this file with automake to produce Makefile.in

lib_LIBRARIES = liblogmoko.a

liblogmoko_a_SOURCES = \
    logmoko.c \
    logmoko_logger.c \
    logmoko_handler.c \
    logmoko_console_log_handler.c \
    logmoko_file_log_handler.c \
    logmoko_socket_log_handler.c \
    logmoko_serial_log_handler.c \
    logmoko_time.c \
    logmoko_config.c \
    logmoko_list.c \
    logmoko_mem.c \
    logmoko_connector.c
    
include_HEADERS =  \
    logmoko.h \
    logmoko_types.h \
    logmoko_time.h \
    logmoko_sync.h \
    logmoko_connector.h \
    logmoko_mem.h \
    logmoko_list.h


if DEBUG
AM_CPPFLAGS  =  -DLMK_DEBUG=1 -I $(srcdir)
else
AM_CPPFLAGS  = -I $(srcdir) 
endif
