CC=gcc
CFLAGS=-I. -std=c99 -pthread

all: mutex without_mutex factorial deadlock

mutex:
	$(CC) mutex.c -o mutex $(CFLAGS)

without_mutex:
	$(CC) without_mutex.c -o without_mutex $(CFLAGS)

factorial: arguments.c arguments.h
	$(CC) factorial.c arguments.c -o factorial $(CFLAGS)

deadlock:
	$(CC) deadlock.c -o deadlock $(CFLAGS)

clean:
	rm mutex without_mutex factorial deadlock
