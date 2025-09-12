CC=gcc
CFLAGS=-std=c99 -pthread

LIB_DIR=../parallel_min_max
LIB_NAME=utils

all: parallel_sum set_ld_library_path

parallel_sum: parallel_sum.c libsum.a
	$(CC) parallel_sum.c libsum.a -L$(LIB_DIR) -l$(LIB_NAME) -I$(LIB_DIR) -I. -o parallel_sum $(CFLAGS)

libsum.a: sum.o
	ar r libsum.a sum.o

sum.o: sum.h
	$(CC) sum.c -c sum.o -I. $(CFLAGS)

set_ld_library_path:
	CURRENT_DIR="$(shell pwd)/$(LIB_DIR)"
	@echo "To execute the program, add utils library to LD_LIBRARY_PATH:"
	@echo "export LD_LIBRARY_PATH=$$CURRENT_DIR:$$LD_LIBRARY_PATH"

clean:
	rm *.o *.a parallel_sum
