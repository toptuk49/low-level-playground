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

all: $(BUILD_DIR) $(BUILD_DIR)/libarguments.a 

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/libarguments.a: $(BUILD_DIR)/arguments.o
	ar rcs $@ $<

$(BUILD_DIR)/arguments.o: arguments.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(BUILD_DIR)/*.o $(BUILD_DIR)/*.a
