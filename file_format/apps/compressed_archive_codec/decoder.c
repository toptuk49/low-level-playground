#include "decoder.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compressed_archive_format.h"
#include "file.h"
#include "file_system.h"
#include "types.h"

#define PATH_LIMIT 1024

Result compressed_archive_decode(const char* input_filename,
                                 const char* output_path)
{
  if (!input_filename || !output_path)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  printf("Начало декодирования: %s -> %s\n", input_filename, output_path);

  File* archive_file = file_create(input_filename);
  if (!archive_file)
  {
    printf("Произошла ошибка при создании объекта архива!\n");
    return RESULT_MEMORY_ERROR;
  }

  Result result = file_open_for_read(archive_file);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при открытии архива для чтения\n");
    file_destroy(archive_file);
    return result;
  }

  result = file_read_bytes(archive_file);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при чтении данных архива!\n");
    file_close(archive_file);
    file_destroy(archive_file);
    return result;
  }

  Size archive_size = file_get_size(archive_file);
  if (archive_size < COMPRESSED_ARCHIVE_HEADER_SIZE)
  {
    printf("Архив слишком мал!\n");
    file_close(archive_file);
    file_destroy(archive_file);
    return RESULT_ERROR;
  }

  const Byte* archive_data = file_get_buffer(archive_file);
  CompressedArchiveHeader header;
  memcpy(&header, archive_data, COMPRESSED_ARCHIVE_HEADER_SIZE);

  if (!compressed_archive_header_is_valid(&header))
  {
    printf("Неверный формат архива!\n");
    file_close(archive_file);
    file_destroy(archive_file);
    return RESULT_ERROR;
  }

  printf("Сигнатура архива: %.6s\n", header.signature);
  printf("Версия формата: %u.%u\n", header.version_major, header.version_minor);
  printf("Исходный размер: %" PRIu64 " байт\n", header.original_size);
  printf("Тип содержимого: %s\n",
         (header.flags & FLAG_DIRECTORY) ? "папка" : "один файл");
  printf("Алгоритм сжатия: %s\n", header.primary_compression == COMPRESSION_NONE
                                    ? "без сжатия"
                                    : "сжатие");

  // TODO: Реализовать извлечение структуры папок. Пока извлекаем как один файл

  if (header.flags & FLAG_DIRECTORY)
  {
    if (!file_system_path_exists(output_path))
    {
      result = file_system_create_directory(output_path);
      if (result != RESULT_OK)
      {
        printf("Произошла ошибка при создании выходной директории!\n");
        file_close(archive_file);
        file_destroy(archive_file);
        return result;
      }
    }
    printf("Создана выходная директория: %s\n", output_path);
  }

  const Byte* file_data = archive_data + COMPRESSED_ARCHIVE_HEADER_SIZE;
  Size data_size = archive_size - COMPRESSED_ARCHIVE_HEADER_SIZE;

  char output_file_path[PATH_LIMIT];
  if (header.flags & FLAG_DIRECTORY)
  {
    snprintf(output_file_path, sizeof(output_file_path), "%s/extracted_file",
             output_path);
  }
  else
  {
    strncpy(output_file_path, output_path, sizeof(output_file_path));
  }

  File* output_file = file_create(output_file_path);
  if (!output_file)
  {
    printf("Произошла ошибка при создании выходного файла!\n");
    file_close(archive_file);
    file_destroy(archive_file);
    return RESULT_MEMORY_ERROR;
  }

  result = file_open_for_write(output_file);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при открытии выходного файла для записи!\n");
    file_close(archive_file);
    file_destroy(archive_file);
    file_destroy(output_file);
    return result;
  }

  result = file_write_bytes(output_file, file_data, data_size);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при записи данных в выходной файл!\n");
    file_close(archive_file);
    file_close(output_file);
    file_destroy(archive_file);
    file_destroy(output_file);
    return result;
  }

  printf("Декодирование завершено успешно!\n");
  printf("Восстановлено в: %s\n", output_file_path);
  printf("Размер: %zu байт\n", data_size);

  file_close(archive_file);
  file_close(output_file);
  file_destroy(archive_file);
  file_destroy(output_file);

  return RESULT_OK;
}
