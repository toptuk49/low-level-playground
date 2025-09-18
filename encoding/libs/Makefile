CC=gcc
CFLAGS=-I. -std=c99

all: arguments

arguments: arguments.h
	$(CC) -shared -fPIC arguments.c -o libarguments.so $(CFLAGS)

clean:
	rm *.so
