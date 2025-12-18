#include "compressed_archive_builder.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "arithmetic.h"
#include "compressed_archive_header.h"
#include "entropy.h"
#include "file_table.h"
#include "huffman.h"
#include "lz77.h"
#include "lz78.h"
#include "path_utils.h"
#include "rle.h"
#include "shannon.h"

#define DEFAULT_COMPRESSION_ALGORITHM COMPRESSION_ARITHMETIC

typedef struct
{
  Byte* compressed_data;
  Size compressed_size;
  union
  {
    HuffmanTree* huffman_tree;
    ArithmeticModel* arithmetic_model;
    ShannonTree* shannon_tree;
    RLEContext* rle_context;
    LZ78Context* lz78_context;
  };
  Byte* tree_model_data;
  Size tree_model_size;
  CompressionAlgorithm algorithm;
} CompressedFileData;

struct CompressedArchiveBuilder
{
  File* archive_file;
  FileTable* file_table;
  Byte* all_data;
  Size all_data_size;
  Size all_data_capacity;
  CompressionAlgorithm selected_algorithm;
  CompressionAlgorithm selected_secondary_algorithm;
  bool force_algorithm;
  bool use_two_stage_compression;
};

CompressedArchiveBuilder* compressed_archive_builder_create(
  const char* output_filename)
{
  if (output_filename == NULL)
  {
    return NULL;
  }

  CompressedArchiveBuilder* builder =
    (CompressedArchiveBuilder*)malloc(sizeof(CompressedArchiveBuilder));
  if (builder == NULL)
  {
    printf("Произошла ошибка при выделении памяти!\n");
    return NULL;
  }

  builder->archive_file = file_create(output_filename);
  if (builder->archive_file == NULL)
  {
    free(builder);
    return NULL;
  }

  builder->file_table = file_table_create();
  if (builder->file_table == NULL)
  {
    file_destroy(builder->archive_file);
    free(builder);
    return NULL;
  }

  builder->all_data = NULL;
  builder->all_data_size = 0;
  builder->all_data_capacity = 0;
  builder->selected_algorithm = COMPRESSION_NONE;
  builder->selected_secondary_algorithm = COMPRESSION_NONE;
  builder->force_algorithm = false;
  builder->use_two_stage_compression = false;

  Result result = file_open_for_write(builder->archive_file);
  if (result != RESULT_OK)
  {
    file_table_destroy(builder->file_table);
    file_destroy(builder->archive_file);
    free(builder);
    return NULL;
  }

  return builder;
}

