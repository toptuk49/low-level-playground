#include "file_table.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "path_utils.h"

#define INITIAL_CAPACITY 16

struct FileTable
{
  FileEntry* entries;
  DWord count;
  DWord capacity;
  QWord total_original_size;
  QWord total_compressed_size;
};

FileTable* file_table_create(void)
{
  FileTable* table = (FileTable*)malloc(sizeof(FileTable));
  if (table == NULL)
  {
    printf("Произошла ошибка при выделении памяти!\n");
    return NULL;
  }

  table->entries = (FileEntry*)malloc(sizeof(FileEntry) * INITIAL_CAPACITY);
  if (table->entries == NULL)
  {
    printf("Произошла ошибка при выделении памяти!\n");
    free(table);
    return NULL;
  }

  table->count = 0;
  table->capacity = INITIAL_CAPACITY;
  table->total_original_size = 0;
  table->total_compressed_size = 0;
  return table;
}

void file_table_destroy(FileTable* self)
{
  if (self == NULL)
  {
    return;
  }

  free(self->entries);
  free(self);
}

static Result file_table_resize(FileTable* self, DWord new_capacity)
{
  FileEntry* new_entries =
    (FileEntry*)realloc(self->entries, sizeof(FileEntry) * new_capacity);
  if (new_entries == NULL)
  {
    printf("Произошла ошибка при перевыделении памяти!\n");
    return RESULT_MEMORY_ERROR;
  }

  self->entries = new_entries;
  self->capacity = new_capacity;
  return RESULT_OK;
}

Result file_table_add_file(FileTable* self, const char* filename, QWord size)
{
  if (self == NULL || filename == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  if (self->count >= self->capacity)
  {
    Result result = file_table_resize(self, self->capacity * 2);
    if (result != RESULT_OK)
    {
      return result;
    }
  }

  FileEntry* entry = &self->entries[self->count];
  strncpy(entry->filename, filename, FILENAME_LIMIT - 1);
  entry->filename[FILENAME_LIMIT - 1] = '\0';
  entry->original_size = size;
  entry->compressed_size = size;
  entry->offset = 0;
  entry->crc = 0;

  self->count++;
  self->total_original_size += size;
  self->total_compressed_size += size;

  return RESULT_OK;
}

static Result file_table_add_directory_recursive(FileTable* self,
                                                 const char* dirname)
{
  DIR* dir = opendir(dirname);
  if (dir == NULL)
  {
    return RESULT_IO_ERROR;
  }

  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL)
  {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
    {
      continue;
    }

    char* full_path = path_utils_join(dirname, entry->d_name);
    if (full_path == NULL)
    {
      closedir(dir);
      return RESULT_MEMORY_ERROR;
    }

    if (entry->d_type == DT_DIR)
    {
      Result result = file_table_add_directory_recursive(self, full_path);
      if (result != RESULT_OK)
      {
        free(full_path);
        closedir(dir);
        return result;
      }
    }
    else
    {
      struct stat stats;
      if (stat(full_path, &stats) == 0)
      {
        file_table_add_file(self, full_path, stats.st_size);
      }
    }

    free(full_path);
  }

  closedir(dir);
  return RESULT_OK;
}

Result file_table_add_directory(FileTable* self, const char* dirname)
{
  if (self == NULL || dirname == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  return file_table_add_directory_recursive(self, dirname);
}

DWord file_table_get_count(const FileTable* self)
{
  return self ? self->count : 0;
}

const FileEntry* file_table_get_entry(const FileTable* self, DWord index)
{
  if (self == NULL || index >= self->count)
  {
    return NULL;
  }

  return &self->entries[index];
}

QWord file_table_get_total_size(const FileTable* self)
{
  return self ? self->total_original_size : 0;
}

Result file_table_write(const FileTable* self, File* file)
{
  if (self == NULL || file == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  Result result =
    file_write_bytes(file, (const Byte*)&self->count, sizeof(self->count));
  if (result != RESULT_OK)
  {
    return result;
  }

  for (DWord i = 0; i < self->count; i++)
  {
    result =
      file_write_bytes(file, (const Byte*)&self->entries[i], sizeof(FileEntry));
    if (result != RESULT_OK)
    {
      return result;
    }
  }

  return RESULT_OK;
}

Result file_table_read(FileTable* self, File* file)
{
  if (self == NULL || file == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  Result result =
    file_read_bytes_size(file, (Byte*)&self->count, sizeof(self->count));
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при чтении количества файлов в таблице файлов!\n");
    return result;
  }

  self->entries = (FileEntry*)malloc(sizeof(FileEntry) * self->count);
  if (self->entries == NULL)
  {
    printf("Произошла ошибка при выделении памяти!\n");
    return RESULT_MEMORY_ERROR;
  }

  for (DWord i = 0; i < self->count; i++)
  {
    result =
      file_read_bytes_size(file, (Byte*)&self->entries[i], sizeof(FileEntry));
    if (result != RESULT_OK)
    {
      printf(
        "Произошла ошибка при чтении конкретного файла из таблицы файлов!\n");
      free(self->entries);
      return result;
    }
  }

  return RESULT_OK;
}
