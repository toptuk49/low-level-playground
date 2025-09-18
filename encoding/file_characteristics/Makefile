CC=gcc
CFLAGS=-I. -std=c99

all: file_characteristics set_ld_library_path

file_characteristics: file_characteristics.o statistics.o summary.o
	$(CC) file_characteristics.o statistics.o summary.o -L../libs -larguments -o file_characteristics $(CFLAGS)

file_characteristics.o:
	$(CC) -c file_characteristics.c -o file_characteristics.o -I../libs $(CFLAGS)

statistics.o: statistics.h
	$(CC) -c statistics.c -o statistics.o $(CFLAGS)

summary.o: summary.h
	$(CC) -c summary.c -o summary.o $(CFLAGS)

set_ld_library_path:
	CURRENT_DIR="$(shell pwd)../libs"
	@echo "To execute the program, add utils library to LD_LIBRARY_PATH:"
	@echo "export LD_LIBRARY_PATH=$$CURRENT_DIR:$$LD_LIBRARY_PATH"

clean:
	rm *.o file_characteristics
