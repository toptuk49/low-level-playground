#include "directory_tree.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "file.h"
#include "types.h"

struct DirectoryTree
{
  FileList* file_list;
  char* root_path;
};

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
bool is_directory(const char* path)
{
  struct stat path_stats;
  if (stat(path, &path_stats) != 0)
  {
    return false;
  }

  return S_ISDIR(path_stats.st_mode);
}

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
