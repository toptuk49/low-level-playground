CC=gcc
CFLAGS=-I. -std=c99

all: file_characteristics

file_characteristics: file_characteristics.o arguments.so statistics.o summary.o
	$(CC) file_characteristics.o statistics.o summary.o -L. -larguments -o file_characteristics $(CFLAGS)

file_characteristics.o:
	$(CC) -c file_characteristics.c -o file_characteristics.o $(CFLAGS)

arguments.so: arguments.h
	$(CC) -shared -fPIC arguments.c -o libarguments.so $(CFLAGS)

statistics.o: statistics.h
	$(CC) -c statistics.c -o statistics.o $(CFLAGS)

summary.o: summary.h
	$(CC) -c summary.c -o summary.o $(CFLAGS)

clean:
	rm *.o *.so file_characteristics
