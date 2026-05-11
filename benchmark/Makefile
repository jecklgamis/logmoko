CC     = gcc
CXX    = g++
CFLAGS = -std=gnu2x -O2
CXXFLAGS = -std=c++23 -O2
LIBDEP = ../src/main/liblogmoko.a

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
  INCLUDES     = -I../src/main -I../src -I/opt/homebrew/include -I.
  LIBS         = $(LIBDEP) /opt/homebrew/lib/libzlog.a -lpthread
  SPDLOG_LIBS  = /opt/homebrew/lib/libspdlog.a /opt/homebrew/lib/libfmt.a -lpthread
  ALL_LIBS     = $(LIBDEP) /opt/homebrew/lib/libzlog.a \
                 /opt/homebrew/lib/libspdlog.a /opt/homebrew/lib/libfmt.a \
                 -L/opt/homebrew/lib -lg3log -llog4c -lpthread
else
  INCLUDES     = -I../src/main -I../src -I.
  LIBS         = $(LIBDEP) -lzlog -lpthread
  SPDLOG_LIBS  = -lspdlog -lfmt -lpthread
  ALL_LIBS     = $(LIBDEP) -lzlog -lspdlog -lfmt -lg3log -llog4c -lpthread
endif

all: bench bench_ringbuf bench_spdlog bench_all

bench: bench.c $(LIBDEP)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

bench_ringbuf: bench_ringbuf.c $(LIBDEP)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

bench_spdlog: bench_spdlog.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(SPDLOG_LIBS)

log_c.o: log_c.c log.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

bench_logmoko.o: bench_logmoko.c bench_logmoko.h $(LIBDEP)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

bench_all: bench_all.cpp bench_logmoko.o log_c.o $(LIBDEP)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< bench_logmoko.o log_c.o $(ALL_LIBS)

$(LIBDEP):
	$(MAKE) -C ../src/main

run: bench
	./bench

run-ringbuf: bench_ringbuf
	./bench_ringbuf

run-spdlog: bench_spdlog
	./bench_spdlog

run-all: bench_all
	./bench_all

clean:
	rm -f bench bench_ringbuf bench_spdlog bench_all log_c.o bench_logmoko.o
	rm -f *.log *.log.*

.PHONY: all run run-ringbuf run-spdlog run-all clean
