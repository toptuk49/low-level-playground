CC=gcc
CFLAGS=-I. -std=c99
LIB_DIR=../libs/build
ABS_LIB_DIR=$(shell cd $(LIB_DIR) && pwd)

all: analyze_unicode set_ld_library_path

analyze_unicode: analyze_unicode.o
	$(CC) analyze_unicode.o -L$(LIB_DIR) -larguments -lfile -o analyze_unicode $(CFLAGS) -Wl,-rpath,$(ABS_LIB_DIR)

analyze_unicode.o:
	$(CC) -c analyze_unicode.c -o analyze_unicode.o -I../libs/arguments -I../libs/file $(CFLAGS)

set_ld_library_path:
	CURRENT_DIR="$(shell pwd)$(LIB_DIR)"
	@echo "To execute the program, add utils library to LD_LIBRARY_PATH:"
	@echo "export LD_LIBRARY_PATH=$$CURRENT_DIR:$$LD_LIBRARY_PATH"

clean:
	rm *.o analyze_unicode 
