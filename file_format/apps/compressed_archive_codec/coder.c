#include "coder.h"

#include <stdio.h>

#include "compressed_archive_builder.h"
#include "path_utils.h"
#include "types.h"

Result compressed_archive_encode(const char* input_path,
                                 const char* output_filename)
{
  if (input_path == NULL || output_filename == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  printf("Создание архива: %s -> %s\n", input_path, output_filename);

  CompressedArchiveBuilder* builder =
    compressed_archive_builder_create(output_filename);
  if (builder == NULL)
  {
    printf("Ошибка: не удалось создать построитель архива\n");
    return RESULT_MEMORY_ERROR;
  }

  Result result;
  if (path_utils_is_directory(input_path))
  {
    printf("Добавление директории: %s\n", input_path);
    result = compressed_archive_builder_add_directory(builder, input_path);
  }
  else
  {
    printf("Добавление файла: %s\n", input_path);
    result = compressed_archive_builder_add_file(builder, input_path);
  }

  if (result != RESULT_OK)
  {
    printf("Ошибка добавления файлов в архив\n");
    compressed_archive_builder_destroy(builder);
    return result;
  }

  result = compressed_archive_builder_finalize(builder);
  compressed_archive_builder_destroy(builder);

  if (result == RESULT_OK)
  {
    printf("Архив успешно создан: %s\n", output_filename);
  }
  else
  {
    printf("Ошибка создания архива\n");
  }

  return result;
}
