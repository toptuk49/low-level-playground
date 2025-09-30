CC=gcc
CFLAGS=-I. -std=c99
DYNAMIC_LIB=-shared -fPIC
BUILD_DIR=../build
UNAME=$(shell uname)

ifeq ($(UNAME),Darwin)
	SONAME_FLAG=-Wl,-install_name,@rpath/
	SO_EXTENSION=so
else
	SONAME_FLAG=-Wl,-soname,
	SO_EXTENSION=so
endif

all: $(BUILD_DIR) $(BUILD_DIR)/libutils.$(SO_EXTENSION) $(BUILD_DIR)/libfind_min_max.$(SO_EXTENSION)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/libutils.$(SO_EXTENSION): $(BUILD_DIR)/utils.o
	$(CC) $(CFLAGS) $(DYNAMIC_LIB) $< -o $@ $(SONAME_FLAG)libutils.$(SO_EXTENSION)

$(BUILD_DIR)/utils.o: utils.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/libfind_min_max.$(SO_EXTENSION): $(BUILD_DIR)/find_min_max.o
	$(CC) $(CFLAGS) $(DYNAMIC_LIB) $< -o $@ $(SONAME_FLAG)libfind_min_max.$(SO_EXTENSION)

$(BUILD_DIR)/find_min_max.o: find_min_max.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(BUILD_DIR)/*.o $(BUILD_DIR)/*.$(SO_EXTENSION)
