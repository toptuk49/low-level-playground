CC=gcc
CFLAGS=-I. -std=c99
BUILD_DIR=build
SOURCE_DIR=src
LIB_DIR=libs
ABS_BUILD_DIR=$(shell cd $(BUILD_DIR) && pwd)
DYNAMIC_CFLAGS=-Wl,-rpath,$(ABS_BUILD_DIR)

all: $(BUILD_DIR) $(BUILD_DIR)/parallel_min_max $(BUILD_DIR)/run_sequential_min_max $(BUILD_DIR)/sequential_min_max

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/parallel_min_max: $(BUILD_DIR)/parallel_min_max.o
	$(CC) $(CFLAGS) $< -o $@ -L$(BUILD_DIR) -lutils -lfind_min_max $(DYNAMIC_CFLAGS)

$(BUILD_DIR)/parallel_min_max.o: $(SOURCE_DIR)/parallel_min_max.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/run_sequential_min_max: $(BUILD_DIR)/run_sequential_min_max.o
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/run_sequential_min_max.o: $(SOURCE_DIR)/run_sequential_min_max.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/sequential_min_max: $(BUILD_DIR)/sequential_min_max.o
	$(CC) $(CFLAGS) $< -o $@ -L$(BUILD_DIR) -lutils -lfind_min_max $(DYNAMIC_CFLAGS)

$(BUILD_DIR)/sequential_min_max.o: $(SOURCE_DIR)/sequential_min_max.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(BUILD_DIR)/*
