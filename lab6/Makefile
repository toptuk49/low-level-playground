CC=gcc
CFLAGS=-I. -pthread -std=c99
SOURCE_DIR=src
BUILD_DIR=build
LIB_DIR=libs
ABS_BUILD_DIR=$(shell cd $(BUILD_DIR) && pwd)
DYNAMIC_CFLAGS=-Wl,-rpath,$(ABS_BUILD_DIR)

all: $(BUILD_DIR) $(BUILD_DIR)/client $(BUILD_DIR)/server $(BUILD_DIR)/servers.txt

$(BUILD_DIR):
	mkdir -p build

$(BUILD_DIR)/client: $(SOURCE_DIR)/client.c
	$(CC) $(CFLAGS) $< -o $@ -L$(BUILD_DIR) -lclient_arguments -lstring_converter -lsafe_modulo_multiplication $(DYNAMIC_CFLAGS)

$(BUILD_DIR)/server: $(SOURCE_DIR)/server.c
	$(CC) $(CFLAGS) $< -o $@ -L$(BUILD_DIR) -lserver_arguments -lstring_converter -lsafe_modulo_multiplication $(DYNAMIC_CFLAGS)

$(BUILD_DIR)/servers.txt: $(SOURCE_DIR)/servers.txt
	cp $< $@

clean:
	rm $(BUILD_DIR)/*
