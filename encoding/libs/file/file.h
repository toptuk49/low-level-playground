#ifndef FILE_FILE_H
#define FILE_FILE_H

#include <stdio.h>

#include "types.h"

typedef struct File File;

File *file_create(const char *path);
void file_destroy(File *self);

Result file_open(File *self);
Result file_close(File *self);
Result file_read_bytes(File *self);

const Byte *file_get_buffer(const File *self);
Size file_get_size(const File *self);
const char *file_get_path(const File *self);

#endif // FILE_FILE_H
