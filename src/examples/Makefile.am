## Process this file with automake to produce Makefile.in

noinst_PROGRAMS = example_app example_app_socket example_app_using_config example_app_socket_rate_limited
example_app_SOURCES = example_app.c
example_app_socket_SOURCES = example_app_socket.c
example_app_using_config_SOURCES = example_app_using_config.c
example_app_socket_rate_limited_SOURCES = example_app_socket_rate_limited.c

clean-local:
	rm -f *.log    

LDADD =	../../src/main/liblogmoko.a
