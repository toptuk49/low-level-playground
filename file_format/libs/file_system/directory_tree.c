#include "directory_tree.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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

  self->root_path = (char*)malloc(strlen(root_path) + 1);
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
