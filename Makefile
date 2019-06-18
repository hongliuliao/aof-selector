.PHONY: all test clean deps test-deps tags 

CXX=g++
CXXFLAGS += -g -Wall
LDFLAGS += -pthread

DEPS_INCLUDE_PATH=-I deps/hiredis/ 
DEPS_LIB_PATH=deps/hiredis/libhiredis.a

#CU_TEST_INC=-I deps/googletest/googletest/include
#CU_TEST_LIB=deps/googletest/googletest/make/gtest_main.a

SRC_INCLUDE_PATH=-I src
OUTPUT_BIN_PATH=output/bin/aof-selector

objects := $(patsubst %.c,%.o,$(wildcard src/*.c))

all: aof-selector

prepare: 
	mkdir -p output/bin

tags:
	ctags -R /usr/include src deps

deps: prepare 
	make -C deps/hiredis

test-deps:
	make -C deps/googletest/googletest/make

aof-selector: deps $(objects)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(DEPS_INCLUDE_PATH) $(SRC_INCLUDE_PATH) $(objects) $(DEPS_LIB_PATH) -o output/bin/$@

test: libehttp.a http_server_test sim_parser_test issue5_server threadpool_test string_utils_test simple_config_test simple_log_test epoll_socket_test

%.o: %.c
	$(CXX) -c $(CXXFLAGS) $(DEPS_INCLUDE_PATH) $(SRC_INCLUDE_PATH) $< -o $@

http_server_test: test/http_server_test.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(DEPS_INCLUDE_PATH) $(OUTPUT_INCLUDE_PATH) $< $(OUTPUT_LIB_PATH) $(DEPS_LIB_PATH) -o output/bin/$@

clean:
	rm -rf *.gcda
	rm -rf *.gcno
	rm -rf src/*.o
	rm -rf src/*.gcda
	rm -rf src/*.gcno
	rm -rf output/*
	make -C deps/hiredis clean
