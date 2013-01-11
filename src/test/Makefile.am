
## Process this file with automake to produce Makefile.in

noinst_PROGRAMS = logmoko_test
logmoko_test_SOURCES = logmoko_test.c testmoko.c testmoko_exception.c

LDADD =  $(top_srcdir)/src/main/liblogmoko.a

if DEBUG
AM_CPPFLAGS  = 
else
AM_CPPFLAGS  =  
endif


