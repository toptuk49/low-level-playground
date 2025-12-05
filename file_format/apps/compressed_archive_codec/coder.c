#include "coder.h"

#include <stdio.h>
#include <string.h>

#include "compressed_archive_builder.h"
#include "path_utils.h"
#include "types.h"

Result compressed_archive_encode_extended(const char* input_path,
                                          const char* output_filename,
                                          const char* algorithm)
{
  if (input_path == NULL || output_filename == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  printf("Создание сжатого архива: %s -> %s\n", input_path, output_filename);

  CompressedArchiveBuilder* builder =
    compressed_archive_builder_create(output_filename);
  if (builder == NULL)
  {
    printf("Произошла ошибка при создании построителя архива!\n");
    return RESULT_MEMORY_ERROR;
  }

  if (algorithm != NULL && strlen(algorithm) > 0)
  {
    Result alg_result =
      compressed_archive_builder_set_algorithm(builder, algorithm);
    if (alg_result != RESULT_OK)
    {
      printf("Предупреждение: не удалось установить алгоритм %s\n", algorithm);
    }
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
    printf("Произошла ошибка при добавлении файлов в архив!\n");
    compressed_archive_builder_destroy(builder);
    return result;
  }

  result = compressed_archive_builder_finalize(builder);
  compressed_archive_builder_destroy(builder);

  if (result == RESULT_OK)
  {
    printf("Сжатый архив успешно создан: %s\n", output_filename);
  }
  else
  {
    printf("Произошла ошибка при создании сжатого архива!\n");
  }

  return result;
}

Result compressed_archive_encode(const char* input_path,
                                 const char* output_filename)
{
  return compressed_archive_encode_extended(input_path, output_filename, NULL);
}
