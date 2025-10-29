#include "file_list.h"

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "path_utils.h"

#define INITIAL_CAPACITY 16

typedef struct
{
  char* path;
  bool is_directory;
  QWord size;
} FileEntry;

struct FileList
{
  FileEntry* entries;
  Size count;
  Size capacity;
};

FileList* file_list_create(void)
{
  FileList* list = (FileList*)malloc(sizeof(FileList));
  if (list == NULL)
  {
    printf("Произошла ошибка при выделении памяти!\n");
    return NULL;
  }

  list->capacity = INITIAL_CAPACITY;
  list->count = 0;
  list->entries = (FileEntry*)malloc(sizeof(FileEntry) * list->capacity);

  if (list->entries == NULL)
  {
    printf("Произошла ошибка при выделении памяти!\n");
    free(list);
    return NULL;
  }

  return list;
}

void file_list_destroy(FileList* self)
{
  if (self == NULL)
  {
    return;
  }

  for (Size i = 0; i < self->count; i++)
  {
    free(self->entries[i].path);
  }

  free(self->entries);
  free(self);
}

static Result file_list_resize(FileList* self, Size new_capacity)
{
  FileEntry* new_entries =
    (FileEntry*)realloc(self->entries, sizeof(FileEntry) * new_capacity);
  if (new_entries == NULL)
  {
    printf("Произошла ошибка при расширении памяти!\n");
    return RESULT_MEMORY_ERROR;
  }

  self->entries = new_entries;
  self->capacity = new_capacity;
  return RESULT_OK;
}

Result file_list_add_file(FileList* self, const char* filename)
{
  if (self == NULL || filename == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  if (self->count >= self->capacity)
  {
    Result result = file_list_resize(self, self->capacity * 2);
    if (result != RESULT_OK)
    {
      return result;
    }
  }

  FileEntry* entry = &self->entries[self->count];

  entry->path = (char*)malloc(strlen(filename) + 1);
  if (entry->path == NULL)
  {
    printf("Произошла ошибка при выделении памяти!\n");
    return RESULT_MEMORY_ERROR;
  }

  strcpy(entry->path, filename);

  entry->is_directory = path_utils_is_directory(filename);
  entry->size = entry->is_directory ? 0 : path_utils_get_file_size(filename);

  self->count++;
  return RESULT_OK;
}

Result file_list_add_directory(FileList* self, const char* dirname,
                               bool recursive)
{
  if (self == NULL || dirname == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  Result result = file_list_add_file(self, dirname);
  if (result != RESULT_OK)
  {
    return result;
  }

  DIR* dir = opendir(dirname);
  if (!dir)
  {
    printf("Не удалось открыть директорию '%s'!\n", dirname);
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
      return RESULT_IO_ERROR;
    }

    if (entry->d_type == DT_DIR && recursive)
    {
      file_list_add_directory(self, full_path, recursive);
    }
    else
    {
      file_list_add_file(self, full_path);
    }

    free(full_path);
  }

  closedir(dir);
  return RESULT_OK;
}

Size file_list_get_count(const FileList* self)
{
  return self ? self->count : 0;
}

const char* file_list_get_path(const FileList* self, Size index)
{
  if (self == NULL || index >= self->count)
  {
    return NULL;
  }

  return self->entries[index].path;
}

bool file_list_is_directory(const FileList* self, Size index)
{
  if (!self || index >= self->count)
  {
    return false;
  }

  return self->entries[index].is_directory;
}

QWord file_list_get_size(const FileList* self, Size index)
{
  if (!self || index >= self->count)
  {
    return 0;
  }

  return self->entries[index].size;
}
