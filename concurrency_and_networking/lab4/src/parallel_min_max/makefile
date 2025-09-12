CC=gcc
CFLAGS=-I. -std=c99 # Standard to work with 64-bit numbers

all: parallel_min_max set_ld_library_path

parallel_min_max: utils.so find_min_max.o parallel_min_max.o utils.h find_min_max.h
	$(CC) -o parallel_min_max find_min_max.o parallel_min_max.o -L. -lutils $(CFLAGS)

set_ld_library_path:
	CURRENT_DIR=$(shell pwd)
	@echo "To execute the program, add utils library to LD_LIBRARY_PATH:"
	@echo "export LD_LIBRARY_PATH=$$CURRENT_DIR:$$LD_LIBRARY_PATH"

utils.so: utils.h
	$(CC) -shared -fPIC -o libutils.so utils.c $(CFLAGS)

find_min_max.o: utils.h find_min_max.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

parallel_min_max.o: parallel_min_max.c
	$(CC) -o parallel_min_max.o -c parallel_min_max.c $(CFLAGS)

clean:
	rm *.o *.so parallel_min_max
