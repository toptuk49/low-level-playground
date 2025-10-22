#include "decoder.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "file.h"
#include "raw_archive_header.h"
#include "types.h"

Result raw_archive_decode(const char* input_filename,
                          const char* output_filename)
{
  if (!input_filename || !output_filename)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  File* archive_file = file_create(input_filename);
  if (!archive_file)
  {
    printf("Произошла ошибка при создании объекта файла для '%s'!\n",
           input_filename);
    return RESULT_MEMORY_ERROR;
  }

  Result result = file_open_for_read(archive_file);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при открытии входного файла '%s' для чтения!\n",
           input_filename);
    file_destroy(archive_file);
    return result;
  }

  result = file_read_bytes(archive_file);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при чтении данных из файла '%s'!\n",
           input_filename);
    file_close(archive_file);
    file_destroy(archive_file);
    return result;
  }

  Size archive_size = file_get_size(archive_file);
  if (archive_size < RAW_ARCHIVE_HEADER_SIZE)
  {
    printf("Архив слишком мал для содержания заголовка!\n");
    file_close(archive_file);
    file_destroy(archive_file);
    return RESULT_ERROR;
  }

  const Byte* archive_data = file_get_buffer(archive_file);
  RawArchiveHeader header;
  memcpy(&header, archive_data, RAW_ARCHIVE_HEADER_SIZE);

  if (!raw_archive_header_is_valid(&header))
  {
    printf("Неверная сигнатура или версия архива!\n");
    file_close(archive_file);
    file_destroy(archive_file);
    return RESULT_ERROR;
  }

  if (archive_size != RAW_ARCHIVE_HEADER_SIZE + header.original_size)
  {
    printf("Размер архива и данных в нем несоответствуют!\n");
    file_close(archive_file);
    file_destroy(archive_file);
    return RESULT_ERROR;
  }

  printf("Сигнатура архива: %.6s\n", header.signature);
  printf("Версия формата: %u\n", header.version);
  printf("Оригинальный размер: %" PRIu64 " байт\n", header.original_size);

  File* output_file = file_create(output_filename);
  if (!output_file)
  {
    printf("Произошла ошибка при создании объекта файла для '%s'!\n",
           input_filename);
    file_close(archive_file);
    file_destroy(archive_file);
    return RESULT_MEMORY_ERROR;
  }

  result = file_open_for_write(output_file);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при открытии входного файла '%s' для записи!\n",
           output_filename);
    file_close(archive_file);
    file_destroy(archive_file);
    file_destroy(output_file);
    return result;
  }

  const Byte* file_data = archive_data + RAW_ARCHIVE_HEADER_SIZE;
  result = file_write_bytes(output_file, file_data, header.original_size);
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
  printf("Восстановлен файл: %s\n", output_filename);
  printf("Размер: %" PRIu64 " байт\n", header.original_size);

  file_close(archive_file);
  file_close(output_file);
  file_destroy(archive_file);
  file_destroy(output_file);

  return RESULT_OK;
}
