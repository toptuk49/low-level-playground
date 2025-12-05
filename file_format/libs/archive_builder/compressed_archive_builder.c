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
#include "path_utils.h"

#define DEFAULT_COMPRESSION_ALGORITHM COMPRESSION_ARITHMETIC

typedef struct
{
  Byte* compressed_data;
  Size compressed_size;
  union
  {
    HuffmanTree* huffman_tree;
    ArithmeticModel* arithmetic_model;
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
  bool force_algorithm;
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
  builder->force_algorithm = false;

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

  if (strcmp(algorithm, "huffman") == 0 || strcmp(algorithm, "huff") == 0)
  {
    self->selected_algorithm = COMPRESSION_HUFFMAN;
    self->force_algorithm = true;
    printf("Принудительно установлен алгоритм: HUFFMAN\n");
  }
  else if (strcmp(algorithm, "arithmetic") == 0 ||
           strcmp(algorithm, "arith") == 0)
  {
    self->selected_algorithm = COMPRESSION_ARITHMETIC;
    self->force_algorithm = true;
    printf("Принудительно установлен алгоритм: ARITHMETIC\n");
  }
  else if (strcmp(algorithm, "none") == 0)
  {
    self->selected_algorithm = COMPRESSION_NONE;
    self->force_algorithm = true;
    printf("Принудительно установлен алгоритм: NONE (без сжатия)\n");
  }
  else
  {
    printf("Неизвестный алгоритм: %s. Используется автоматический выбор.\n",
           algorithm);
    self->force_algorithm = false;
  }

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
      new_capacity = 1024;
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
  else
  {
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

static void free_compressed_file_data(CompressedFileData* data)
{
  if (data)
  {
    free(data->compressed_data);
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
  }
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

  CompressionAlgorithm selected_algorithm = COMPRESSION_NONE;
  Byte* tree_model_data = NULL;
  Size tree_model_size = 0;

  void* compression_model = NULL;

  if (self->all_data_size > 0)
  {
    printf("\n=== Анализ данных для выбора алгоритма сжатия ===\n");
    printf("Объем данных для анализа: %zu байт\n", self->all_data_size);

    double entropy = calculate_entropy(self->all_data, self->all_data_size);
    printf("Энтропия данных: %.4f бит/символ\n", entropy);

    if (self->force_algorithm)
    {
      selected_algorithm = self->selected_algorithm;
      printf("Используется принудительно выбранный алгоритм: %s\n",
             selected_algorithm == COMPRESSION_HUFFMAN      ? "HUFFMAN"
             : selected_algorithm == COMPRESSION_ARITHMETIC ? "ARITHMETIC"
                                                            : "NONE");
    }
    else
    {
      // Автоматический выбор на основе энтропии
      if (entropy < 4.0)
      {
        // Низкая энтропия - лучше Хаффман
        printf("Низкая энтропия, выбираем алгоритм Хаффмана\n");
        selected_algorithm = COMPRESSION_HUFFMAN;
      }
      else if (entropy < 7.0)
      {
        // Средняя энтропия - арифметическое кодирование
        printf("Средняя энтропия, выбираем арифметическое кодирование\n");
        selected_algorithm = COMPRESSION_ARITHMETIC;
      }
      else
      {
        // Высокая энтропия - сжатие неэффективно
        printf("Высокая энтропия, сжатие неэффективно\n");
        selected_algorithm = COMPRESSION_NONE;
      }
    }

    if (selected_algorithm == COMPRESSION_HUFFMAN)
    {
      HuffmanTree* tree = huffman_tree_create();
      if (tree)
      {
        Result result =
          huffman_tree_build(tree, self->all_data, self->all_data_size);
        if (result == RESULT_OK)
        {
          result =
            huffman_serialize_tree(tree, &tree_model_data, &tree_model_size);
          if (result == RESULT_OK)
          {
            compression_model = tree;
          }
          else
          {
            huffman_tree_destroy(tree);
            selected_algorithm = COMPRESSION_NONE;
          }
        }
        else
        {
          huffman_tree_destroy(tree);
          selected_algorithm = COMPRESSION_NONE;
        }
      }
    }
    else if (selected_algorithm == COMPRESSION_ARITHMETIC)
    {
      ArithmeticModel* model = arithmetic_model_create();
      if (model)
      {
        Result result =
          arithmetic_model_build(model, self->all_data, self->all_data_size);
        if (result == RESULT_OK)
        {
          result = arithmetic_serialize_model(model, &tree_model_data,
                                              &tree_model_size);
          if (result == RESULT_OK)
          {
            compression_model = model;
          }
          else
          {
            arithmetic_model_destroy(model);
            selected_algorithm = COMPRESSION_NONE;
          }
        }
        else
        {
          arithmetic_model_destroy(model);
          selected_algorithm = COMPRESSION_NONE;
        }
      }
    }
  }

  self->selected_algorithm = selected_algorithm;

  // Шаг 2: Создаем заголовок
  DWord flags =
    file_table_get_count(self->file_table) > 1 ? FLAG_DIRECTORY : FLAG_NONE;

  if (selected_algorithm == COMPRESSION_HUFFMAN)
  {
    flags |= FLAG_COMPRESSED | FLAG_HUFFMAN_TREE;
  }
  else if (selected_algorithm == COMPRESSION_ARITHMETIC)
  {
    flags |= FLAG_COMPRESSED | FLAG_ARITHMETIC_MODEL;
  }

  CompressedArchiveHeader header;
  compressed_archive_header_init(
    &header, file_table_get_total_size(self->file_table), selected_algorithm,
    COMPRESSION_NONE, ERROR_CORRECTION_NONE, flags);

  if (selected_algorithm == COMPRESSION_HUFFMAN)
  {
    header.huffman_tree_size = (DWord)tree_model_size;
    header.arithmetic_model_size = 0;
  }
  else if (selected_algorithm == COMPRESSION_ARITHMETIC)
  {
    header.huffman_tree_size = 0;
    header.arithmetic_model_size = (DWord)tree_model_size;
  }
  else
  {
    header.huffman_tree_size = 0;
    header.arithmetic_model_size = 0;
  }

  printf("\n=== Создание заголовка ===\n");
  printf("Алгоритм сжатия: %s\n",
         selected_algorithm == COMPRESSION_HUFFMAN      ? "HUFFMAN"
         : selected_algorithm == COMPRESSION_ARITHMETIC ? "ARITHMETIC"
                                                        : "NONE");
  printf("Флаги: 0x%08X\n", flags);
  printf("Размер модели/дерева: %u байт\n",
         selected_algorithm == COMPRESSION_HUFFMAN ? header.huffman_tree_size
         : selected_algorithm == COMPRESSION_ARITHMETIC
           ? header.arithmetic_model_size
           : 0);

  Result result = compressed_archive_header_write(&header, self->archive_file);
  if (result != RESULT_OK)
  {
    printf("Ошибка записи заголовка архива!\n");

    if (compression_model)
    {
      if (selected_algorithm == COMPRESSION_HUFFMAN)
      {
        huffman_tree_destroy((HuffmanTree*)compression_model);
      }
      else if (selected_algorithm == COMPRESSION_ARITHMETIC)
      {
        arithmetic_model_destroy((ArithmeticModel*)compression_model);
      }
    }

    free(tree_model_data);
    if (self->all_data)
    {
      free(self->all_data);
    }
    return result;
  }

  // Шаг 3: Рассчитываем смещения
  QWord data_offset = COMPRESSED_ARCHIVE_HEADER_SIZE;
  data_offset += sizeof(DWord);  // file_count
  data_offset += file_table_get_count(self->file_table) * sizeof(FileEntry);

  if (tree_model_data)
  {
    data_offset += tree_model_size;  // Место для дерева/модели
  }

  printf("\n=== Расчет смещений ===\n");
  printf("Смещение после заголовка: %llu байт\n", data_offset);

  // Массив для хранения сжатых данных каждого файла
  Byte** compressed_files_data = NULL;
  Size* compressed_files_sizes = NULL;

  if (selected_algorithm != COMPRESSION_NONE &&
      file_table_get_count(self->file_table) > 0)
  {
    compressed_files_data =
      (Byte**)malloc(sizeof(Byte*) * file_table_get_count(self->file_table));
    compressed_files_sizes =
      (Size*)malloc(sizeof(Size) * file_table_get_count(self->file_table));

    if (compressed_files_data == NULL || compressed_files_sizes == NULL)
    {
      printf("Произошла ошибка при выделении памяти для сжатых данных!\n");
      if (compressed_files_data)
      {
        free((void*)compressed_files_data);
      }
      if (compressed_files_sizes)
      {
        free(compressed_files_sizes);
      }
      if (compression_model)
      {
        if (selected_algorithm == COMPRESSION_HUFFMAN)
        {
          huffman_tree_destroy((HuffmanTree*)compression_model);
        }
        else if (selected_algorithm == COMPRESSION_ARITHMETIC)
        {
          arithmetic_model_destroy((ArithmeticModel*)compression_model);
        }
      }

      free(tree_model_data);
      if (self->all_data)
      {
        free(self->all_data);
      }

      return RESULT_MEMORY_ERROR;
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

    if (selected_algorithm != COMPRESSION_NONE)
    {
      CompressedFileData compressed_file_data;
      memset(&compressed_file_data, 0, sizeof(CompressedFileData));

      result =
        compress_file_data(entry->filename, selected_algorithm, self->all_data,
                           self->all_data_size, &compressed_file_data);

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

        free(compressed_file_data.tree_model_data);
        if (selected_algorithm == COMPRESSION_HUFFMAN)
        {
          if (compressed_file_data.huffman_tree &&
              compressed_file_data.huffman_tree != compression_model)
          {
            huffman_tree_destroy(compressed_file_data.huffman_tree);
          }
        }
        else if (selected_algorithm == COMPRESSION_ARITHMETIC)
        {
          if (compressed_file_data.arithmetic_model &&
              compressed_file_data.arithmetic_model != compression_model)
          {
            arithmetic_model_destroy(compressed_file_data.arithmetic_model);
          }
        }
      }
      else
      {
        printf("  Ошибка сжатия файла!\n");
        free_compressed_file_data(&compressed_file_data);
        compression_successful = false;
        break;
      }
    }
    else
    {
      entry->compressed_size = entry->original_size;
      entry->offset = data_offset;
      data_offset += entry->original_size;
      printf("  Используется исходный размер: %llu байт\n",
             entry->original_size);
      printf("  Смещение в архиве: %llu байт\n", entry->offset);
    }
  }

  if (!compression_successful && selected_algorithm != COMPRESSION_NONE)
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

    if (compression_model)
    {
      if (selected_algorithm == COMPRESSION_HUFFMAN)
      {
        huffman_tree_destroy((HuffmanTree*)compression_model);
      }
      else if (selected_algorithm == COMPRESSION_ARITHMETIC)
      {
        arithmetic_model_destroy((ArithmeticModel*)compression_model);
      }
    }

    free(tree_model_data);
    tree_model_data = NULL;
    tree_model_size = 0;
    selected_algorithm = COMPRESSION_NONE;

    flags &= ~(FLAG_COMPRESSED | FLAG_HUFFMAN_TREE | FLAG_ARITHMETIC_MODEL);
    flags |=
      file_table_get_count(self->file_table) > 1 ? FLAG_DIRECTORY : FLAG_NONE;

    compressed_archive_header_init(
      &header, file_table_get_total_size(self->file_table), COMPRESSION_NONE,
      COMPRESSION_NONE, ERROR_CORRECTION_NONE, flags);

    header.huffman_tree_size = 0;
    header.arithmetic_model_size = 0;

    file_seek(self->archive_file, 0, SEEK_SET);
    result = compressed_archive_header_write(&header, self->archive_file);
    if (result != RESULT_OK)
    {
      printf("Ошибка перезаписи заголовка!\n");
      if (self->all_data)
      {
        free(self->all_data);
      }

      return result;
    }

    data_offset = COMPRESSED_ARCHIVE_HEADER_SIZE;
    data_offset += sizeof(DWord);
    data_offset += file_table_get_count(self->file_table) * sizeof(FileEntry);

    for (DWord i = 0; i < file_table_get_count(self->file_table); i++)
    {
      FileEntry* entry = (FileEntry*)file_table_get_entry(self->file_table, i);
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
    if (compression_model)
    {
      if (selected_algorithm == COMPRESSION_HUFFMAN)
      {
        huffman_tree_destroy((HuffmanTree*)compression_model);
      }
      else if (selected_algorithm == COMPRESSION_ARITHMETIC)
      {
        arithmetic_model_destroy((ArithmeticModel*)compression_model);
      }
    }

    free(tree_model_data);

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

    if (self->all_data)
    {
      free(self->all_data);
    }

    return result;
  }

  printf("Таблица файлов записана успешно\n");

  // Шаг 5: Записываем дерево Хаффмана или арифметическую модель (если есть)
  if (selected_algorithm != COMPRESSION_NONE && tree_model_data &&
      tree_model_size > 0)
  {
    printf("\n=== Запись модели/дерева сжатия ===\n");
    printf("Размер модели/дерева: %zu байт\n", tree_model_size);

    result =
      file_write_bytes(self->archive_file, tree_model_data, tree_model_size);
    if (result != RESULT_OK)
    {
      printf("Ошибка записи модели/дерева!\n");
      // Освобождаем ресурсы
      if (compression_model)
      {
        if (selected_algorithm == COMPRESSION_HUFFMAN)
        {
          huffman_tree_destroy((HuffmanTree*)compression_model);
        }
        else if (selected_algorithm == COMPRESSION_ARITHMETIC)
        {
          arithmetic_model_destroy((ArithmeticModel*)compression_model);
        }
      }

      free(tree_model_data);
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

      if (self->all_data)
      {
        free(self->all_data);
      }

      return result;
    }

    printf("Модель/дерево записано успешно\n");
  }

  // Шаг 6: Записываем данные файлов
  printf("\n=== Запись данных файлов ===\n");
  for (DWord i = 0; i < file_table_get_count(self->file_table); i++)
  {
    const FileEntry* entry = file_table_get_entry(self->file_table, i);
    printf("Файл %u/%u: %s ", i + 1, file_table_get_count(self->file_table),
           entry->filename);

    if (selected_algorithm != COMPRESSION_NONE && compressed_files_data &&
        compressed_files_data[i])
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

  printf("\n=== Очистка ресурсов ===\n");
  if (compression_model)
  {
    if (selected_algorithm == COMPRESSION_HUFFMAN)
    {
      huffman_tree_destroy((HuffmanTree*)compression_model);
    }
    else if (selected_algorithm == COMPRESSION_ARITHMETIC)
    {
      arithmetic_model_destroy((ArithmeticModel*)compression_model);
    }
  }

  free(tree_model_data);

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

  if (self->all_data)
  {
    free(self->all_data);
    self->all_data = NULL;
    self->all_data_size = 0;
    self->all_data_capacity = 0;
  }

  if (result != RESULT_OK)
  {
    printf("\nОшибка при создании архива!\n");
    return result;
  }

  printf("\nАрхив успешно создан!\n");
  printf("Режим: %s\n",
         selected_algorithm == COMPRESSION_HUFFMAN      ? "СЖАТЫЙ (Huffman)"
         : selected_algorithm == COMPRESSION_ARITHMETIC ? "СЖАТЫЙ (Arithmetic)"
                                                        : "НЕСЖАТЫЙ");
  printf("Файлов в архиве: %u\n", file_table_get_count(self->file_table));

  file_seek(self->archive_file, 0, SEEK_END);
  long archive_size = file_tell(self->archive_file);
  printf("Размер архива: %ld байт\n", archive_size);

  if (selected_algorithm != COMPRESSION_NONE)
  {
    printf("\n=== Анализ эффективности сжатия ===\n");

    printf("\n=== Общий анализ архива ===\n");
    printf("Общий исходный размер: %llu байт\n",
           file_table_get_total_size(self->file_table));
    printf("Общий сжатый размер: %ld байт\n", archive_size);
    printf("Общий коэффициент сжатия: %.2f%%\n",
           (1.0 - (double)archive_size /
                    (double)file_table_get_total_size(self->file_table)) *
             100);
  }

  return RESULT_OK;
}
