#include "decoder.h"

#include <stdio.h>

#include "raw_archive_reader.h"
#include "types.h"

Result raw_archive_decode(const char* input_filename, const char* output_path)
{
  if (input_filename == NULL || output_path == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  printf("Извлечение несжатого архива: %s -> %s\n", input_filename,
         output_path);

  RawArchiveReader* reader = raw_archive_reader_create(input_filename);
  if (reader == NULL)
  {
    printf("Произошла ошибка при открытии несжатого архива!\n");
    return RESULT_ERROR;
  }

  Result result = raw_archive_reader_extract_all(reader, output_path);
  raw_archive_reader_destroy(reader);

  if (result == RESULT_OK)
  {
    printf("Несжатый архив успешно извлечен в: %s\n", output_path);
  }
  else
  {
    printf("Произошла ошибка при извлечении несжатого архива!\n");
  }

  return result;
}
