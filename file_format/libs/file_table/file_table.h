#ifndef FILE_TABLE_FILE_TABLE_H
#define FILE_TABLE_FILE_TABLE_H

#include "file.h"
#include "types.h"

#define FILENAME_LIMIT 256

typedef struct FileTable FileTable;

typedef struct
{
  char filename[FILENAME_LIMIT];
  uint64_t original_size;
  uint64_t compressed_size;
  uint64_t offset;
  uint32_t crc;
} FileEntry;

FileTable* file_table_create(void);
void file_table_destroy(FileTable* self);

Result file_table_add_file(FileTable* self, const char* filename,
                           uint64_t size);
Result file_table_add_directory(FileTable* self, const char* dirname);

uint32_t file_table_get_count(const FileTable* self);
const FileEntry* file_table_get_entry(const FileTable* self, uint32_t index);
uint64_t file_table_get_total_size(const FileTable* self);

Result file_table_write(const FileTable* self, File* file);
Result file_table_read(FileTable* self, File* file);

#endif  // FILE_TABLE_FILE_TABLE_H
