#include <stdio.h>
#include <stdlib.h>

#include "file.h"

#define BYTES_AMOUNT 1

void openFile(File *file, char *path) {
  file->descriptor = fopen(path, "rb");
  if (!file->descriptor) {
    printf("An error occured while opening the file!\n");
    exit(1);
  }
}

void readBytes(File *file) {
  fseek(file->descriptor, 0, SEEK_END);
  file->size = ftell(file->descriptor);
  rewind(file->descriptor);
  file->buffer = (unsigned char *)malloc(sizeof(unsigned char) * file->size);
  fread(file->buffer, BYTES_AMOUNT, file->size, file->descriptor);
}
