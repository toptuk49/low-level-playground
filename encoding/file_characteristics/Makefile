CC=gcc
CFLAGS=-I. -std=c99
LIB_DIR=../libs/build
ABS_LIB_DIR=$(shell cd $(LIB_DIR) && pwd)

all: file_characteristics set_ld_library_path

file_characteristics: file_characteristics.o statistics.o summary.o
	$(CC) file_characteristics.o statistics.o summary.o -L$(LIB_DIR) -larguments -lfile -o file_characteristics $(CFLAGS) -Wl,-rpath,$(ABS_LIB_DIR)

file_characteristics.o:
	$(CC) -c file_characteristics.c -o file_characteristics.o -I../libs/arguments -I../libs/file $(CFLAGS)

statistics.o: statistics.h
	$(CC) -c statistics.c -o statistics.o $(CFLAGS)

summary.o: summary.h
	$(CC) -c summary.c -o summary.o $(CFLAGS)

set_ld_library_path:
	CURRENT_DIR="$(shell pwd)$(LIB_DIR)"
	@echo "To execute the program, add utils library to LD_LIBRARY_PATH:"
	@echo "export LD_LIBRARY_PATH=$$CURRENT_DIR:$$LD_LIBRARY_PATH"

clean:
	rm *.o file_characteristics
