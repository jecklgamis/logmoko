## Process this file with automake to produce Makefile.in

noinst_PROGRAMS = example_app example_app_socket
example_app_SOURCES = example_app.c
example_app_socket_SOURCES = example_app_socket.c

clean-local:
	rm -f *.log    

LDADD =	../../src/main/liblogmoko.a
