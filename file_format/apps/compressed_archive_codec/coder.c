#include "coder.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compressed_archive_format.h"
#include "file.h"
#include "file_system.h"
#include "types.h"

static Result write_file_to_archive(File* archive_file, const char* filepath,
                                    const char* relative_path)
{
  File* input_file = file_create(filepath);
  if (input_file == NULL)
  {
    printf("Произошла ошибка при создании объекта файла '%s'!\n", filepath);
    return RESULT_MEMORY_ERROR;
  }

  Result result = file_open_for_read(input_file);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при открытии файла '%s'!\n", filepath);
    file_destroy(input_file);
    return result;
  }

  result = file_read_bytes(input_file);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при чтении файла '%s'!\n", filepath);
    file_close(input_file);
    file_destroy(input_file);
    return result;
  }

  // TODO: Записать информацию о файле (FileEntry). Пока просто записываем
  // данные
  result = file_write_from_file(archive_file, input_file);

  file_close(input_file);
  file_destroy(input_file);

  return result;
}

Result compressed_archive_encode(const char* input_path,
                                 const char* output_filename)
{
  if (input_path == NULL || output_filename == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  printf("Начало кодирования: %s -> %s\n", input_path, output_filename);

  DirectoryTree* tree = directory_tree_create();
  if (!tree)
  {
    printf("Произошла ошибка при создании объекта дерева директорий!\n");
    return RESULT_MEMORY_ERROR;
  }

  Result result = directory_tree_build_from_path(tree, input_path);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при построении дерева директорий!\n");
    directory_tree_destroy(tree);
    return result;
  }

  FileList* file_list = directory_tree_get_file_list(tree);
  Size file_count = file_list_get_count(file_list);

  printf("Найдено файлов и папок: %zu\n", file_count);

  uint64_t total_size = 0;
  for (Size i = 0; i < file_count; i++)
  {
    if (!file_list_is_directory(file_list, i))
    {
      total_size += file_list_get_size(file_list, i);
    }
  }

  File* archive_file = file_create(output_filename);
  if (archive_file == NULL)
  {
    printf("Произошла ошибка при создании объекта архива!\n");
    directory_tree_destroy(tree);
    return RESULT_MEMORY_ERROR;
  }

  result = file_open_for_write(archive_file);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при открытии архива для записи!\n");
    directory_tree_destroy(tree);
    file_destroy(archive_file);
    return result;
  }

  CompressedArchiveHeader header;
  uint32_t flags = file_count > 1 ? FLAG_DIRECTORY : FLAG_NONE;
  compressed_archive_header_init(&header, total_size, COMPRESSION_NONE,
                                 COMPRESSION_NONE, ERROR_CORRECTION_NONE,
                                 flags);

  result = compressed_archive_header_write(&header, archive_file);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при записи заголовка архива!\n");
    directory_tree_destroy(tree);
    file_close(archive_file);
    file_destroy(archive_file);
    return result;
  }

  // TODO: Записать таблицу файлов (FileEntry для каждого файла)

  Size files_written = 0;
  for (Size i = 0; i < file_count; i++)
  {
    if (!file_list_is_directory(file_list, i))
    {
      const char* filepath = file_list_get_path(file_list, i);
      result = write_file_to_archive(archive_file, filepath, filepath);
      if (result == RESULT_OK)
      {
        files_written++;
      }
    }
  }

  printf("Записано файлов: %zu/%zu\n", files_written, file_count);
  printf("Кодирование завершено успешно!\n");
  printf("Создан архив: %s\n", output_filename);
  printf("Общий размер данных: %" PRIu64 " байт\n", total_size);
  printf("Тип архива: %s\n", (flags & FLAG_DIRECTORY) ? "папка" : "один файл");
  printf("Алгоритмы сжатия: без сжатия\n");
  printf("Версия формата: %u.%u\n", COMPRESSED_ARCHIVE_VERSION_MAJOR,
         COMPRESSED_ARCHIVE_VERSION_MINOR);

  directory_tree_destroy(tree);
  file_close(archive_file);
  file_destroy(archive_file);

  return files_written > 0 ? RESULT_OK : RESULT_ERROR;
}
