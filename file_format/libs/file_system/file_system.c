#include "file_system.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "file.h"
#include "types.h"

#define INITIAL_CAPACITY 16
#define PATH_BUFFER_SIZE 4096

typedef struct
{
  char* path;
  bool is_directory;
  uint64_t size;
} FileEntry;

struct FileList
{
  FileEntry* entries;
  Size count;
  Size capacity;
};

struct DirectoryTree
{
  FileList* file_list;
  char* root_path;
};

static bool is_directory(const char* path);
static uint64_t get_file_size(const char* path);
static char* join_paths(const char* dir, const char* file);

// FileList implementation
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

  entry->path = (char*)malloc(strlen(filename));
  if (entry->path == NULL)
  {
    printf("Произошла ошибка при выделении памяти!\n");
    return RESULT_MEMORY_ERROR;
  }

  strcpy(entry->path, filename);

  entry->is_directory = is_directory(filename);
  entry->size = entry->is_directory ? 0 : get_file_size(filename);

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

    char* full_path = join_paths(dirname, entry->d_name);
    if (full_path == NULL)
    {
      closedir(dir);
      return RESULT_MEMORY_ERROR;
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

uint64_t file_list_get_size(const FileList* self, Size index)
{
  if (!self || index >= self->count)
  {
    return 0;
  }

  return self->entries[index].size;
}

// DirectoryTree implementation
DirectoryTree* directory_tree_create(void)
{
  DirectoryTree* tree = malloc(sizeof(DirectoryTree));
  if (tree == NULL)
  {
    printf("Произошла ошибка при выделении памяти!");
    return NULL;
  }

  tree->file_list = file_list_create();
  tree->root_path = NULL;

  if (!tree->file_list)
  {
    free(tree);
    return NULL;
  }

  return tree;
}

void directory_tree_destroy(DirectoryTree* self)
{
  if (self == NULL)
  {
    return;
  }

  file_list_destroy(self->file_list);
  free(self->root_path);
  free(self);
}

Result directory_tree_build_from_path(DirectoryTree* self,
                                      const char* root_path)
{
  if (self == NULL || root_path == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  free(self->root_path);

  self->root_path = (char*)malloc(strlen(root_path));
  if (self->root_path == NULL)
  {
    printf("Произошла ошибка при выделении памяти!\n");
    return RESULT_MEMORY_ERROR;
  }

  strcpy(self->root_path, root_path);

  file_list_destroy(self->file_list);
  self->file_list = file_list_create();
  if (self->file_list == NULL)
  {
    return RESULT_MEMORY_ERROR;
  }

  return file_list_add_directory(self->file_list, root_path, true);
}

Result directory_tree_create_from_archive(DirectoryTree* self,
                                          const char* archive_path)
{
  if (self == NULL || archive_path == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  // TODO: Реализовать создание дерева из архива
  // Это потребует парсинга структуры архива
  printf("Создание дерева из архива еще не реализовано!\n");
  return RESULT_ERROR;
}

FileList* directory_tree_get_file_list(DirectoryTree* self)
{
  return self->file_list ? self->file_list : NULL;
}

// Дополнительные утилиты
Result file_system_create_directory(const char* path)
{
  if (path == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  const int rights = 0755;
  if (mkdir(path, rights) != 0)
  {
    printf("Произошла ошибка при создании директории '%s'!\n", path);
    return RESULT_IO_ERROR;
  }

  return RESULT_OK;
}

bool file_system_path_exists(const char* path)
{
  if (path == NULL)
  {
    return false;
  }

  struct stat stats;
  return stat(path, &stats) == 0;
}

Result file_system_copy_file(const char* source, const char* destination)
{
  if (source == NULL || destination == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  File* source_file = file_create(source);
  if (source_file == NULL)
  {
    return RESULT_MEMORY_ERROR;
  }

  Result result = file_open_for_read(source_file);
  if (result != RESULT_OK)
  {
    file_destroy(source_file);
    return result;
  }

  result = file_read_bytes(source_file);
  if (result != RESULT_OK)
  {
    file_close(source_file);
    file_destroy(source_file);
    return result;
  }

  File* destination_file = file_create(destination);
  if (!destination_file)
  {
    file_close(source_file);
    file_destroy(source_file);
    return RESULT_MEMORY_ERROR;
  }

  result = file_open_for_write(destination_file);
  if (result != RESULT_OK)
  {
    file_close(source_file);
    file_destroy(source_file);
    file_destroy(destination_file);
    return result;
  }

  result = file_write_from_file(destination_file, source_file);

  file_close(source_file);
  file_close(destination_file);
  file_destroy(source_file);
  file_destroy(destination_file);

  return result;
}

static bool is_directory(const char* path)
{
  struct stat path_stats;
  if (stat(path, &path_stats) != 0)
  {
    return false;
  }

  return S_ISDIR(path_stats.st_mode);
}

static uint64_t get_file_size(const char* path)
{
  struct stat stats;
  if (stat(path, &stats) == 0)
  {
    return stats.st_size;
  }

  return 0;
}

static char* join_paths(const char* dir, const char* file)
{
  char* full_path = (char*)malloc(PATH_BUFFER_SIZE);
  if (full_path == NULL)
  {
    printf("Произошла ошибка при выделении памяти!");
    return NULL;
  }

  snprintf(full_path, PATH_BUFFER_SIZE, "%s/%s", dir, file);
  return full_path;
}
