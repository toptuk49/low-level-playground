CC=gcc
CFLAGS=-I. -std=c99 -pthread
BUILD_DIR=build
SOURCE_DIR=src
LIB_DIR=libs

all: $(BUILD_DIR) $(BUILD_DIR)/mutex $(BUILD_DIR)/without_mutex $(BUILD_DIR)/factorial $(BUILD_DIR)/deadlock

$(BUILD_DIR)/mutex: $(SOURCE_DIR)/mutex.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/without_mutex: $(SOURCE_DIR)/without_mutex.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/factorial: $(SOURCE_DIR)/factorial.c
	$(CC) $(CFLAGS) $< -o $@ -L$(BUILD_DIR) -larguments -I$(LIB_DIR)

$(BUILD_DIR)/deadlock: $(SOURCE_DIR)/deadlock.c
	$(CC) $< -o $@ $(CFLAGS)

clean:
	rm $(BUILD_DIR)/*
