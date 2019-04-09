#CXX = ${CXX}
DEBUG_FLAGS = -O2 -g -Wall -Wextra -DDEBUG -std=gnu++14
RELEASE_FLAGS= -O3 -march=native -Wall -Wextra -std=gnu++14 -flto #-static
CXXFLAGS=$(RELEASE_FLAGS)
DEPS=$(wildcard *.h) Makefile
OBJ=system.o cache.o prefetch.o
BUILD_DIR=$(shell pwd)
# Local
ZSTR_DIR=/home/vic/zstr/src

all: cache tags check cscope.out 

cache: main.cpp $(DEPS) $(OBJ)
	$(CXX) $(CXXFLAGS) -I$(ZSTR_DIR) -o cache main.cpp $(OBJ) -lz

%.o: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -o $@ -c $< 

tags: *.cpp *.h
	ctags *.cpp *.h

cscope.out: *.cpp *.h
	cscope -Rb

.PHONY: check
check:
	cppcheck --enable=all .

.PHONY: clean
clean:
	rm -f *.o cache tags cscope.out
