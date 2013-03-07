
## Process this file with automake to produce Makefile.in

noinst_PROGRAMS = logmoko_test

logmoko_test_SOURCES = logmoko_test.c testmoko.c  testmoko_exception.c  

noinst_HEADERS =  testmoko.h testmoko_exception.h

check : logmoko_test 
	./logmoko_test

clean-local:
	rm -f *.log

LDADD =	../../src/main/liblogmoko.a
AM_CPPFLAGS = -I $(top_srcdir)/src  -I $(srcdir)



