#ifndef FILE_FILE_H
#define FILE_FILE_H

#include "types.h"

typedef struct File File;

File* file_create(const char* path);
void file_destroy(File* self);

Result file_open_for_read(File* self);
Result file_close(File* self);
Result file_read_bytes(File* self);
Result file_read_bytes_size(File* self, Byte* buffer, Size size_to_read);

Result file_open_for_write(File* self);
Result file_write_bytes(File* self, const Byte* data, Size data_size);
Result file_write_from_file(File* self, const File* source);

Result file_seek(File* self, long offset, int whence);
long file_tell(File* self);
Result file_read_at(File* self, Byte* buffer, Size size, QWord offset);

const Byte* file_get_buffer(const File* self);
Size file_get_size(const File* self);
const char* file_get_path(const File* self);

#endif  // FILE_FILE_H
