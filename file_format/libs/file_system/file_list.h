#ifndef FILE_SYSTEM_FILE_LIST_H
#define FILE_SYSTEM_FILE_LIST_H

#include <stdbool.h>

#include "types.h"

typedef struct FileList FileList;

FileList* file_list_create(void);
void file_list_destroy(FileList* self);
Result file_list_add_file(FileList* self, const char* filename);
Result file_list_add_directory(FileList* self, const char* dirname,
                               bool recursive);
Size file_list_get_count(const FileList* self);
const char* file_list_get_path(const FileList* self, Size index);
bool file_list_is_directory(const FileList* self, Size index);
QWord file_list_get_size(const FileList* self, Size index);

#endif  // FILE_SYSTEM_FILE_LIST_H
