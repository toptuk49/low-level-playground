#include "decoder.h"

#include <stdio.h>

#include "compressed_archive_reader.h"
#include "types.h"

Result compressed_archive_decode(const char* input_filename,
                                 const char* output_path)
{
  if (input_filename == NULL || output_path == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  printf("Извлечение сжатого архива: %s -> %s\n", input_filename, output_path);

  CompressedArchiveReader* reader =
    compressed_archive_reader_create(input_filename);
  if (reader == NULL)
  {
    printf("Произошла ошибка при открытии сжатого архива!\n");
    return RESULT_ERROR;
  }

  Result result = compressed_archive_reader_extract_all(reader, output_path);
  compressed_archive_reader_destroy(reader);

  if (result == RESULT_OK)
  {
    printf("Сжатый архив успешно извлечен в: %s\n", output_path);
  }
  else
  {
    printf("Произошла ошибка при извлечении сжатого архива!\n");
  }

  return result;
}
