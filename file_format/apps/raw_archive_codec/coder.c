#include "coder.h"

#include <stdio.h>

#include "path_utils.h"
#include "raw_archive_builder.h"
#include "types.h"

Result raw_archive_encode(const char* input_path, const char* output_filename)
{
  if (input_path == NULL || output_filename == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  printf("Создание несжатого архива: %s -> %s\n", input_path, output_filename);

  RawArchiveBuilder* builder = raw_archive_builder_create(output_filename);
  if (builder == NULL)
  {
    printf("Произошла ошибка при создании построителя архива!\n");
    return RESULT_MEMORY_ERROR;
  }

  Result result;
  if (path_utils_is_directory(input_path))
  {
    printf("Добавление директории: %s\n", input_path);
    result = raw_archive_builder_add_directory(builder, input_path);
  }
  else
  {
    printf("Добавление файла: %s\n", input_path);
    result = raw_archive_builder_add_file(builder, input_path);
  }

  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при добавлении файлов в архив!\n");
    raw_archive_builder_destroy(builder);
    return result;
  }

  result = raw_archive_builder_finalize(builder);
  raw_archive_builder_destroy(builder);

  if (result == RESULT_OK)
  {
    printf("Несжатый архив успешно создан: %s\n", output_filename);
  }
  else
  {
    printf("Произошла ошибка при создании несжатого архива!\n");
  }

  return result;
}
