#include "path_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define PATH_LENGTH_LIMIT 4096

bool path_utils_is_directory(const char* path)
{
  struct stat path_stat;
  if (stat(path, &path_stat) != 0)
  {
    return false;
  }
  return S_ISDIR(path_stat.st_mode);
}

bool path_utils_exists(const char* path)
{
  struct stat stats;
  return stat(path, &stats) == 0;
}

Result path_utils_create_directory(const char* path)
{
  const int chown = 0755;
  if (mkdir(path, chown) != 0)
  {
    printf("Произошла ошибка при создании папки!");
    return RESULT_IO_ERROR;
  }

  return RESULT_OK;
}

char* path_utils_join(const char* dir, const char* file)
{
  char* full_path = (char*)malloc(sizeof(char) * PATH_LENGTH_LIMIT);
  if (full_path == NULL)
  {
    printf("Произошла ошибка при выделении памяти!");
    return NULL;
  }

  size_t bytes_written =
    snprintf(full_path, PATH_LENGTH_LIMIT, "%s/%s", dir, file);

  if (bytes_written != strlen(dir) + strlen(file) + 1)
  {
    printf("Произошла ошибка при склеивании пути!");
    return NULL;
  }

  return full_path;
}

char* path_utils_get_parent(const char* path)
{
  char* last_slash = strrchr(path, '/');
  if (last_slash == NULL)
  {
    printf("Родительская директория не обнаружена!\n");
    return NULL;
  }

  size_t parent_len = last_slash - path;
  char* parent = (char*)malloc(parent_len + 1);
  if (parent == NULL)
  {
    printf("Произошла ошибка при выделении памяти!\n");
    return NULL;
  }

  strncpy(parent, path, parent_len);
  parent[parent_len] = '\0';
  return parent;
}

const char* path_utils_get_filename(const char* path)
{
  const char* last_slash = strrchr(path, '/');
  return last_slash ? last_slash + 1 : path;
}

uint64_t path_utils_get_file_size(const char* path)
{
  struct stat stats;
  if (stat(path, &stats) == 0)
  {
    return stats.st_size;
  }

  return 0;
}

const char* path_utils_get_relative_path(const char* full_path,
                                         const char* base_path)
{
  const char* relative = full_path;
  if (strncmp(full_path, base_path, strlen(base_path)) == 0)
  {
    relative = full_path + strlen(base_path);

    if (*relative == '/')
    {
      relative++;
    }
  }
  return relative;
}

bool path_utils_is_file_excluded(const char* filename)
{
  // Пропускаем системные файлы и временные файлы
  const char* excluded_patterns[] = {".DS_Store", "Thumbs.db", "~", ".tmp"};
  size_t num_patterns =
    sizeof(excluded_patterns) / sizeof(excluded_patterns[0]);

  for (size_t i = 0; i < num_patterns; i++)
  {
    if (strstr(filename, excluded_patterns[i]) != NULL)
    {
      return false;
    }
  }

  return true;
}