Result compressed_archive_builder_set_algorithm(CompressedArchiveBuilder* self,
                                                const char* algorithm)
{
  if (self == NULL || algorithm == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  if (strcmp(algorithm, "huffman") == 0 || strcmp(algorithm, "huff") == 0 ||
      strcmp(algorithm, "h") == 0)
  {
    self->selected_algorithm = COMPRESSION_HUFFMAN;
    self->force_algorithm = true;
    printf("Принудительно установлен основной алгоритм: HUFFMAN\n");
  }
  else if (strcmp(algorithm, "arithmetic") == 0 ||
           strcmp(algorithm, "arith") == 0)
  {
    self->selected_algorithm = COMPRESSION_ARITHMETIC;
    self->force_algorithm = true;
    printf("Принудительно установлен основной алгоритм: ARITHMETIC\n");
  }
  else if (strcmp(algorithm, "shannon") == 0 ||
           strcmp(algorithm, "shan") == 0 || strcmp(algorithm, "s") == 0)
  {
    self->selected_algorithm = COMPRESSION_SHANNON;
    self->force_algorithm = true;
    printf("Принудительно установлен основной алгоритм: SHANNON\n");
  }
  else if (strcmp(algorithm, "rle") == 0 || strcmp(algorithm, "r") == 0)
  {
    self->selected_algorithm = COMPRESSION_RLE;
    self->force_algorithm = true;
    printf("Принудительно установлен основной алгоритм: RLE\n");
  }
  else if (strcmp(algorithm, "lz78") == 0)
  {
    self->selected_algorithm = COMPRESSION_LZ78;
    self->force_algorithm = true;
    printf("Принудительно установлен основной алгоритм: LZ78\n");
  }
  else if (strcmp(algorithm, "lz77") == 0)
  {
    self->selected_algorithm = COMPRESSION_LZ77;
    self->force_algorithm = true;
    printf("Принудительно установлен основной алгоритм: LZ77\n");
  }
  else if (strcmp(algorithm, "none") == 0 || strcmp(algorithm, "n") == 0)
  {
    self->selected_algorithm = COMPRESSION_NONE;
    self->force_algorithm = true;
    printf("Принудительно установлен основной алгоритм: NONE (без сжатия)\n");
  }
  else if (strcmp(algorithm, "auto") == 0 || strcmp(algorithm, "a") == 0)
  {
    self->selected_algorithm = COMPRESSION_NONE;
    self->force_algorithm = false;
    printf("Установлен автоматический выбор основного алгоритма\n");
  }
  else
  {
    printf(
      "Неизвестный основной алгоритм: %s. Используется автоматический выбор.\n",
      algorithm);
    self->force_algorithm = false;
  }

  return RESULT_OK;
}

Result compressed_archive_builder_set_secondary_algorithm(
  CompressedArchiveBuilder* self, const char* algorithm)
{
  if (self == NULL || algorithm == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  if (strcmp(algorithm, "huffman") == 0 || strcmp(algorithm, "huff") == 0 ||
      strcmp(algorithm, "h") == 0)
  {
    self->selected_secondary_algorithm = COMPRESSION_HUFFMAN;
    printf("Установлен вторичный алгоритм: HUFFMAN\n");
  }
  else if (strcmp(algorithm, "arithmetic") == 0 ||
           strcmp(algorithm, "arith") == 0)
  {
    self->selected_secondary_algorithm = COMPRESSION_ARITHMETIC;
    printf("Установлен вторичный алгоритм: ARITHMETIC\n");
  }
  else if (strcmp(algorithm, "shannon") == 0 ||
           strcmp(algorithm, "shan") == 0 || strcmp(algorithm, "s") == 0)
  {
    self->selected_secondary_algorithm = COMPRESSION_SHANNON;
    printf("Установлен вторичный алгоритм: SHANNON\n");
  }
  else if (strcmp(algorithm, "rle") == 0 || strcmp(algorithm, "r") == 0)
  {
    self->selected_secondary_algorithm = COMPRESSION_RLE;
    printf("Установлен вторичный алгоритм: RLE\n");
  }
  else if (strcmp(algorithm, "lz78") == 0)
  {
    self->selected_secondary_algorithm = COMPRESSION_LZ78;
    printf("Установлен вторичный алгоритм: LZ78\n");
  }
  else if (strcmp(algorithm, "lz77") == 0)
  {
    self->selected_secondary_algorithm = COMPRESSION_LZ77;
    printf("Установлен вторичный алгоритм: LZ77\n");
  }
  else if (strcmp(algorithm, "none") == 0 || strcmp(algorithm, "n") == 0)
  {
    self->selected_secondary_algorithm = COMPRESSION_NONE;
    printf("Установлен вторичный алгоритм: NONE\n");
  }
  else if (strcmp(algorithm, "auto") == 0 || strcmp(algorithm, "a") == 0)
  {
    self->selected_secondary_algorithm = COMPRESSION_NONE;
    printf("Установлен автоматический выбор вторичного алгоритма\n");
  }
  else
  {
    printf(
      "Неизвестный вторичный алгоритм: %s. Используется автоматический "
      "выбор.\n",
      algorithm);
    self->selected_secondary_algorithm = COMPRESSION_NONE;
  }

  return RESULT_OK;
}

Result compressed_archive_builder_set_two_staged(CompressedArchiveBuilder* self,
                                                 bool enabled)
{
  if (self == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  self->use_two_stage_compression = enabled;
  printf("Двухэтапное сжатие: %s\n", enabled ? "ВКЛЮЧЕНО" : "ВЫКЛЮЧЕНО");

  return RESULT_OK;
}

void compressed_archive_builder_destroy(CompressedArchiveBuilder* self)
{
  if (self == NULL)
  {
    return;
  }

  file_table_destroy(self->file_table);
  file_close(self->archive_file);
  file_destroy(self->archive_file);
  free(self);
}

static Result append_to_buffer(CompressedArchiveBuilder* self, const Byte* data,
                               Size size)
{
  printf("Добавление %zu байт в буфер для построения модели\n", size);
  printf("Текущий размер буфера: %zu, capacity: %zu\n", self->all_data_size,
         self->all_data_capacity);

  if (self->all_data_size + size > self->all_data_capacity)
  {
    Size new_capacity = self->all_data_capacity * 2;

    if (new_capacity < 1024)
    {
      new_capacity = 1024;
    }

    if (new_capacity < self->all_data_size + size)
    {
      new_capacity = self->all_data_size + size;
    }

    printf("Увеличиваем capacity до %zu\n", new_capacity);

    Byte* new_data = realloc(self->all_data, new_capacity);
    if (!new_data)
    {
      printf("Ошибка перевыделения памяти для буфера данных!\n");
      return RESULT_MEMORY_ERROR;
    }

    self->all_data = new_data;
    self->all_data_capacity = new_capacity;
  }

  memcpy(self->all_data + self->all_data_size, data, size);
  self->all_data_size += size;

  printf("Новый размер буфера: %zu байт\n", self->all_data_size);
  return RESULT_OK;
}

Result compressed_archive_builder_add_file(CompressedArchiveBuilder* self,
                                           const char* filename)
{
  if (self == NULL || filename == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  struct stat stats;
  if (stat(filename, &stats) != 0)
  {
    return RESULT_IO_ERROR;
  }

  File* file = file_create(filename);

  if (file == NULL)
  {
    return RESULT_MEMORY_ERROR;
  }

  Result result = file_open_for_read(file);
  if (result != RESULT_OK)
  {
    file_destroy(file);
    return result;
  }

  result = file_read_bytes(file);
  if (result == RESULT_OK)
  {
    const Byte* data = file_get_buffer(file);
    Size size = file_get_size(file);

    result = append_to_buffer(self, data, size);
    if (result != RESULT_OK)
    {
      printf("Ошибка добавления данных в буфер для построения модели!\n");
      file_close(file);
      file_destroy(file);
      return result;
    }
  }

  file_close(file);
  file_destroy(file);

  if (result != RESULT_OK)
  {
    return result;
  }

  return file_table_add_file(self->file_table, filename, stats.st_size);
}

static Result process_directory(CompressedArchiveBuilder* self,
                                const char* dirname);

Result compressed_archive_builder_add_directory(CompressedArchiveBuilder* self,
                                                const char* dirname)
{
  if (self == NULL || dirname == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  printf("Добавление директории: %s (рекурсивно с сбором данных)\n", dirname);

  return process_directory(self, dirname);
}

static Result process_directory(CompressedArchiveBuilder* self,
                                const char* dirname)
{
  DIR* directory = opendir(dirname);

  if (directory == NULL)
  {
    printf("Ошибка открытия директории: %s\n", dirname);
    return RESULT_IO_ERROR;
  }

  struct dirent* entry;
  while ((entry = readdir(directory)) != NULL)
  {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
    {
      continue;
    }

    char* full_path = path_utils_join(dirname, entry->d_name);
    if (full_path == NULL)
    {
      closedir(directory);
      return RESULT_MEMORY_ERROR;
    }

    printf("  Обработка: %s\n", full_path);

    if (entry->d_type == DT_DIR)
    {
      Result result = process_directory(self, full_path);
      if (result != RESULT_OK)
      {
        free(full_path);
        closedir(directory);
        return result;
      }
    }
    else
    {
      struct stat stats;
      if (stat(full_path, &stats) != 0)
      {
        printf("    Ошибка получения информации о файле: %s\n", full_path);
        free(full_path);
        closedir(directory);
        return RESULT_IO_ERROR;
      }

      Result result =
        file_table_add_file(self->file_table, full_path, stats.st_size);
      if (result != RESULT_OK)
      {
        printf("    Ошибка добавления файла в таблицу: %s\n", full_path);
        free(full_path);
        closedir(directory);
        return result;
      }

      File* file = file_create(full_path);
      if (!file)
      {
        printf("    Ошибка создания объекта файла: %s\n", full_path);
        free(full_path);
        closedir(directory);
        return RESULT_MEMORY_ERROR;
      }

      result = file_open_for_read(file);
      if (result != RESULT_OK)
      {
        printf("    Ошибка открытия файла: %s\n", full_path);
        file_destroy(file);
        free(full_path);
        closedir(directory);
        return result;
      }

      result = file_read_bytes(file);
      if (result == RESULT_OK)
      {
        const Byte* data = file_get_buffer(file);
        Size size = file_get_size(file);

        printf("    Чтение %zu байт из файла\n", size);
        result = append_to_buffer(self, data, size);
        if (result != RESULT_OK)
        {
          printf("    Ошибка добавления данных в буфер модели\n");
          file_close(file);
          file_destroy(file);
          free(full_path);
          closedir(directory);
          return result;
        }
      }
      else
      {
        printf("    Ошибка чтения файла: %s\n", full_path);
      }

      file_close(file);
      file_destroy(file);
      printf("    Файл успешно обработан: %s\n", full_path);
    }

    free(full_path);
  }

  closedir(directory);
  return RESULT_OK;
}

static Result write_file_data(File* archive_file, const char* filename)
{
  File* input_file = file_create(filename);
  if (input_file == NULL)
  {
    return RESULT_MEMORY_ERROR;
  }

  Result result = file_open_for_read(input_file);
  if (result != RESULT_OK)
  {
    file_destroy(input_file);
    return result;
  }

  result = file_read_bytes(input_file);
  if (result != RESULT_OK)
  {
    file_close(input_file);
    file_destroy(input_file);
    return result;
  }

  result = file_write_from_file(archive_file, input_file);

  file_close(input_file);
  file_destroy(input_file);
  return result;
}

static Result compress_file_data(const char* filename,
                                 CompressionAlgorithm algorithm,
                                 const Byte* all_data, Size all_data_size,
                                 CompressedFileData* compressed_data)
{
  File* input_file = file_create(filename);
  if (input_file == NULL)
  {
    return RESULT_MEMORY_ERROR;
  }

  Result result = file_open_for_read(input_file);
  if (result != RESULT_OK)
  {
    file_destroy(input_file);
    return result;
  }

  result = file_read_bytes(input_file);
  if (result != RESULT_OK)
  {
    file_close(input_file);
    file_destroy(input_file);
    return result;
  }

  const Byte* original_data = file_get_buffer(input_file);
  Size original_size = file_get_size(input_file);

  compressed_data->algorithm = algorithm;

  if (algorithm == COMPRESSION_HUFFMAN)
  {
    HuffmanTree* tree = NULL;

    if (all_data && all_data_size > 0)
    {
      tree = huffman_tree_create();
      if (tree == NULL)
      {
        file_close(input_file);
        file_destroy(input_file);
        return RESULT_MEMORY_ERROR;
      }
      result = huffman_tree_build(tree, all_data, all_data_size);
    }
    else
    {
      tree = huffman_tree_create();
      if (tree == NULL)
      {
        file_close(input_file);
        file_destroy(input_file);
        return RESULT_MEMORY_ERROR;
      }
      result = huffman_tree_build(tree, original_data, original_size);
    }

    if (result != RESULT_OK)
    {
      if (tree)
      {
        huffman_tree_destroy(tree);
      }

      file_close(input_file);
      file_destroy(input_file);
      return result;
    }

    compressed_data->huffman_tree = tree;

    result = huffman_compress(original_data, original_size,
                              &compressed_data->compressed_data,
                              &compressed_data->compressed_size, tree);

    if (result == RESULT_OK)
    {
      result = huffman_serialize_tree(tree, &compressed_data->tree_model_data,
                                      &compressed_data->tree_model_size);
    }
  }
  else if (algorithm == COMPRESSION_ARITHMETIC)
  {
    ArithmeticModel* model = arithmetic_model_create();

    if (model == NULL)
    {
      file_close(input_file);
      file_destroy(input_file);
      return RESULT_MEMORY_ERROR;
    }

    if (all_data && all_data_size > 0)
    {
      result = arithmetic_model_build(model, all_data, all_data_size);
    }
    else
    {
      result = arithmetic_model_build(model, original_data, original_size);
    }

    if (result != RESULT_OK)
    {
      arithmetic_model_destroy(model);
      file_close(input_file);
      file_destroy(input_file);
      return result;
    }

    compressed_data->arithmetic_model = model;

    result = arithmetic_compress(original_data, original_size,
                                 &compressed_data->compressed_data,
                                 &compressed_data->compressed_size, model);

    if (result == RESULT_OK)
    {
      result =
        arithmetic_serialize_model(model, &compressed_data->tree_model_data,
                                   &compressed_data->tree_model_size);
    }
  }
  else if (algorithm == COMPRESSION_SHANNON)
  {
    ShannonTree* tree = shannon_tree_create();
    if (tree == NULL)
    {
      file_close(input_file);
      file_destroy(input_file);
      return RESULT_MEMORY_ERROR;
    }

    if (all_data && all_data_size > 0)
    {
      result = shannon_tree_build(tree, all_data, all_data_size);
    }
    else
    {
      result = shannon_tree_build(tree, original_data, original_size);
    }

    if (result != RESULT_OK)
    {
      shannon_tree_destroy(tree);
      file_close(input_file);
      file_destroy(input_file);
      return result;
    }

    compressed_data->shannon_tree = tree;

    result = shannon_compress(original_data, original_size,
                              &compressed_data->compressed_data,
                              &compressed_data->compressed_size, tree);

    if (result == RESULT_OK)
    {
      result = shannon_serialize_tree(tree, &compressed_data->tree_model_data,
                                      &compressed_data->tree_model_size);
    }
  }
  else if (algorithm == COMPRESSION_RLE)
  {
    if (all_data && all_data_size > 0)
    {
      // Если передан глобальный контекст RLE, используем его
      if (compressed_data->rle_context)
      {
        result = rle_compress(
          original_data, original_size, &compressed_data->compressed_data,
          &compressed_data->compressed_size, compressed_data->rle_context);
      }
      else
      {
        // Создаем временный контекст на основе данных этого файла
        Byte prefix = rle_analyze_prefix(original_data, original_size);
        RLEContext* temp_context = rle_create(prefix);
        if (temp_context == NULL)
        {
          file_close(input_file);
          file_destroy(input_file);
          return RESULT_MEMORY_ERROR;
        }

        result = rle_compress(original_data, original_size,
                              &compressed_data->compressed_data,
                              &compressed_data->compressed_size, temp_context);

        rle_destroy(temp_context);
      }
    }
    else
    {
      // Без глобальных данных - используем данные файла
      Byte prefix = rle_analyze_prefix(original_data, original_size);
      RLEContext* rle_context = rle_create(prefix);
      if (rle_context == NULL)
      {
        file_close(input_file);
        file_destroy(input_file);
        return RESULT_MEMORY_ERROR;
      }

      compressed_data->rle_context = rle_context;

      result = rle_compress(original_data, original_size,
                            &compressed_data->compressed_data,
                            &compressed_data->compressed_size, rle_context);
    }
  }
  else if (algorithm == COMPRESSION_LZ78)
  {
    // LZ78 не требует глобальных данных, можно сжимать каждый файл отдельно
    result = lz78_compress(original_data, original_size,
                           &compressed_data->compressed_data,
                           &compressed_data->compressed_size);

    if (result == RESULT_OK)
    {
      // LZ78 не имеет отдельной модели для сериализации
      compressed_data->tree_model_data = NULL;
      compressed_data->tree_model_size = 0;
    }
  }
  else if (algorithm == COMPRESSION_LZ77)
  {
    // LZ77 использует префикс p
    Byte prefix = 0;

    if (all_data && all_data_size > 0)
    {
      // Анализируем данные для выбора оптимального префикса
      prefix = lz77_analyze_prefix(all_data, all_data_size);
    }
    else
    {
      // Используем данные файла для анализа
      prefix = lz77_analyze_prefix(original_data, original_size);
    }

    printf("[LZ77] Используется префикс: 0x%02X\n", prefix);

    // Сжимаем данные с LZ77
    result = lz77_compress(original_data, original_size,
                           &compressed_data->compressed_data,
                           &compressed_data->compressed_size, prefix);

    if (result == RESULT_OK)
    {
      // LZ77 требует сохранения префикса
      compressed_data->tree_model_data = malloc(1);
      if (compressed_data->tree_model_data)
      {
        compressed_data->tree_model_data[0] = prefix;
        compressed_data->tree_model_size = 1;
      }
    }
  }
  else
  {
    // COMPRESSION_NONE - без сжатия
    compressed_data->compressed_data = malloc(original_size);

    if (compressed_data->compressed_data == NULL)
    {
      printf("Произошла ошибка при выделении памяти!\n");
      file_close(input_file);
      file_destroy(input_file);
      return RESULT_MEMORY_ERROR;
    }

    memcpy(compressed_data->compressed_data, original_data, original_size);
    compressed_data->compressed_size = original_size;
    compressed_data->tree_model_data = NULL;
    compressed_data->tree_model_size = 0;
    result = RESULT_OK;
  }

  file_close(input_file);
  file_destroy(input_file);
  return result;
}

static void free_compressed_file_data(CompressedFileData* data,
                                      bool keep_compressed_data)
{
  if (data)
  {
    if (!keep_compressed_data)
    {
      free(data->compressed_data);
    }
    free(data->tree_model_data);

    if (data->algorithm == COMPRESSION_HUFFMAN)
    {
      if (data->huffman_tree)
      {
        huffman_tree_destroy(data->huffman_tree);
      }
    }
    else if (data->algorithm == COMPRESSION_ARITHMETIC)
    {
      if (data->arithmetic_model)
      {
        arithmetic_model_destroy(data->arithmetic_model);
      }
    }
    else if (data->algorithm == COMPRESSION_SHANNON)
    {
      if (data->shannon_tree)
      {
        shannon_tree_destroy(data->shannon_tree);
      }
    }
    else if (data->algorithm == COMPRESSION_RLE)
    {
      if (data->rle_context)
      {
        rle_destroy(data->rle_context);
      }
    }
    else if (data->algorithm == COMPRESSION_LZ78)
    {
      if (data->lz78_context)
      {
        lz78_destroy(data->lz78_context);
      }
    }
    // LZ77 не требует освобождения специального контекста
  }
}

// Функция для двухэтапного сжатия
static Result apply_two_stage_compression(
  const Byte* input, Size input_size, Byte** output, Size* output_size,
  CompressionAlgorithm primary_algo, CompressionAlgorithm secondary_algo,
  const Byte* context_data, Size context_size, CompressedFileData* primary_data,
  CompressedFileData* secondary_data)
{
  printf("[TWO-STAGE] Начало двухэтапного сжатия\n");
  printf("[TWO-STAGE] Исходный размер: %zu байт\n", input_size);
  printf("[TWO-STAGE] Первичный алгоритм: %s\n",
         primary_algo == COMPRESSION_HUFFMAN      ? "HUFFMAN"
         : primary_algo == COMPRESSION_ARITHMETIC ? "ARITHMETIC"
         : primary_algo == COMPRESSION_SHANNON    ? "SHANNON"
         : primary_algo == COMPRESSION_RLE        ? "RLE"
         : primary_algo == COMPRESSION_LZ78       ? "LZ78"
         : primary_algo == COMPRESSION_LZ77       ? "LZ77"
                                                  : "NONE");
  printf("[TWO-STAGE] Вторичный алгоритм: %s\n",
         secondary_algo == COMPRESSION_HUFFMAN      ? "HUFFMAN"
         : secondary_algo == COMPRESSION_ARITHMETIC ? "ARITHMETIC"
         : secondary_algo == COMPRESSION_SHANNON    ? "SHANNON"
         : secondary_algo == COMPRESSION_RLE        ? "RLE"
         : secondary_algo == COMPRESSION_LZ78       ? "LZ78"
         : secondary_algo == COMPRESSION_LZ77       ? "LZ77"
                                                    : "NONE");

  Result result = RESULT_OK;
  Byte* stage1_output = NULL;
  Size stage1_size = 0;

  // Этап 1: Первичное сжатие (обычно с контекстом)
  if (primary_algo != COMPRESSION_NONE)
  {
    CompressedFileData stage1_data;
    memset(&stage1_data, 0, sizeof(stage1_data));
    stage1_data.algorithm = primary_algo;

    result = compress_file_data("[memory]", primary_algo, context_data,
                                context_size, &stage1_data);

    if (result == RESULT_OK && stage1_data.compressed_data)
    {
      // Копируем результаты первичного сжатия
      stage1_output = stage1_data.compressed_data;
      stage1_size = stage1_data.compressed_size;

      // Сохраняем данные модели для первичного алгоритма
      if (primary_data)
      {
        primary_data->algorithm = primary_algo;
        primary_data->tree_model_data = stage1_data.tree_model_data;
        primary_data->tree_model_size = stage1_data.tree_model_size;

        // Копируем модель/дерево в зависимости от алгоритма
        if (primary_algo == COMPRESSION_HUFFMAN)
        {
          primary_data->huffman_tree = stage1_data.huffman_tree;
        }
        else if (primary_algo == COMPRESSION_ARITHMETIC)
        {
          primary_data->arithmetic_model = stage1_data.arithmetic_model;
        }
        else if (primary_algo == COMPRESSION_SHANNON)
        {
          primary_data->shannon_tree = stage1_data.shannon_tree;
        }
        else if (primary_algo == COMPRESSION_RLE)
        {
          primary_data->rle_context = stage1_data.rle_context;
        }
      }

      printf("[TWO-STAGE] Этап 1 завершен: %zu -> %zu байт (%.2f%%)\n",
             input_size, stage1_size,
             (1.0 - (double)stage1_size / (double)input_size) * 100);
    }
    else
    {
      printf(
        "[TWO-STAGE] Ошибка первичного сжатия, используем исходные данные\n");
      stage1_output = (Byte*)malloc(input_size);
      if (stage1_output)
      {
        memcpy(stage1_output, input, input_size);
        stage1_size = input_size;
      }
    }
  }
  else
  {
    // Без первичного сжатия
    stage1_output = (Byte*)malloc(input_size);
    if (stage1_output)
    {
      memcpy(stage1_output, input, input_size);
      stage1_size = input_size;
    }
    printf("[TWO-STAGE] Первичное сжатие отключено\n");
  }

  // Этап 2: Вторичное сжатие
  if (stage1_output && secondary_algo != COMPRESSION_NONE &&
      secondary_algo != primary_algo)
  {
    CompressedFileData stage2_data;
    memset(&stage2_data, 0, sizeof(stage2_data));
    stage2_data.algorithm = secondary_algo;

    // Для вторичного сжатия используем данные после первичного сжатия
    if (secondary_algo == COMPRESSION_RLE)
    {
      Byte prefix = rle_analyze_prefix(stage1_output, stage1_size);
      RLEContext* rle_context = rle_create(prefix);
      if (rle_context)
      {
        stage2_data.rle_context = rle_context;
        result =
          rle_compress(stage1_output, stage1_size, &stage2_data.compressed_data,
                       &stage2_data.compressed_size, rle_context);

        if (result == RESULT_OK && secondary_data)
        {
          // Сохраняем контекст RLE
          secondary_data->algorithm = COMPRESSION_RLE;
          secondary_data->rle_context = rle_context;

          // Сериализуем контекст
          rle_serialize_context(rle_context, &secondary_data->tree_model_data,
                                &secondary_data->tree_model_size);
        }
      }
    }
    else if (secondary_algo == COMPRESSION_LZ78)
    {
      result =
        lz78_compress(stage1_output, stage1_size, &stage2_data.compressed_data,
                      &stage2_data.compressed_size);

      if (result == RESULT_OK && secondary_data)
      {
        secondary_data->algorithm = COMPRESSION_LZ78;
      }
    }
    else if (secondary_algo == COMPRESSION_LZ77)
    {
      Byte prefix = lz77_analyze_prefix(stage1_output, stage1_size);
      result =
        lz77_compress(stage1_output, stage1_size, &stage2_data.compressed_data,
                      &stage2_data.compressed_size, prefix);

      if (result == RESULT_OK && secondary_data)
      {
        secondary_data->algorithm = COMPRESSION_LZ77;
        secondary_data->tree_model_data = malloc(1);
        if (secondary_data->tree_model_data)
        {
          secondary_data->tree_model_data[0] = prefix;
          secondary_data->tree_model_size = 1;
        }
      }
    }

    if (result == RESULT_OK && stage2_data.compressed_data)
    {
      printf("[TWO-STAGE] Этап 2 завершен: %zu -> %zu байт (%.2f%%)\n",
             stage1_size, stage2_data.compressed_size,
             (1.0 - (double)stage2_data.compressed_size / (double)stage1_size) *
               100);

      free(stage1_output);
      *output = stage2_data.compressed_data;
      *output_size = stage2_data.compressed_size;
    }
    else
    {
      // Если вторичное сжатие не удалось, используем результат первичного
      printf(
        "[TWO-STAGE] Вторичное сжатие не удалось, используем первичный "
        "результат\n");
      *output = stage1_output;
      *output_size = stage1_size;
    }
  }
  else
  {
    // Без вторичного сжатия
    *output = stage1_output;
    *output_size = stage1_size;
    printf(
      "[TWO-STAGE] Вторичное сжатие отключено или совпадает с первичным\n");
  }

  printf("[TWO-STAGE] Итоговый размер: %zu байт\n", *output_size);
  printf("[TWO-STAGE] Общий коэффициент сжатия: %.2f%%\n",
         (1.0 - (double)*output_size / (double)input_size) * 100);

  return result;
}

Result compressed_archive_builder_finalize(CompressedArchiveBuilder* self)
{
  if (self == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  printf("\n=== Начало создания сжатого архива ===\n");
  printf("Файлов в архиве: %u\n", file_table_get_count(self->file_table));
  printf("Общий размер файлов: %llu байт\n",
         file_table_get_total_size(self->file_table));

  // Определяем режим сжатия
  bool use_two_stage = self->use_two_stage_compression;
  CompressionAlgorithm primary_algo = COMPRESSION_NONE;
  CompressionAlgorithm secondary_algo = COMPRESSION_NONE;

  // Данные моделей для алгоритмов
  CompressedFileData primary_model_data;
  CompressedFileData secondary_model_data;
  memset(&primary_model_data, 0, sizeof(primary_model_data));
  memset(&secondary_model_data, 0, sizeof(secondary_model_data));

  Byte* primary_tree_model_data = NULL;
  Size primary_tree_model_size = 0;
  Byte* secondary_context_data = NULL;
  Size secondary_context_size = 0;

  void* primary_compression_model = NULL;
  void* secondary_compression_model = NULL;

  if (self->all_data_size > 0)
  {
    printf("\n=== Анализ данных для выбора алгоритма сжатия ===\n");
    printf("Объем данных для анализа: %zu байт\n", self->all_data_size);

    double entropy = calculate_entropy(self->all_data, self->all_data_size);
    printf("Энтропия данных: %.4f бит/символ\n", entropy);

    // Выбор алгоритмов
    if (use_two_stage)
    {
      printf("\n=== РЕЖИМ ДВУХЭТАПНОГО СЖАТИЯ ===\n");

      // Определяем первичный алгоритм (с контекстом)
      if (self->force_algorithm && self->selected_algorithm != COMPRESSION_NONE)
      {
        primary_algo = self->selected_algorithm;
        printf("Используется принудительно выбранный первичный алгоритм: %s\n",
               primary_algo == COMPRESSION_HUFFMAN      ? "HUFFMAN"
               : primary_algo == COMPRESSION_ARITHMETIC ? "ARITHMETIC"
               : primary_algo == COMPRESSION_SHANNON    ? "SHANNON"
               : primary_algo == COMPRESSION_RLE        ? "RLE"
               : primary_algo == COMPRESSION_LZ78       ? "LZ78"
               : primary_algo == COMPRESSION_LZ77       ? "LZ77"
                                                        : "NONE");
      }
      else
      {
        // Автоматический выбор первичного алгоритма на основе энтропии
        if (entropy < 4.0)
        {
          primary_algo = COMPRESSION_HUFFMAN;
          printf(
            "Низкая энтропия, выбираем алгоритм Хаффмана в качестве "
            "первичного\n");
        }
        else if (entropy < 6.0)
        {
          primary_algo = COMPRESSION_ARITHMETIC;
          printf(
            "Средняя энтропия, выбираем арифметическое кодирование в качестве "
            "первичного\n");
        }
        else
        {
          primary_algo = COMPRESSION_SHANNON;
          printf(
            "Высокая энтропия, выбираем алгоритм Шеннона в качестве "
            "первичного\n");
        }
      }

      // Определяем вторичный алгоритм (без контекста)
      if (self->selected_secondary_algorithm != COMPRESSION_NONE)
      {
        secondary_algo = self->selected_secondary_algorithm;
        printf("Используется принудительно выбранный вторичный алгоритм: %s\n",
               secondary_algo == COMPRESSION_HUFFMAN      ? "HUFFMAN"
               : secondary_algo == COMPRESSION_ARITHMETIC ? "ARITHMETIC"
               : secondary_algo == COMPRESSION_SHANNON    ? "SHANNON"
               : secondary_algo == COMPRESSION_RLE        ? "RLE"
               : secondary_algo == COMPRESSION_LZ78       ? "LZ78"
               : secondary_algo == COMPRESSION_LZ77       ? "LZ77"
                                                          : "NONE");
      }
      else
      {
        // Автоматический выбор вторичного алгоритма
        if (entropy < 3.0)
        {
          secondary_algo = COMPRESSION_RLE;
          printf(
            "Очень низкая энтропия, выбираем RLE в качестве вторичного "
            "алгоритма\n");
        }
        else if (entropy < 5.0)
        {
          secondary_algo = COMPRESSION_LZ77;
          printf(
            "Низкая энтропия, выбираем LZ77 в качестве вторичного алгоритма\n");
        }
        else
        {
          secondary_algo = COMPRESSION_LZ78;
          printf(
            "Средне-высокая энтропия, выбираем LZ78 в качестве вторичного "
            "алгоритма\n");
        }
      }

      // Создаем модель для первичного алгоритма
      if (primary_algo == COMPRESSION_HUFFMAN)
      {
        HuffmanTree* tree = huffman_tree_create();
        if (tree)
        {
          Result result =
            huffman_tree_build(tree, self->all_data, self->all_data_size);
          if (result == RESULT_OK)
          {
            result = huffman_serialize_tree(tree, &primary_tree_model_data,
                                            &primary_tree_model_size);
            if (result == RESULT_OK)
            {
              primary_compression_model = tree;
              primary_model_data.huffman_tree = tree;
              primary_model_data.tree_model_data = primary_tree_model_data;
              primary_model_data.tree_model_size = primary_tree_model_size;
            }
          }
        }
      }
      else if (primary_algo == COMPRESSION_ARITHMETIC)
      {
        ArithmeticModel* model = arithmetic_model_create();
        if (model)
        {
          Result result =
            arithmetic_model_build(model, self->all_data, self->all_data_size);
          if (result == RESULT_OK)
          {
            result = arithmetic_serialize_model(model, &primary_tree_model_data,
                                                &primary_tree_model_size);
            if (result == RESULT_OK)
            {
              primary_compression_model = model;
              primary_model_data.arithmetic_model = model;
              primary_model_data.tree_model_data = primary_tree_model_data;
              primary_model_data.tree_model_size = primary_tree_model_size;
            }
          }
        }
      }
      else if (primary_algo == COMPRESSION_SHANNON)
      {
        ShannonTree* tree = shannon_tree_create();
        if (tree)
        {
          Result result =
            shannon_tree_build(tree, self->all_data, self->all_data_size);
          if (result == RESULT_OK)
          {
            result = shannon_serialize_tree(tree, &primary_tree_model_data,
                                            &primary_tree_model_size);
            if (result == RESULT_OK)
            {
              primary_compression_model = tree;
              primary_model_data.shannon_tree = tree;
              primary_model_data.tree_model_data = primary_tree_model_data;
              primary_model_data.tree_model_size = primary_tree_model_size;
            }
          }
        }
      }

      // Создаем контекст для вторичного алгоритма
      if (secondary_algo == COMPRESSION_RLE)
      {
        Byte prefix = rle_analyze_prefix(self->all_data, self->all_data_size);
        RLEContext* rle_context = rle_create(prefix);
        if (rle_context)
        {
          secondary_compression_model = rle_context;
          secondary_model_data.rle_context = rle_context;

          // Сериализуем контекст
          Result result = rle_serialize_context(
            rle_context, &secondary_context_data, &secondary_context_size);
          if (result != RESULT_OK)
          {
            printf("[RLE] Ошибка сериализации контекста!\n");
            secondary_context_data = NULL;
            secondary_context_size = 0;
          }
        }
      }
      else if (secondary_algo == COMPRESSION_LZ77)
      {
        Byte prefix = lz77_analyze_prefix(self->all_data, self->all_data_size);
        secondary_context_size = 1;
        secondary_context_data = malloc(secondary_context_size);
        if (secondary_context_data)
        {
          secondary_context_data[0] = prefix;
          secondary_model_data.tree_model_data = secondary_context_data;
          secondary_model_data.tree_model_size = secondary_context_size;
        }
      }
    }
    else
    {
      // Обычное одноэтапное сжатие
      printf("\n=== РЕЖИМ ОДНОЭТАПНОГО СЖАТИЯ ===\n");

      if (self->force_algorithm)
      {
        primary_algo = self->selected_algorithm;
        printf("Используется принудительно выбранный алгоритм: %s\n",
               primary_algo == COMPRESSION_HUFFMAN      ? "HUFFMAN"
               : primary_algo == COMPRESSION_ARITHMETIC ? "ARITHMETIC"
               : primary_algo == COMPRESSION_SHANNON    ? "SHANNON"
               : primary_algo == COMPRESSION_RLE        ? "RLE"
               : primary_algo == COMPRESSION_LZ78       ? "LZ78"
               : primary_algo == COMPRESSION_LZ77       ? "LZ77"
                                                        : "NONE");
      }
      else
      {
        // Автоматический выбор на основе энтропии
        if (entropy < 3.0)
        {
          primary_algo = COMPRESSION_RLE;
          printf("Очень низкая энтропия, выбираем алгоритм RLE\n");
        }
        else if (entropy < 4.0)
        {
          primary_algo = COMPRESSION_HUFFMAN;
          printf("Низкая энтропия, выбираем алгоритм Хаффмана\n");
        }
        else if (entropy < 5.0)
        {
          primary_algo = COMPRESSION_LZ77;
          printf("Средняя энтропия, выбираем алгоритм LZ77\n");
        }
        else if (entropy < 6.0)
        {
          primary_algo = COMPRESSION_ARITHMETIC;
          printf(
            "Средне-высокая энтропия, выбираем арифметическое кодирование\n");
        }
        else if (entropy < 7.0)
        {
          primary_algo = COMPRESSION_LZ78;
          printf("Средне-высокая энтропия, выбираем алгоритм LZ78\n");
        }
        else if (entropy < 7.5)
        {
          primary_algo = COMPRESSION_SHANNON;
          printf("Средне-высокая энтропия, выбираем алгоритм Шеннона\n");
        }
        else
        {
          primary_algo = COMPRESSION_NONE;
          printf("Высокая энтропия, сжатие неэффективно\n");
        }
      }

      // Создаем модель для выбранного алгоритма
      if (primary_algo == COMPRESSION_HUFFMAN)
      {
        HuffmanTree* tree = huffman_tree_create();
        if (tree)
        {
          Result result =
            huffman_tree_build(tree, self->all_data, self->all_data_size);
          if (result == RESULT_OK)
          {
            result = huffman_serialize_tree(tree, &primary_tree_model_data,
                                            &primary_tree_model_size);
            if (result == RESULT_OK)
            {
              primary_compression_model = tree;
            }
          }
        }
      }
      else if (primary_algo == COMPRESSION_ARITHMETIC)
      {
        ArithmeticModel* model = arithmetic_model_create();
        if (model)
        {
          Result result =
            arithmetic_model_build(model, self->all_data, self->all_data_size);
          if (result == RESULT_OK)
          {
            result = arithmetic_serialize_model(model, &primary_tree_model_data,
                                                &primary_tree_model_size);
            if (result == RESULT_OK)
            {
              primary_compression_model = model;
            }
          }
        }
      }
      else if (primary_algo == COMPRESSION_SHANNON)
      {
        ShannonTree* tree = shannon_tree_create();
        if (tree)
        {
          Result result =
            shannon_tree_build(tree, self->all_data, self->all_data_size);
          if (result == RESULT_OK)
          {
            result = shannon_serialize_tree(tree, &primary_tree_model_data,
                                            &primary_tree_model_size);
            if (result == RESULT_OK)
            {
              primary_compression_model = tree;
            }
          }
        }
      }
      else if (primary_algo == COMPRESSION_RLE)
      {
        Byte prefix = rle_analyze_prefix(self->all_data, self->all_data_size);
        RLEContext* rle_context = rle_create(prefix);
        if (rle_context)
        {
          primary_compression_model = rle_context;

          // Сериализуем контекст
          Result result = rle_serialize_context(
            rle_context, &primary_tree_model_data, &primary_tree_model_size);
          if (result != RESULT_OK)
          {
            printf("[RLE] Ошибка сериализации контекста!\n");
            primary_tree_model_data = NULL;
            primary_tree_model_size = 0;
          }
        }
      }
      else if (primary_algo == COMPRESSION_LZ77)
      {
        Byte prefix = lz77_analyze_prefix(self->all_data, self->all_data_size);
        primary_tree_model_size = 1;
        primary_tree_model_data = malloc(primary_tree_model_size);
        if (primary_tree_model_data)
        {
          primary_tree_model_data[0] = prefix;
        }
      }
    }

    // Шаг 2: Создаем заголовок
    DWord flags =
      file_table_get_count(self->file_table) > 1 ? FLAG_DIRECTORY : FLAG_NONE;

    if (primary_algo != COMPRESSION_NONE || secondary_algo != COMPRESSION_NONE)
    {
      flags |= FLAG_COMPRESSED;
    }

    if (use_two_stage)
    {
      flags |= FLAG_TWO_STAGE_COMPRESSION;
    }

    // Устанавливаем флаги для конкретных алгоритмов
    if (primary_algo == COMPRESSION_HUFFMAN)
    {
      flags |= FLAG_HUFFMAN_TREE;
    }
    else if (primary_algo == COMPRESSION_ARITHMETIC)
    {
      flags |= FLAG_ARITHMETIC_MODEL;
    }
    else if (primary_algo == COMPRESSION_SHANNON)
    {
      flags |= FLAG_SHANNON_TREE;
    }
    else if (primary_algo == COMPRESSION_RLE)
    {
      flags |= FLAG_RLE_CONTEXT;
    }
    else if (primary_algo == COMPRESSION_LZ78)
    {
      flags |= FLAG_LZ78_CONTEXT;
    }
    else if (primary_algo == COMPRESSION_LZ77)
    {
      flags |= FLAG_LZ77_CONTEXT;
    }

    CompressedArchiveHeader header;
    Result result = compressed_archive_header_init(
      &header, file_table_get_total_size(self->file_table), primary_algo,
      secondary_algo, ERROR_CORRECTION_NONE, flags);

    if (result != RESULT_OK)
    {
      printf("Ошибка инициализации заголовка архива!\n");
      goto cleanup;
    }

    // Устанавливаем размеры моделей
    if (use_two_stage)
    {
      header.primary_tree_model_size = (DWord)primary_tree_model_size;
      header.secondary_context_size = (DWord)secondary_context_size;
    }
    else
    {
      // Для обратной совместимости
      if (primary_algo == COMPRESSION_HUFFMAN)
      {
        header.huffman_tree_size = (DWord)primary_tree_model_size;
      }
      else if (primary_algo == COMPRESSION_ARITHMETIC)
      {
        header.arithmetic_model_size = (DWord)primary_tree_model_size;
      }
      else if (primary_algo == COMPRESSION_SHANNON)
      {
        header.shannon_tree_size = (DWord)primary_tree_model_size;
      }
      else if (primary_algo == COMPRESSION_RLE)
      {
        header.rle_context_size = (DWord)primary_tree_model_size;
      }
      else if (primary_algo == COMPRESSION_LZ78)
      {
        header.lz78_context_size = (DWord)primary_tree_model_size;
      }
      else if (primary_algo == COMPRESSION_LZ77)
      {
        header.lz77_context_size = (DWord)primary_tree_model_size;
      }
    }

    printf("\n=== Создание заголовка ===\n");
    printf("Алгоритм сжатия: %s\n",
           primary_algo == COMPRESSION_HUFFMAN      ? "HUFFMAN"
           : primary_algo == COMPRESSION_ARITHMETIC ? "ARITHMETIC"
           : primary_algo == COMPRESSION_SHANNON    ? "SHANNON"
           : primary_algo == COMPRESSION_RLE        ? "RLE"
           : primary_algo == COMPRESSION_LZ78       ? "LZ78"
           : primary_algo == COMPRESSION_LZ77       ? "LZ77"
                                                    : "NONE");
    if (use_two_stage)
    {
      printf("Вторичный алгоритм: %s\n",
             secondary_algo == COMPRESSION_HUFFMAN      ? "HUFFMAN"
             : secondary_algo == COMPRESSION_ARITHMETIC ? "ARITHMETIC"
             : secondary_algo == COMPRESSION_SHANNON    ? "SHANNON"
             : secondary_algo == COMPRESSION_RLE        ? "RLE"
             : secondary_algo == COMPRESSION_LZ78       ? "LZ78"
             : secondary_algo == COMPRESSION_LZ77       ? "LZ77"
                                                        : "NONE");
    }
    printf("Флаги: 0x%08X\n", flags);
    printf("Размер модели/дерева: %u байт\n", header.primary_tree_model_size);
    if (use_two_stage)
    {
      printf("Размер вторичного контекста: %u байт\n",
             header.secondary_context_size);
    }

    result = compressed_archive_header_write(&header, self->archive_file);
    if (result != RESULT_OK)
    {
      printf("Ошибка записи заголовка архива!\n");
      goto cleanup;
    }

    // Шаг 3: Рассчитываем смещения
    QWord data_offset = COMPRESSED_ARCHIVE_HEADER_SIZE;
    data_offset += sizeof(DWord);  // file_count
    data_offset += file_table_get_count(self->file_table) * sizeof(FileEntry);

    // Добавляем место для моделей/деревьев
    if (primary_tree_model_data && primary_tree_model_size > 0)
    {
      data_offset += primary_tree_model_size;
    }
    if (secondary_context_data && secondary_context_size > 0)
    {
      data_offset += secondary_context_size;
    }

    printf("\n=== Расчет смещений ===\n");
    printf("Смещение после заголовка: %llu байт\n", data_offset);

    // Массив для хранения сжатых данных каждого файла
    Byte** compressed_files_data = NULL;
    Size* compressed_files_sizes = NULL;

    if ((primary_algo != COMPRESSION_NONE ||
         secondary_algo != COMPRESSION_NONE) &&
        file_table_get_count(self->file_table) > 0)
    {
      compressed_files_data =
        (Byte**)malloc(sizeof(Byte*) * file_table_get_count(self->file_table));
      compressed_files_sizes =
        (Size*)malloc(sizeof(Size) * file_table_get_count(self->file_table));

      if (compressed_files_data == NULL || compressed_files_sizes == NULL)
      {
        printf("Произошла ошибка при выделении памяти для сжатых данных!\n");
        result = RESULT_MEMORY_ERROR;
        goto cleanup;
      }

      memset((void*)compressed_files_data, 0,
             sizeof(Byte*) * file_table_get_count(self->file_table));
      memset(compressed_files_sizes, 0,
             sizeof(Size) * file_table_get_count(self->file_table));
    }

    printf("\n=== Сжатие файлов ===\n");
    bool compression_successful = true;

    for (DWord i = 0; i < file_table_get_count(self->file_table); i++)
    {
      FileEntry* entry = (FileEntry*)file_table_get_entry(self->file_table, i);
      printf("Файл %u/%u: %s\n", i + 1, file_table_get_count(self->file_table),
             entry->filename);

      if (use_two_stage)
      {
        // Двухэтапное сжатие
        Byte* compressed_data = NULL;
        Size compressed_size = 0;

        File* input_file = file_create(entry->filename);
        if (input_file == NULL)
        {
          printf("  Ошибка открытия файла для сжатия!\n");
          compression_successful = false;
          break;
        }

        Result open_result = file_open_for_read(input_file);
        if (open_result != RESULT_OK)
        {
          file_destroy(input_file);
          printf("  Ошибка открытия файла для чтения!\n");
          compression_successful = false;
          break;
        }

        Result read_result = file_read_bytes(input_file);
        if (read_result != RESULT_OK)
        {
          file_close(input_file);
          file_destroy(input_file);
          printf("  Ошибка чтения файла!\n");
          compression_successful = false;
          break;
        }

        const Byte* original_data = file_get_buffer(input_file);
        Size original_size = file_get_size(input_file);

        // Применяем двухэтапное сжатие
        result = apply_two_stage_compression(
          original_data, original_size, &compressed_data, &compressed_size,
          primary_algo, secondary_algo, self->all_data, self->all_data_size,
          &primary_model_data, &secondary_model_data);

        file_close(input_file);
        file_destroy(input_file);

        if (result == RESULT_OK && compressed_data)
        {
          printf("  Исходный размер: %llu байт\n", entry->original_size);
          printf("  Сжатый размер: %zu байт\n", compressed_size);
          printf(
            "  Коэффициент сжатия: %.1f%%\n",
            (1.0 - (double)compressed_size / (double)entry->original_size) *
              100);

          compressed_files_data[i] = compressed_data;
          compressed_files_sizes[i] = compressed_size;

          entry->compressed_size = compressed_size;
          entry->offset = data_offset;
          data_offset += compressed_size;

          printf("  Смещение в архиве: %llu байт\n", entry->offset);
        }
        else
        {
          printf("  Ошибка двухэтапного сжатия файла!\n");
          free(compressed_data);
          compression_successful = false;
          break;
        }
      }
      else
      {
        // Одноэтапное сжатие (старый код)
        CompressedFileData compressed_file_data;
        memset(&compressed_file_data, 0, sizeof(CompressedFileData));

        if (primary_algo == COMPRESSION_RLE)
        {
          // Используем общий контекст RLE
          compressed_file_data.rle_context =
            (RLEContext*)primary_compression_model;
          compressed_file_data.algorithm = COMPRESSION_RLE;

          File* input_file = file_create(entry->filename);
          if (input_file == NULL)
          {
            printf("  Ошибка открытия файла для сжатия RLE!\n");
            compression_successful = false;
            break;
          }

          Result open_result = file_open_for_read(input_file);
          if (open_result != RESULT_OK)
          {
            file_destroy(input_file);
            printf("  Ошибка открытия файла для чтения!\n");
            compression_successful = false;
            break;
          }

          Result read_result = file_read_bytes(input_file);
          if (read_result != RESULT_OK)
          {
            file_close(input_file);
            file_destroy(input_file);
            printf("  Ошибка чтения файла!\n");
            compression_successful = false;
            break;
          }

          const Byte* original_data = file_get_buffer(input_file);
          Size original_size = file_get_size(input_file);

          // Сжимаем с использованием общего контекста
          result = rle_compress(original_data, original_size,
                                &compressed_file_data.compressed_data,
                                &compressed_file_data.compressed_size,
                                (RLEContext*)primary_compression_model);

          file_close(input_file);
          file_destroy(input_file);
        }
        else if (primary_algo == COMPRESSION_LZ78)
        {
          result =
            compress_file_data(entry->filename, primary_algo, NULL,
                               0,  // LZ78 не использует глобальные данные
                               &compressed_file_data);
        }
        else if (primary_algo == COMPRESSION_LZ77)
        {
          // Используем глобальный префикс LZ77
          Byte prefix = 0;
          if (primary_tree_model_data && primary_tree_model_size >= 1)
          {
            prefix = primary_tree_model_data[0];
          }
          else
          {
            // Если нет глобального префикса, анализируем файл отдельно
            File* input_file = file_create(entry->filename);
            if (input_file == NULL)
            {
              printf("  Ошибка открытия файла для анализа LZ77!\n");
              compression_successful = false;
              break;
            }

            Result open_result = file_open_for_read(input_file);
            if (open_result != RESULT_OK)
            {
              file_destroy(input_file);
              printf("  Ошибка открытия файла для чтения!\n");
              compression_successful = false;
              break;
            }

            Result read_result = file_read_bytes(input_file);
            if (read_result != RESULT_OK)
            {
              file_close(input_file);
              file_destroy(input_file);
              printf("  Ошибка чтения файла!\n");
              compression_successful = false;
              break;
            }

            const Byte* file_data = file_get_buffer(input_file);
            Size file_size = file_get_size(input_file);

            prefix = lz77_analyze_prefix(file_data, file_size);

            file_close(input_file);
            file_destroy(input_file);
          }

          printf("  Используется префикс LZ77: 0x%02X\n", prefix);

          // Сжимаем с LZ77
          File* input_file = file_create(entry->filename);
          if (input_file == NULL)
          {
            printf("  Ошибка открытия файла для сжатия LZ77!\n");
            compression_successful = false;
            break;
          }

          Result open_result = file_open_for_read(input_file);
          if (open_result != RESULT_OK)
          {
            file_destroy(input_file);
            printf("  Ошибка открытия файла для чтения!\n");
            compression_successful = false;
            break;
          }

          Result read_result = file_read_bytes(input_file);
          if (read_result != RESULT_OK)
          {
            file_close(input_file);
            file_destroy(input_file);
            printf("  Ошибка чтения файла!\n");
            compression_successful = false;
            break;
          }

          const Byte* original_data = file_get_buffer(input_file);
          Size original_size = file_get_size(input_file);

          // Сжимаем с использованием LZ77
          result = lz77_compress(original_data, original_size,
                                 &compressed_file_data.compressed_data,
                                 &compressed_file_data.compressed_size, prefix);

          file_close(input_file);
          file_destroy(input_file);
        }
        else if (primary_algo != COMPRESSION_NONE)
        {
          result =
            compress_file_data(entry->filename, primary_algo, self->all_data,
                               self->all_data_size, &compressed_file_data);
        }
        else
        {
          // COMPRESSION_NONE - без сжатия
          compressed_file_data.compressed_data = malloc(entry->original_size);

          if (compressed_file_data.compressed_data == NULL)
          {
            printf("Произошла ошибка при выделении памяти!\n");
            compression_successful = false;
            break;
          }

          File* input_file = file_create(entry->filename);
          if (input_file == NULL)
          {
            free(compressed_file_data.compressed_data);
            compression_successful = false;
            break;
          }

          Result open_result = file_open_for_read(input_file);
          if (open_result != RESULT_OK)
          {
            file_destroy(input_file);
            free(compressed_file_data.compressed_data);
            compression_successful = false;
            break;
          }

          Result read_result = file_read_bytes(input_file);
          if (read_result != RESULT_OK)
          {
            file_close(input_file);
            file_destroy(input_file);
            free(compressed_file_data.compressed_data);
            compression_successful = false;
            break;
          }

          const Byte* original_data = file_get_buffer(input_file);
          memcpy(compressed_file_data.compressed_data, original_data,
                 entry->original_size);
          compressed_file_data.compressed_size = entry->original_size;
          result = RESULT_OK;

          file_close(input_file);
          file_destroy(input_file);
        }

        if (result == RESULT_OK && compressed_file_data.compressed_data)
        {
          printf("  Исходный размер: %llu байт\n", entry->original_size);
          printf("  Сжатый размер: %zu байт\n",
                 compressed_file_data.compressed_size);
          printf("  Коэффициент сжатия: %.1f%%\n",
                 (1.0 - (double)compressed_file_data.compressed_size /
                          (double)entry->original_size) *
                   100);

          compressed_files_data[i] = compressed_file_data.compressed_data;
          compressed_files_sizes[i] = compressed_file_data.compressed_size;

          entry->compressed_size = compressed_file_data.compressed_size;
          entry->offset = data_offset;
          data_offset += compressed_file_data.compressed_size;

          printf("  Смещение в архиве: %llu байт\n", entry->offset);

          free_compressed_file_data(&compressed_file_data, true);
        }
        else
        {
          printf("  Ошибка сжатия файла!\n");
          free_compressed_file_data(&compressed_file_data, false);
          compression_successful = false;
          break;
        }
      }
    }

    if (!compression_successful && (primary_algo != COMPRESSION_NONE ||
                                    secondary_algo != COMPRESSION_NONE))
    {
      printf("\n=== Переход на несжатый режим ===\n");

      if (compressed_files_data)
      {
        for (DWord i = 0; i < file_table_get_count(self->file_table); i++)
        {
          if (compressed_files_data[i])
          {
            free(compressed_files_data[i]);
          }
        }
        free((void*)compressed_files_data);
        free(compressed_files_sizes);
        compressed_files_data = NULL;
        compressed_files_sizes = NULL;
      }

      primary_algo = COMPRESSION_NONE;
      secondary_algo = COMPRESSION_NONE;
      use_two_stage = false;

      flags &= ~(FLAG_COMPRESSED | FLAG_HUFFMAN_TREE | FLAG_ARITHMETIC_MODEL |
                 FLAG_SHANNON_TREE | FLAG_RLE_CONTEXT | FLAG_LZ78_CONTEXT |
                 FLAG_LZ77_CONTEXT | FLAG_TWO_STAGE_COMPRESSION);
      flags |=
        file_table_get_count(self->file_table) > 1 ? FLAG_DIRECTORY : FLAG_NONE;

      compressed_archive_header_init(
        &header, file_table_get_total_size(self->file_table), COMPRESSION_NONE,
        COMPRESSION_NONE, ERROR_CORRECTION_NONE, flags);

      header.primary_tree_model_size = 0;
      header.secondary_context_size = 0;

      file_seek(self->archive_file, 0, SEEK_SET);
      result = compressed_archive_header_write(&header, self->archive_file);
      if (result != RESULT_OK)
      {
        printf("Ошибка перезаписи заголовка!\n");
        goto cleanup;
      }

      data_offset = COMPRESSED_ARCHIVE_HEADER_SIZE;
      data_offset += sizeof(DWord);
      data_offset += file_table_get_count(self->file_table) * sizeof(FileEntry);

      for (DWord i = 0; i < file_table_get_count(self->file_table); i++)
      {
        FileEntry* entry =
          (FileEntry*)file_table_get_entry(self->file_table, i);
        entry->compressed_size = entry->original_size;
        entry->offset = data_offset;
        data_offset += entry->original_size;
      }
    }

    // Шаг 4: Записываем таблицу файлов
    printf("\n=== Запись таблицы файлов ===\n");
    file_seek(self->archive_file, COMPRESSED_ARCHIVE_HEADER_SIZE, SEEK_SET);
    result = file_table_write(self->file_table, self->archive_file);
    if (result != RESULT_OK)
    {
      printf("Ошибка записи таблицы файлов!\n");
      goto cleanup_compressed_files;
    }

    printf("Таблица файлов записана успешно\n");

    // Шаг 5: Записываем модель/дерево сжатия (если есть)
    if (primary_tree_model_data && primary_tree_model_size > 0)
    {
      printf("\n=== Запись модели/дерева первичного алгоритма ===\n");
      printf("Размер модели/дерева: %zu байт\n", primary_tree_model_size);

      result = file_write_bytes(self->archive_file, primary_tree_model_data,
                                primary_tree_model_size);
      if (result != RESULT_OK)
      {
        printf("Ошибка записи модели/дерева первичного алгоритма!\n");
        goto cleanup_compressed_files;
      }

      printf("Модель/дерево первичного алгоритма записано успешно\n");
    }

    // Шаг 6: Записываем контекст вторичного алгоритма (если есть)
    if (use_two_stage && secondary_context_data && secondary_context_size > 0)
    {
      printf("\n=== Запись контекста вторичного алгоритма ===\n");
      printf("Размер контекста: %zu байт\n", secondary_context_size);

      result = file_write_bytes(self->archive_file, secondary_context_data,
                                secondary_context_size);
      if (result != RESULT_OK)
      {
        printf("Ошибка записи контекста вторичного алгоритма!\n");
        goto cleanup_compressed_files;
      }

      printf("Контекст вторичного алгоритма записан успешно\n");
    }

    // Шаг 7: Записываем данные файлов
    printf("\n=== Запись данных файлов ===\n");
    for (DWord i = 0; i < file_table_get_count(self->file_table); i++)
    {
      const FileEntry* entry = file_table_get_entry(self->file_table, i);
      printf("Файл %u/%u: %s ", i + 1, file_table_get_count(self->file_table),
             entry->filename);

      if ((primary_algo != COMPRESSION_NONE ||
           secondary_algo != COMPRESSION_NONE) &&
          compressed_files_data && compressed_files_data[i])
      {
        printf("(сжатый, %zu -> %llu байт)\n", compressed_files_sizes[i],
               entry->original_size);

        result = file_write_bytes(self->archive_file, compressed_files_data[i],
                                  compressed_files_sizes[i]);
        if (result != RESULT_OK)
        {
          printf("Ошибка записи сжатых данных файла!\n");
          break;
        }
      }
      else
      {
        printf("(несжатый, %llu байт)\n", entry->original_size);

        result = write_file_data(self->archive_file, entry->filename);
        if (result != RESULT_OK)
        {
          printf("Ошибка записи данных файла!\n");
          break;
        }
      }

      if (result != RESULT_OK)
      {
        break;
      }
    }

  cleanup_compressed_files:
    if (compressed_files_data)
    {
      for (DWord i = 0; i < file_table_get_count(self->file_table); i++)
      {
        if (compressed_files_data[i])
        {
          free(compressed_files_data[i]);
        }
      }
      free((void*)compressed_files_data);
      free(compressed_files_sizes);
    }

    if (result != RESULT_OK)
    {
      printf("\nОшибка при создании архива!\n");
      goto cleanup;
    }

    printf("\nАрхив успешно создан!\n");
    if (use_two_stage)
    {
      printf("Режим: ДВУХЭТАПНОЕ СЖАТИЕ\n");
      printf("Первичный алгоритм: %s\n",
             primary_algo == COMPRESSION_HUFFMAN      ? "HUFFMAN"
             : primary_algo == COMPRESSION_ARITHMETIC ? "ARITHMETIC"
             : primary_algo == COMPRESSION_SHANNON    ? "SHANNON"
                                                      : "NONE");
      printf("Вторичный алгоритм: %s\n",
             secondary_algo == COMPRESSION_RLE    ? "RLE"
             : secondary_algo == COMPRESSION_LZ78 ? "LZ78"
             : secondary_algo == COMPRESSION_LZ77 ? "LZ77"
                                                  : "NONE");
    }
    else
    {
      printf("Режим: %s\n",
             primary_algo == COMPRESSION_HUFFMAN      ? "СЖАТЫЙ (Huffman)"
             : primary_algo == COMPRESSION_ARITHMETIC ? "СЖАТЫЙ (Arithmetic)"
             : primary_algo == COMPRESSION_SHANNON    ? "СЖАТЫЙ (Shannon)"
             : primary_algo == COMPRESSION_RLE        ? "СЖАТЫЙ (RLE)"
             : primary_algo == COMPRESSION_LZ78       ? "СЖАТЫЙ (LZ78)"
             : primary_algo == COMPRESSION_LZ77       ? "СЖАТЫЙ (LZ77)"
                                                      : "НЕСЖАТЫЙ");
    }
    printf("Файлов в архиве: %u\n", file_table_get_count(self->file_table));

    file_seek(self->archive_file, 0, SEEK_END);
    long archive_size = file_tell(self->archive_file);
    printf("Размер архива: %ld байт\n", archive_size);

    if (primary_algo != COMPRESSION_NONE || secondary_algo != COMPRESSION_NONE)
    {
      printf("\n=== Общий анализ архива ===\n");
      printf("Общий исходный размер: %llu байт\n",
             file_table_get_total_size(self->file_table));
      printf("Общий сжатый размер: %ld байт\n", archive_size);
      printf("Общий коэффициент сжатия: %.2f%%\n",
             (1.0 - (double)archive_size /
                      (double)file_table_get_total_size(self->file_table)) *
               100);
    }

  cleanup:
    // Освобождаем ресурсы
    if (primary_compression_model)
    {
      if (primary_algo == COMPRESSION_HUFFMAN)
      {
        huffman_tree_destroy((HuffmanTree*)primary_compression_model);
      }
      else if (primary_algo == COMPRESSION_ARITHMETIC)
      {
        arithmetic_model_destroy((ArithmeticModel*)primary_compression_model);
      }
      else if (primary_algo == COMPRESSION_SHANNON)
      {
        shannon_tree_destroy((ShannonTree*)primary_compression_model);
      }
      else if (primary_algo == COMPRESSION_RLE)
      {
        rle_destroy((RLEContext*)primary_compression_model);
      }
    }

    if (secondary_compression_model &&
        secondary_compression_model != primary_compression_model)
    {
      if (secondary_algo == COMPRESSION_RLE)
      {
        rle_destroy((RLEContext*)secondary_compression_model);
      }
    }

    free(primary_tree_model_data);
    free(secondary_context_data);

    if (self->all_data)
    {
      free(self->all_data);
      self->all_data = NULL;
      self->all_data_size = 0;
      self->all_data_capacity = 0;
    }

    return result;
  }
  else
  {
    printf("Нет данных для анализа. Создание несжатого архива.\n");

    // Создаем заголовок для несжатого архива
    DWord flags =
      file_table_get_count(self->file_table) > 1 ? FLAG_DIRECTORY : FLAG_NONE;

    CompressedArchiveHeader header;
    compressed_archive_header_init(
      &header, file_table_get_total_size(self->file_table), COMPRESSION_NONE,
      COMPRESSION_NONE, ERROR_CORRECTION_NONE, flags);

    Result result =
      compressed_archive_header_write(&header, self->archive_file);
    if (result != RESULT_OK)
    {
      printf("Ошибка записи заголовка архива!\n");
      return result;
    }

    // Записываем таблицу файлов
    file_seek(self->archive_file, COMPRESSED_ARCHIVE_HEADER_SIZE, SEEK_SET);
    result = file_table_write(self->file_table, self->archive_file);
    if (result != RESULT_OK)
    {
      printf("Ошибка записи таблицы файлов!\n");
      return result;
    }

    // Записываем данные файлов
    for (DWord i = 0; i < file_table_get_count(self->file_table); i++)
    {
      const FileEntry* entry = file_table_get_entry(self->file_table, i);
      result = write_file_data(self->archive_file, entry->filename);
      if (result != RESULT_OK)
      {
        printf("Ошибка записи данных файла: %s\n", entry->filename);
        return result;
      }
    }

    printf("\nНесжатый архив успешно создан!\n");
    printf("Файлов в архиве: %u\n", file_table_get_count(self->file_table));

    file_seek(self->archive_file, 0, SEEK_END);
    long archive_size = file_tell(self->archive_file);
    printf("Размер архива: %ld байт\n", archive_size);

    return RESULT_OK;
  }
}
