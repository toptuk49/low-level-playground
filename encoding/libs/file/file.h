#ifndef FILE_H
#define FILE_H

#include <stdio.h>

typedef struct {
  FILE *descriptor;
  unsigned char *buffer;
  long size;
} File;

void openFile(File *file, char *path);
void readBytes(File *file);

#endif
