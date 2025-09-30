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

all: $(BUILD_DIR) $(BUILD_DIR)/libclient_arguments.a $(BUILD_DIR)/libserver_arguments.a $(BUILD_DIR)/libstring_converter.$(SO_EXTENSION) $(BUILD_DIR)/libsafe_modulo_multiplication.$(SO_EXTENSION)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/libclient_arguments.a: $(BUILD_DIR)/client_arguments.o
	ar rcs $@ $<

$(BUILD_DIR)/client_arguments.o: client_arguments.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/libserver_arguments.a: $(BUILD_DIR)/server_arguments.o
	ar rcs $@ $<

$(BUILD_DIR)/server_arguments.o: server_arguments.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/libstring_converter.$(SO_EXTENSION): string_converter.c
	$(CC) $(CFLAGS) $(DYNAMIC_LIB) $< -o $@ $(SONAME_FLAG)libstring_converter.$(SO_EXTENSION)

$(BUILD_DIR)/libsafe_modulo_multiplication.$(SO_EXTENSION): safe_modulo_multiplication.c
	$(CC) $(CFLAGS) $(DYNAMIC_LIB) $< -o $@ $(SONAME_FLAG)libsafe_modulo_multiplication.$(SO_EXTENSION)

clean:
	rm $(BUILD_DIR)/*.$(SO_EXTENSION) $(BUILD_DIR)/*.a $(BUILD_DIR)/*.o
