CC=gcc
CFLAGS=-shared -fPIC -std=c99
BUILD_DIR=build

UNAME_S := $(shell uname)
LIB_DIR="$(shell pwd)/$(BUILD_DIR)"

ifeq ($(UNAME_S),Darwin)
	SONAME_FLAG = -Wl,-install_name,$(LIB_DIR)/
	SO_EXTENSION = so
else
	SONAME_FLAG = -Wl,-soname,
	SO_EXTENSION = so
endif

all: $(BUILD_DIR) libarguments.so libfile.so 

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

libarguments.so:
	$(CC) $(CFLAGS) arguments/arguments.c -o $(BUILD_DIR)/libarguments.so -Iarguments $(SONAME_FLAG)libarguments.so
libfile.so:
	$(CC) $(CFLAGS) file/file.c -o $(BUILD_DIR)/libfile.so -Ifile $(SONAME_FLAG)libfile.so
clean:
	rm $(BUILD_DIR)/*.$(SO_EXTENSION)
