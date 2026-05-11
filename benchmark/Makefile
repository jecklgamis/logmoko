CC     = gcc
CXX    = g++
CFLAGS = -std=gnu2x -O2
CXXFLAGS = -std=c++17 -O2
LIBDEP = ../src/main/liblogmoko.a

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
  INCLUDES    = -I../src/main -I../src -I/opt/homebrew/include
  LIBS        = $(LIBDEP) /opt/homebrew/lib/libzlog.a -lpthread
  SPDLOG_LIBS = /opt/homebrew/lib/libspdlog.a /opt/homebrew/lib/libfmt.a -lpthread
else
  INCLUDES    = -I../src/main -I../src
  LIBS        = $(LIBDEP) -lzlog -lpthread
  SPDLOG_LIBS = -lspdlog -lfmt -lpthread
endif

all: bench bench_ringbuf bench_spdlog bench_all

bench: bench.c $(LIBDEP)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

bench_ringbuf: bench_ringbuf.c $(LIBDEP)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

bench_spdlog: bench_spdlog.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(SPDLOG_LIBS)

bench_all: bench_all.cpp $(LIBDEP)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(LIBS) $(SPDLOG_LIBS)

$(LIBDEP):
	$(MAKE) -C ../src/main

run: bench
	./bench

run-ringbuf: bench_ringbuf
	./bench_ringbuf

run-spdlog: bench_spdlog
	./bench_spdlog

clean:
	rm -f bench bench_ringbuf bench_spdlog
	rm -f *.log *.log.*

.PHONY: all run run-ringbuf run-spdlog clean
