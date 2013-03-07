## Process this file with automake to produce Makefile.in

noinst_PROGRAMS = my_app socket_log_handler
my_app_SOURCES = my_app.c
socket_log_handler_SOURCES = socket_log_handler.c

clean-local:
	rm -f *.log    

LDADD =	../../src/main/liblogmoko.a
