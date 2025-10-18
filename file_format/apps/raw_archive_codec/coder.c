#include "coder.h"

#include <stdio.h>
#include <stdlib.h>

#include "archive_format.h"
#include "file.h"
#include "types.h"

Result raw_archive_encode(const char* input_filename,
                          const char* output_filename)
{
  if (!input_filename || !output_filename)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  File* input_file = file_create(input_filename);
  if (!input_file)
  {
    printf("Произошла ошибка при создании объекта файла для '%s'!\n",
           input_filename);
    return RESULT_MEMORY_ERROR;
  }

  Result result = file_open_for_read(input_file);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при открытии входного файла '%s' для чтения!\n",
           input_filename);
    file_destroy(input_file);
    return result;
  }

  result = file_read_bytes(input_file);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при чтении данных из файла '%s'!\n",
           input_filename);
    file_close(input_file);
    file_destroy(input_file);
    return result;
  }

  Size file_size = file_get_size(input_file);
  if (file_size == 0)
  {
    printf("Входной файл пустой!\n");
    file_close(input_file);
    file_destroy(input_file);
    return RESULT_ERROR;
  }

  File* output_file = file_create(output_filename);
  if (!output_file)
  {
    printf("Произошла ошибка при создании объекта файла для '%s'!\n",
           output_filename);
    file_close(input_file);
    file_destroy(input_file);
    return RESULT_MEMORY_ERROR;
  }

  result = file_open_for_write(output_file);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при открытии входного файла '%s' для записи!\n",
           output_filename);
    file_close(input_file);
    file_destroy(input_file);
    file_destroy(output_file);
    return result;
  }

  ArchiveHeader header;
  archive_header_init(&header, file_size);

  result = archive_header_write(&header, output_file);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при записи заголовка архива!\n");
    file_close(input_file);
    file_close(output_file);
    file_destroy(input_file);
    file_destroy(output_file);
    return result;
  }

  result = file_write_from_file(output_file, input_file);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при записи данных в архив!\n");
    file_close(input_file);
    file_close(output_file);
    file_destroy(input_file);
    file_destroy(output_file);
    return result;
  }

  printf("Кодирование завершено успешно!\n");
  printf("Создан архив: %s\n", output_filename);
  printf("Исходный размер: %zu Байт\n", file_size);
  printf("Размер архива: %zu Байт\n", file_size + ARCHIVE_HEADER_SIZE);
  printf("Заголовок: %zu Байт\n", ARCHIVE_HEADER_SIZE);
  printf("Сигнатура: %.6s\n", ARCHIVE_SIGNATURE);
  printf("Версия формата: %u\n", ARCHIVE_VERSION);

  file_close(input_file);
  file_close(output_file);
  file_destroy(input_file);
  file_destroy(output_file);

  return RESULT_OK;
}
