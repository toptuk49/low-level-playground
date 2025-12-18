#include "compressed_archive_reader.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arithmetic.h"
#include "compressed_archive_header.h"
#include "file_table.h"
#include "huffman.h"
#include "lz77.h"
#include "lz78.h"
#include "path_utils.h"
#include "rle.h"
#include "shannon.h"

#define PATH_LIMIT 4096

struct CompressedArchiveReader
{
  File* archive_file;
  FileTable* file_table;
  CompressedArchiveHeader header;
  HuffmanTree* huffman_tree;  // Дерево Хаффмана (если используется)
  ArithmeticModel*
    arithmetic_model;         // Арифметическая модель (если используется)
  ShannonTree* shannon_tree;  // Дерево Шеннона (если используется)
  RLEContext* rle_context;    // Контекст RLE (если используется)
  LZ78Context* lz78_context;  // Контекст LZ78 (если используется)
  Byte* lz77_context_data;    // Данные контекста LZ77 (префикс)
  Size lz77_context_size;     // Размер контекста LZ77

  // Для двухэтапного сжатия
  HuffmanTree*
    secondary_huffman_tree;  // Дерево Хаффмана для вторичного алгоритма
  ArithmeticModel*
    secondary_arithmetic_model;  // Модель для вторичного алгоритма
  ShannonTree*
    secondary_shannon_tree;           // Дерево Шеннона для вторичного алгоритма
  RLEContext* secondary_rle_context;  // Контекст RLE для вторичного алгоритма
  Byte* secondary_lz77_context_data;  // Данные контекста LZ77 для вторичного
                                      // алгоритма
  Size secondary_lz77_context_size;   // Размер контекста LZ77 для вторичного
                                      // алгоритма
};

CompressedArchiveReader* compressed_archive_reader_create(
  const char* input_filename)
{
  if (input_filename == NULL)
  {
    return NULL;
  }

  CompressedArchiveReader* reader =
    (CompressedArchiveReader*)malloc(sizeof(CompressedArchiveReader));

  if (reader == NULL)
  {
    return NULL;
  }

  reader->archive_file = file_create(input_filename);

  if (reader->archive_file == NULL)
  {
    free(reader);
    return NULL;
  }

  reader->file_table = file_table_create();

  if (reader->file_table == NULL)
  {
    file_destroy(reader->archive_file);
    free(reader);
    return NULL;
  }

  reader->huffman_tree = NULL;
  reader->arithmetic_model = NULL;
  reader->shannon_tree = NULL;
  reader->rle_context = NULL;
  reader->lz78_context = NULL;
  reader->lz77_context_data = NULL;
  reader->lz77_context_size = 0;

  reader->secondary_huffman_tree = NULL;
  reader->secondary_arithmetic_model = NULL;
  reader->secondary_shannon_tree = NULL;
  reader->secondary_rle_context = NULL;
  reader->secondary_lz77_context_data = NULL;
  reader->secondary_lz77_context_size = 0;

  printf("\n=== Открытие архива для чтения ===\n");
  printf("Файл: %s\n", input_filename);

  Result result = file_open_for_read(reader->archive_file);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при открытии файла архива!\n");
    goto error;
  }

  result = file_read_bytes(reader->archive_file);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при чтении данных архива!\n");
    goto error;
  }

  const Byte* data = file_get_buffer(reader->archive_file);
  memcpy(&reader->header, data, COMPRESSED_ARCHIVE_HEADER_SIZE);

  printf("Сигнатура: %.6s\n", reader->header.signature);
  printf("Версия: %u.%u\n", reader->header.version_major,
         reader->header.version_minor);
  printf("Флаги: 0x%08X\n", reader->header.flags);
  printf("Основной алгоритм сжатия: %u\n", reader->header.primary_compression);
  printf("Вторичный алгоритм сжатия: %u\n",
         reader->header.secondary_compression);
  printf("Размер модели первичного алгоритма: %u байт\n",
         reader->header.primary_tree_model_size);
  printf("Размер контекста вторичного алгоритма: %u байт\n",
         reader->header.secondary_context_size);

  // Для обратной совместимости
  printf("Размер дерева Хаффмана: %u байт\n", reader->header.huffman_tree_size);
  printf("Размер арифметической модели: %u байт\n",
         reader->header.arithmetic_model_size);
  printf("Размер дерева Шеннона: %u байт\n", reader->header.shannon_tree_size);
  printf("Размер контекста RLE: %u байт\n", reader->header.rle_context_size);
  printf("Размер контекста LZ78: %u байт\n", reader->header.lz78_context_size);
  printf("Размер контекста LZ77: %u байт\n", reader->header.lz77_context_size);

  if (!compressed_archive_header_is_valid(&reader->header))
  {
    printf("Неверный заголовок архива!\n");
    result = RESULT_ERROR;
    goto error;
  }

  printf("\n=== Чтение таблицы файлов ===\n");
  file_seek(reader->archive_file, COMPRESSED_ARCHIVE_HEADER_SIZE, SEEK_SET);
  result = file_table_read(reader->file_table, reader->archive_file);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при чтении таблицы файлов!\n");
    goto error;
  }

  printf("Файлов в архиве: %u\n", file_table_get_count(reader->file_table));
  for (DWord i = 0; i < file_table_get_count(reader->file_table); i++)
  {
    const FileEntry* entry = file_table_get_entry(reader->file_table, i);
    printf("  Файл %u: %s (исходный: %llu, сжатый: %llu, смещение: %llu)\n",
           i + 1, entry->filename, entry->original_size, entry->compressed_size,
           entry->offset);
  }

  // Чтение моделей/деревьев сжатия
  QWord model_offset = COMPRESSED_ARCHIVE_HEADER_SIZE;
  model_offset += sizeof(DWord);  // file_count
  model_offset += file_table_get_count(reader->file_table) * sizeof(FileEntry);

  Size primary_model_size = 0;
  Byte* primary_model_data = NULL;

  Size secondary_context_size = 0;
  Byte* secondary_context_data = NULL;

  // Для обратной совместимости с версией 2.0
  if (reader->header.version_minor == 0)
  {
    // Используем старый формат
    primary_model_size =
      reader->header.huffman_tree_size > 0 ? reader->header.huffman_tree_size
      : reader->header.arithmetic_model_size > 0
        ? reader->header.arithmetic_model_size
      : reader->header.shannon_tree_size > 0 ? reader->header.shannon_tree_size
      : reader->header.rle_context_size > 0  ? reader->header.rle_context_size
      : reader->header.lz78_context_size > 0 ? reader->header.lz78_context_size
      : reader->header.lz77_context_size > 0 ? reader->header.lz77_context_size
                                             : 0;
  }
  else
  {
    // Используем новый формат для версии 2.1+
    primary_model_size = reader->header.primary_tree_model_size;
    secondary_context_size = reader->header.secondary_context_size;
  }

  // Чтение модели первичного алгоритма
  if (primary_model_size > 0)
  {
    printf("\n=== Чтение модели первичного алгоритма ===\n");
    printf("Смещение модели: %llu байт\n", model_offset);
    printf("Размер модели: %zu байт\n", primary_model_size);

    primary_model_data = (Byte*)malloc(primary_model_size * sizeof(Byte));
    if (primary_model_data == NULL)
    {
      printf("Произошла ошибка выделения памяти!\n");
      goto error;
    }

    result = file_read_at(reader->archive_file, primary_model_data,
                          primary_model_size, model_offset);
    if (result != RESULT_OK)
    {
      printf("Произошла ошибка при чтении модели первичного алгоритма!\n");
      free(primary_model_data);
      goto error;
    }

    printf("Модель прочитана успешно\n");

    // Десериализация в зависимости от алгоритма
    if (reader->header.primary_compression == COMPRESSION_HUFFMAN)
    {
      reader->huffman_tree = huffman_tree_create();
      if (reader->huffman_tree == NULL)
      {
        printf("Произошла ошибка при создании дерева Хаффмана!\n");
        free(primary_model_data);
        goto error;
      }

      result = huffman_deserialize_tree(reader->huffman_tree,
                                        primary_model_data, primary_model_size);
      if (result != RESULT_OK)
      {
        printf("Произошла ошибка при десериализации дерева Хаффмана!\n");
        free(primary_model_data);
        goto error;
      }

      printf("Дерево Хаффмана десериализовано успешно\n");
    }
    else if (reader->header.primary_compression == COMPRESSION_ARITHMETIC)
    {
      reader->arithmetic_model = arithmetic_model_create();
      if (reader->arithmetic_model == NULL)
      {
        printf("Произошла ошибка при создании арифметической модели!\n");
        free(primary_model_data);
        goto error;
      }

      result = arithmetic_deserialize_model(
        reader->arithmetic_model, primary_model_data, primary_model_size);
      if (result != RESULT_OK)
      {
        printf("Произошла ошибка при десериализации арифметической модели!\n");
        free(primary_model_data);
        goto error;
      }

      printf("Арифметическая модель десериализована успешно\n");
    }
    else if (reader->header.primary_compression == COMPRESSION_SHANNON)
    {
      reader->shannon_tree = shannon_tree_create();
      if (reader->shannon_tree == NULL)
      {
        printf("Произошла ошибка при создании дерева Шеннона!\n");
        free(primary_model_data);
        goto error;
      }

      result = shannon_deserialize_tree(reader->shannon_tree,
                                        primary_model_data, primary_model_size);
      if (result != RESULT_OK)
      {
        printf("Произошла ошибка при десериализации дерева Шеннона!\n");
        free(primary_model_data);
        goto error;
      }

      printf("Дерево Шеннона десериализовано успешно\n");
    }
    else if (reader->header.primary_compression == COMPRESSION_RLE)
    {
      reader->rle_context = rle_create(0);
      if (reader->rle_context == NULL)
      {
        printf("Произошла ошибка при создании контекста RLE!\n");
        free(primary_model_data);
        goto error;
      }

      result = rle_deserialize_context(reader->rle_context, primary_model_data,
                                       primary_model_size);
      if (result != RESULT_OK)
      {
        printf("Произошла ошибка при десериализации контекста RLE!\n");
        free(primary_model_data);
        goto error;
      }

      printf("Контекст RLE десериализован успешно\n");
      printf("Префикс RLE: 0x%02X\n", rle_get_prefix(reader->rle_context));
    }
    else if (reader->header.primary_compression == COMPRESSION_LZ77)
    {
      reader->lz77_context_data = primary_model_data;
      reader->lz77_context_size = primary_model_size;
      printf("Контекст LZ77 прочитан успешно\n");
      if (primary_model_size >= 1)
      {
        printf("Префикс LZ77: 0x%02X\n", primary_model_data[0]);
      }
    }

    free(primary_model_data);
    model_offset += primary_model_size;
  }

  // Чтение контекста вторичного алгоритма (только для двухэтапного сжатия)
  if (secondary_context_size > 0 &&
      (reader->header.flags & FLAG_TWO_STAGE_COMPRESSION))
  {
    printf("\n=== Чтение контекста вторичного алгоритма ===\n");
    printf("Смещение контекста: %llu байт\n", model_offset);
    printf("Размер контекста: %zu байт\n", secondary_context_size);

    secondary_context_data =
      (Byte*)malloc(secondary_context_size * sizeof(Byte));
    if (secondary_context_data == NULL)
    {
      printf("Произошла ошибка выделения памяти!\n");
      goto error;
    }

    result = file_read_at(reader->archive_file, secondary_context_data,
                          secondary_context_size, model_offset);
    if (result != RESULT_OK)
    {
      printf("Произошла ошибка при чтении контекста вторичного алгоритма!\n");
      free(secondary_context_data);
      goto error;
    }

    printf("Контекст прочитан успешно\n");

    // Десериализация в зависимости от вторичного алгоритма
    if (reader->header.secondary_compression == COMPRESSION_HUFFMAN)
    {
      reader->secondary_huffman_tree = huffman_tree_create();
      if (reader->secondary_huffman_tree == NULL)
      {
        printf(
          "Произошла ошибка при создании дерева Хаффмана для вторичного "
          "алгоритма!\n");
        free(secondary_context_data);
        goto error;
      }

      result = huffman_deserialize_tree(reader->secondary_huffman_tree,
                                        secondary_context_data,
                                        secondary_context_size);
      if (result != RESULT_OK)
      {
        printf(
          "Произошла ошибка при десериализации дерева Хаффмана для вторичного "
          "алгоритма!\n");
        free(secondary_context_data);
        goto error;
      }

      printf(
        "Дерево Хаффмана для вторичного алгоритма десериализовано успешно\n");
    }
    else if (reader->header.secondary_compression == COMPRESSION_ARITHMETIC)
    {
      reader->secondary_arithmetic_model = arithmetic_model_create();
      if (reader->secondary_arithmetic_model == NULL)
      {
        printf(
          "Произошла ошибка при создании арифметической модели для вторичного "
          "алгоритма!\n");
        free(secondary_context_data);
        goto error;
      }

      result = arithmetic_deserialize_model(reader->secondary_arithmetic_model,
                                            secondary_context_data,
                                            secondary_context_size);
      if (result != RESULT_OK)
      {
        printf(
          "Произошла ошибка при десериализации арифметической модели для "
          "вторичного алгоритма!\n");
        free(secondary_context_data);
        goto error;
      }

      printf(
        "Арифметическая модель для вторичного алгоритма десериализована "
        "успешно\n");
    }
    else if (reader->header.secondary_compression == COMPRESSION_SHANNON)
    {
      reader->secondary_shannon_tree = shannon_tree_create();
      if (reader->secondary_shannon_tree == NULL)
      {
        printf(
          "Произошла ошибка при создании дерева Шеннона для вторичного "
          "алгоритма!\n");
        free(secondary_context_data);
        goto error;
      }

      result = shannon_deserialize_tree(reader->secondary_shannon_tree,
                                        secondary_context_data,
                                        secondary_context_size);
      if (result != RESULT_OK)
      {
        printf(
          "Произошла ошибка при десериализации дерева Шеннона для вторичного "
          "алгоритма!\n");
        free(secondary_context_data);
        goto error;
      }

      printf(
        "Дерево Шеннона для вторичного алгоритма десериализовано успешно\n");
    }
    else if (reader->header.secondary_compression == COMPRESSION_RLE)
    {
      reader->secondary_rle_context = rle_create(0);
      if (reader->secondary_rle_context == NULL)
      {
        printf(
          "Произошла ошибка при создании контекста RLE для вторичного "
          "алгоритма!\n");
        free(secondary_context_data);
        goto error;
      }

      result =
        rle_deserialize_context(reader->secondary_rle_context,
                                secondary_context_data, secondary_context_size);
      if (result != RESULT_OK)
      {
        printf(
          "Произошла ошибка при десериализации контекста RLE для вторичного "
          "алгоритма!\n");
        free(secondary_context_data);
        goto error;
      }

      printf("Контекст RLE для вторичного алгоритма десериализован успешно\n");
      printf("Префикс RLE: 0x%02X\n",
             rle_get_prefix(reader->secondary_rle_context));
    }
    else if (reader->header.secondary_compression == COMPRESSION_LZ77)
    {
      reader->secondary_lz77_context_data = secondary_context_data;
      reader->secondary_lz77_context_size = secondary_context_size;
      printf("Контекст LZ77 для вторичного алгоритма прочитан успешно\n");
      if (secondary_context_size >= 1)
      {
        printf("Префикс LZ77: 0x%02X\n", secondary_context_data[0]);
      }
    }

    free(secondary_context_data);
  }

  printf("\n=== Архив успешно открыт ===\n");
  if (reader->header.flags & FLAG_TWO_STAGE_COMPRESSION)
  {
    printf("Режим: ДВУХЭТАПНОЕ СЖАТИЕ\n");
    printf("Первичный алгоритм: %s\n",
           reader->header.primary_compression == COMPRESSION_HUFFMAN ? "HUFFMAN"
           : reader->header.primary_compression == COMPRESSION_ARITHMETIC
             ? "ARITHMETIC"
           : reader->header.primary_compression == COMPRESSION_SHANNON
             ? "SHANNON"
           : reader->header.primary_compression == COMPRESSION_RLE  ? "RLE"
           : reader->header.primary_compression == COMPRESSION_LZ78 ? "LZ78"
           : reader->header.primary_compression == COMPRESSION_LZ77 ? "LZ77"
                                                                    : "NONE");
    printf(
      "Вторичный алгоритм: %s\n",
      reader->header.secondary_compression == COMPRESSION_HUFFMAN ? "HUFFMAN"
      : reader->header.secondary_compression == COMPRESSION_ARITHMETIC
        ? "ARITHMETIC"
      : reader->header.secondary_compression == COMPRESSION_SHANNON ? "SHANNON"
      : reader->header.secondary_compression == COMPRESSION_RLE     ? "RLE"
      : reader->header.secondary_compression == COMPRESSION_LZ78    ? "LZ78"
      : reader->header.secondary_compression == COMPRESSION_LZ77    ? "LZ77"
                                                                    : "NONE");
  }
  else
  {
    printf("Режим: ОДНОЭТАПНОЕ СЖАТИЕ\n");
  }

  return reader;

error:
  compressed_archive_reader_destroy(reader);
  return NULL;
}

void compressed_archive_reader_destroy(CompressedArchiveReader* self)
{
  if (self == NULL)
  {
    return;
  }

  if (self->huffman_tree)
  {
    huffman_tree_destroy(self->huffman_tree);
  }

  if (self->arithmetic_model)
  {
    arithmetic_model_destroy(self->arithmetic_model);
  }

  if (self->shannon_tree)
  {
    shannon_tree_destroy(self->shannon_tree);
  }

  if (self->rle_context)
  {
    rle_destroy(self->rle_context);
  }

  if (self->lz78_context)
  {
    lz78_destroy(self->lz78_context);
  }

  if (self->lz77_context_data)
  {
    free(self->lz77_context_data);
  }

  if (self->secondary_huffman_tree)
  {
    huffman_tree_destroy(self->secondary_huffman_tree);
  }

  if (self->secondary_arithmetic_model)
  {
    arithmetic_model_destroy(self->secondary_arithmetic_model);
  }

  if (self->secondary_shannon_tree)
  {
    shannon_tree_destroy(self->secondary_shannon_tree);
  }

  if (self->secondary_rle_context)
  {
    rle_destroy(self->secondary_rle_context);
  }

  if (self->secondary_lz77_context_data)
  {
    free(self->secondary_lz77_context_data);
  }

  file_table_destroy(self->file_table);
  file_close(self->archive_file);
  file_destroy(self->archive_file);
  free(self);
}

// Функция для двухэтапной декомпрессии
// Функция для двухэтапной декомпрессии
static Result apply_two_stage_decompression(
  const Byte* input, Size input_size, Byte** output, Size* output_size,
  CompressionAlgorithm primary_algo, CompressionAlgorithm secondary_algo,
  void* primary_context, void* secondary_context, bool primary_failed)
{
  printf("[TWO-STAGE] Начало двухэтапной декомпрессии\n");
  printf("[TWO-STAGE] Входной размер: %zu байт\n", input_size);
  printf("[TWO-STAGE] Ожидаемый выход: %zu байт\n", *output_size);

  // ИСПРАВЛЕНИЕ: Если первичное сжатие не применялось при кодировании,
  // не применяем его и при декодировании
  if (primary_failed)
  {
    printf("[TWO-STAGE] Первичное сжатие не применялось, пропускаем этап 2\n");
    // Пропускаем этап первичной декомпрессии
    primary_algo = COMPRESSION_NONE;
  }

  Result result = RESULT_OK;
  Byte* stage1_output = NULL;
  Size stage1_size = *output_size;

  // Этап 1: Декомпрессия вторичного алгоритма
  if (secondary_algo != COMPRESSION_NONE)
  {
    printf("[TWO-STAGE] Этап 1: Декомпрессия вторичного алгоритма (%s)\n",
           secondary_algo == COMPRESSION_HUFFMAN      ? "HUFFMAN"
           : secondary_algo == COMPRESSION_ARITHMETIC ? "ARITHMETIC"
           : secondary_algo == COMPRESSION_SHANNON    ? "SHANNON"
           : secondary_algo == COMPRESSION_RLE        ? "RLE"
           : secondary_algo == COMPRESSION_LZ78       ? "LZ78"
           : secondary_algo == COMPRESSION_LZ77       ? "LZ77"
                                                      : "NONE");

    if (secondary_algo == COMPRESSION_HUFFMAN && secondary_context)
    {
      result =
        huffman_decompress(input, input_size, &stage1_output, &stage1_size,
                           (HuffmanTree*)secondary_context);
    }
    else if (secondary_algo == COMPRESSION_ARITHMETIC && secondary_context)
    {
      result =
        arithmetic_decompress(input, input_size, &stage1_output, &stage1_size,
                              (ArithmeticModel*)secondary_context);
    }
    else if (secondary_algo == COMPRESSION_SHANNON && secondary_context)
    {
      result =
        shannon_decompress(input, input_size, &stage1_output, &stage1_size,
                           (ShannonTree*)secondary_context);
    }
    else if (secondary_algo == COMPRESSION_RLE && secondary_context)
    {
      result = rle_decompress(input, input_size, &stage1_output, &stage1_size,
                              (RLEContext*)secondary_context);
    }
    else if (secondary_algo == COMPRESSION_LZ78)
    {
      result = lz78_decompress(input, input_size, &stage1_output, &stage1_size);
    }
    else if (secondary_algo == COMPRESSION_LZ77 && secondary_context)
    {
      Byte prefix = *(Byte*)secondary_context;
      result = lz77_decompress(input, input_size, &stage1_output, &stage1_size,
                               prefix);
    }
    else
    {
      // Без вторичной декомпрессии
      stage1_output = (Byte*)malloc(input_size);
      if (stage1_output)
      {
        memcpy(stage1_output, input, input_size);
        stage1_size = input_size;
      }
      else
      {
        result = RESULT_MEMORY_ERROR;
      }
    }

    if (result != RESULT_OK || stage1_output == NULL)
    {
      printf("[TWO-STAGE] Ошибка на этапе 1 декомпрессии\n");
      return result != RESULT_OK ? result : RESULT_ERROR;
    }

    printf("[TWO-STAGE] Этап 1 завершен: %zu -> %zu байт\n", input_size,
           stage1_size);
  }
  else
  {
    // Без вторичной декомпрессии
    stage1_output = (Byte*)malloc(input_size);
    if (stage1_output)
    {
      memcpy(stage1_output, input, input_size);
      stage1_size = input_size;
    }
    else
    {
      return RESULT_MEMORY_ERROR;
    }
    printf("[TWO-STAGE] Вторичная декомпрессия отключена\n");
  }

  // Этап 2: Декомпрессия первичного алгоритма
  if (primary_algo != COMPRESSION_NONE && !primary_failed)
  {
    printf("[TWO-STAGE] Этап 2: Декомпрессия первичного алгоритма (%s)\n",
           primary_algo == COMPRESSION_HUFFMAN      ? "HUFFMAN"
           : primary_algo == COMPRESSION_ARITHMETIC ? "ARITHMETIC"
           : primary_algo == COMPRESSION_SHANNON    ? "SHANNON"
           : primary_algo == COMPRESSION_RLE        ? "RLE"
           : primary_algo == COMPRESSION_LZ78       ? "LZ78"
           : primary_algo == COMPRESSION_LZ77       ? "LZ77"
                                                    : "NONE");

    if (primary_algo == COMPRESSION_HUFFMAN && primary_context)
    {
      result = huffman_decompress(stage1_output, stage1_size, output,
                                  output_size, (HuffmanTree*)primary_context);
    }
    else if (primary_algo == COMPRESSION_ARITHMETIC && primary_context)
    {
      result =
        arithmetic_decompress(stage1_output, stage1_size, output, output_size,
                              (ArithmeticModel*)primary_context);
    }
    else if (primary_algo == COMPRESSION_SHANNON && primary_context)
    {
      result = shannon_decompress(stage1_output, stage1_size, output,
                                  output_size, (ShannonTree*)primary_context);
    }
    else if (primary_algo == COMPRESSION_RLE && primary_context)
    {
      result = rle_decompress(stage1_output, stage1_size, output, output_size,
                              (RLEContext*)primary_context);
    }
    else if (primary_algo == COMPRESSION_LZ78)
    {
      result = lz78_decompress(stage1_output, stage1_size, output, output_size);
    }
    else if (primary_algo == COMPRESSION_LZ77 && primary_context)
    {
      Byte prefix = *(Byte*)primary_context;
      result = lz77_decompress(stage1_output, stage1_size, output, output_size,
                               prefix);
    }
    else
    {
      // Без первичной декомпрессии
      *output = stage1_output;
      *output_size = stage1_size;
      printf("[TWO-STAGE] Первичная декомпрессия не требуется\n");
      return RESULT_OK;
    }

    free(stage1_output);

    if (result != RESULT_OK)
    {
      printf("[TWO-STAGE] Ошибка на этапе 2 декомпрессии\n");
      return result;
    }

    printf("[TWO-STAGE] Этап 2 завершен: %zu -> %zu байт\n", stage1_size,
           *output_size);
  }
  else
  {
    // Без первичной декомпрессии
    *output = stage1_output;
    *output_size = stage1_size;
    printf("[TWO-STAGE] Первичная декомпрессия отключена\n");
  }

  printf("[TWO-STAGE] Декомпрессия завершена успешно\n");
  return RESULT_OK;
}

static Result extract_single_file(CompressedArchiveReader* self,
                                  DWord file_index, const char* output_path)
{
  const FileEntry* entry = file_table_get_entry(self->file_table, file_index);
  if (entry == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  printf("\n=== Извлечение файла ===\n");
  printf("Файл: %s\n", entry->filename);
  printf("Исходный размер: %llu байт\n", entry->original_size);
  printf("Сжатый размер: %llu байт\n", entry->compressed_size);
  printf("Смещение в архиве: %llu байт\n", entry->offset);

  Byte* file_data = (Byte*)malloc(entry->compressed_size * sizeof(Byte));

  if (file_data == NULL)
  {
    printf("Произошла ошибка при выделении памяти для данных файла!\n");
    return RESULT_MEMORY_ERROR;
  }

  Result result = file_read_at(self->archive_file, file_data,
                               entry->compressed_size, entry->offset);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при чтении данных файла из архива!\n");
    free(file_data);
    return result;
  }

  printf("Данные прочитаны успешно (%llu байт)\n", entry->compressed_size);

  Byte* final_data = NULL;
  Size final_size = entry->original_size;

  bool needs_decompression = (self->header.flags & FLAG_COMPRESSED);

  if (needs_decompression)
  {
    printf("Требуется декомпрессия...\n");

    if (self->header.flags & FLAG_TWO_STAGE_COMPRESSION)
    {
      printf("Режим: ДВУХЭТАПНОЕ СЖАТИЕ\n");
      printf("Первичный алгоритм: %s\n",
             self->header.primary_compression == COMPRESSION_HUFFMAN ? "HUFFMAN"
             : self->header.primary_compression == COMPRESSION_ARITHMETIC
               ? "ARITHMETIC"
             : self->header.primary_compression == COMPRESSION_SHANNON
               ? "SHANNON"
             : self->header.primary_compression == COMPRESSION_RLE  ? "RLE"
             : self->header.primary_compression == COMPRESSION_LZ78 ? "LZ78"
             : self->header.primary_compression == COMPRESSION_LZ77 ? "LZ77"
                                                                    : "NONE");
      printf(
        "Вторичный алгоритм: %s\n",
        self->header.secondary_compression == COMPRESSION_HUFFMAN ? "HUFFMAN"
        : self->header.secondary_compression == COMPRESSION_ARITHMETIC
          ? "ARITHMETIC"
        : self->header.secondary_compression == COMPRESSION_SHANNON ? "SHANNON"
        : self->header.secondary_compression == COMPRESSION_RLE     ? "RLE"
        : self->header.secondary_compression == COMPRESSION_LZ78    ? "LZ78"
        : self->header.secondary_compression == COMPRESSION_LZ77    ? "LZ77"
                                                                    : "NONE");

      // Получаем контексты для алгоритмов
      void* primary_context = NULL;
      void* secondary_context = NULL;

      // Определяем контекст для первичного алгоритма
      if (self->header.primary_compression == COMPRESSION_HUFFMAN)
      {
        primary_context = self->huffman_tree;
      }
      else if (self->header.primary_compression == COMPRESSION_ARITHMETIC)
      {
        primary_context = self->arithmetic_model;
      }
      else if (self->header.primary_compression == COMPRESSION_SHANNON)
      {
        primary_context = self->shannon_tree;
      }
      else if (self->header.primary_compression == COMPRESSION_RLE)
      {
        primary_context = self->rle_context;
      }
      else if (self->header.primary_compression == COMPRESSION_LZ77)
      {
        primary_context = self->lz77_context_data;
      }

      // Определяем контекст для вторичного алгоритма
      if (self->header.secondary_compression == COMPRESSION_HUFFMAN)
      {
        secondary_context = self->secondary_huffman_tree;
      }
      else if (self->header.secondary_compression == COMPRESSION_ARITHMETIC)
      {
        secondary_context = self->secondary_arithmetic_model;
      }
      else if (self->header.secondary_compression == COMPRESSION_SHANNON)
      {
        secondary_context = self->secondary_shannon_tree;
      }
      else if (self->header.secondary_compression == COMPRESSION_RLE)
      {
        secondary_context = self->secondary_rle_context;
      }
      else if (self->header.secondary_compression == COMPRESSION_LZ77)
      {
        secondary_context = self->secondary_lz77_context_data;
      }

      bool primary_failed = (entry->compressed_size == entry->original_size);

      result = apply_two_stage_decompression(
        file_data, entry->compressed_size, &final_data, &final_size,
        self->header.primary_compression, self->header.secondary_compression,
        primary_context, secondary_context, primary_failed);
    }
    else
    {
      // Одноэтапное сжатие
      printf("Режим: ОДНОЭТАПНОЕ СЖАТИЕ\n");
      printf(
        "Алгоритм сжатия: %s\n",
        self->header.primary_compression == COMPRESSION_HUFFMAN ? "HUFFMAN"
        : self->header.primary_compression == COMPRESSION_ARITHMETIC
          ? "ARITHMETIC"
        : self->header.primary_compression == COMPRESSION_SHANNON ? "SHANNON"
        : self->header.primary_compression == COMPRESSION_RLE     ? "RLE"
        : self->header.primary_compression == COMPRESSION_LZ78    ? "LZ78"
        : self->header.primary_compression == COMPRESSION_LZ77    ? "LZ77"
                                                                  : "UNKNOWN");

      final_data = malloc(entry->original_size);
      if (final_data == NULL)
      {
        printf("Произошла ошибка при выделении памяти!\n");
        free(file_data);
        return RESULT_MEMORY_ERROR;
      }

      Size expected_size = entry->original_size;

      if (self->header.primary_compression == COMPRESSION_HUFFMAN &&
          self->huffman_tree != NULL)
      {
        printf("Декомпрессия методом Хаффмана...\n");
        printf("  Входные данные: %llu байт\n", entry->compressed_size);
        printf("  Ожидаемый размер: %llu байт\n", entry->original_size);

        result =
          huffman_decompress(file_data, entry->compressed_size, &final_data,
                             &expected_size, self->huffman_tree);
      }
      else if (self->header.primary_compression == COMPRESSION_ARITHMETIC &&
               self->arithmetic_model != NULL)
      {
        printf("Декомпрессия арифметическим методом...\n");
        printf("  Входные данные: %llu байт\n", entry->compressed_size);
        printf("  Ожидаемый размер: %llu байт\n", entry->original_size);

        result =
          arithmetic_decompress(file_data, entry->compressed_size, &final_data,
                                &expected_size, self->arithmetic_model);
      }
      else if (self->header.primary_compression == COMPRESSION_SHANNON &&
               self->shannon_tree != NULL)
      {
        printf("Декомпрессия методом Шеннона...\n");
        printf("  Входные данные: %llu байт\n", entry->compressed_size);
        printf("  Ожидаемый размер: %llu байт\n", entry->original_size);

        result =
          shannon_decompress(file_data, entry->compressed_size, &final_data,
                             &expected_size, self->shannon_tree);
      }
      else if (self->header.primary_compression == COMPRESSION_RLE &&
               self->rle_context != NULL)
      {
        printf("Декомпрессия методом RLE...\n");
        printf("  Входные данные: %llu байт\n", entry->compressed_size);
        printf("  Ожидаемый размер: %llu байт\n", entry->original_size);
        printf("  Префикс RLE: 0x%02X\n", rle_get_prefix(self->rle_context));

        result = rle_decompress(file_data, entry->compressed_size, &final_data,
                                &expected_size, self->rle_context);
      }
      else if (self->header.primary_compression == COMPRESSION_LZ78)
      {
        printf("Декомпрессия методом LZ78...\n");
        printf("  Входные данные: %llu байт\n", entry->compressed_size);
        printf("  Ожидаемый размер: %llu байт\n", entry->original_size);

        result = lz78_decompress(file_data, entry->compressed_size, &final_data,
                                 &expected_size);
      }
      else if (self->header.primary_compression == COMPRESSION_LZ77)
      {
        printf("Декомпрессия методом LZ77...\n");
        printf("  Входные данные: %llu байт\n", entry->compressed_size);
        printf("  Ожидаемый размер: %llu байт\n", entry->original_size);

        // LZ77 использует только префикс из контекста
        Byte prefix = 0;
        if (self->lz77_context_data && self->lz77_context_size >= 1)
        {
          prefix = self->lz77_context_data[0];
        }
        else
        {
          printf("  ВНИМАНИЕ: префикс LZ77 не найден, используется 0x00\n");
        }

        printf("  Префикс LZ77: 0x%02X\n", prefix);

        result = lz77_decompress(file_data, entry->compressed_size, &final_data,
                                 &expected_size, prefix);
      }
      else
      {
        printf(
          "Произошла ошибка: алгоритм сжатия не поддерживается или модель "
          "отсутствует!\n");
        result = RESULT_ERROR;
      }

      if (result == RESULT_OK)
      {
        printf("Декомпрессия успешна!\n");
        printf("  Фактический размер после декомпрессии: %zu байт\n",
               expected_size);

        final_size = expected_size;
        free(file_data);
      }
      else
      {
        printf("Ошибка декомпрессии! Код ошибки: %d\n", result);
        free(final_data);
        final_data = file_data;
        final_size = entry->compressed_size;
      }
    }
  }
  else
  {
    printf("Декомпрессия не требуется\n");
    final_data = file_data;
    final_size = entry->compressed_size;
  }

  printf("Запись файла: %s\n", output_path);
  File* output_file = file_create(output_path);
  if (output_file == NULL)
  {
    printf("Произошла ошибка при создании выходного файла!\n");
    free(final_data);
    return RESULT_MEMORY_ERROR;
  }

  result = file_open_for_write(output_file);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при открытии выходного файла для записи!\n");
    file_destroy(output_file);
    free(final_data);
    return result;
  }

  printf("Записываем %zu байт...\n", final_size);
  result = file_write_bytes(output_file, final_data, final_size);

  free(final_data);
  file_close(output_file);
  file_destroy(output_file);

  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при записи файла!\n");
  }
  else
  {
    printf("Файл успешно записан\n");
  }

  return result;
}

Result compressed_archive_reader_extract_all(CompressedArchiveReader* self,
                                             const char* output_path)
{
  if (self == NULL || !output_path)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  printf("\n=== Начало извлечения архива ===\n");
  printf("Целевая директория: %s\n", output_path);

  if (self->header.flags & FLAG_DIRECTORY)
  {
    if (!path_utils_exists(output_path))
    {
      printf("Создание директории: %s\n", output_path);
      path_utils_create_directory(output_path);
    }
  }

  for (DWord i = 0; i < file_table_get_count(self->file_table); i++)
  {
    const FileEntry* entry = file_table_get_entry(self->file_table, i);

    char output_file_path[PATH_LIMIT];
    if (self->header.flags & FLAG_DIRECTORY)
    {
      snprintf(output_file_path, sizeof(output_file_path), "%s/%s", output_path,
               entry->filename);

      char* parent = path_utils_get_parent(output_file_path);
      if (parent)
      {
        if (!path_utils_exists(parent))
        {
          printf("Создание поддиректории: %s\n", parent);
          path_utils_create_directory_recursive(parent);
        }
        free(parent);
      }
    }
    else
    {
      strncpy(output_file_path, output_path, sizeof(output_file_path));
    }

    printf("\n--- Файл %u/%u ---\n", i + 1,
           file_table_get_count(self->file_table));
    Result result = extract_single_file(self, i, output_file_path);
    if (result != RESULT_OK)
    {
      printf("Ошибка извлечения файла: %s\n", entry->filename);
      return result;
    }
  }

  printf("\nАрхив успешно извлечен!\n");
  return RESULT_OK;
}

Result compressed_archive_reader_extract_file(CompressedArchiveReader* self,
                                              DWord file_index,
                                              const char* output_path)
{
  if (self == NULL || file_index >= file_table_get_count(self->file_table) ||
      output_path == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  return extract_single_file(self, file_index, output_path);
}

DWord compressed_archive_reader_get_file_count(
  const CompressedArchiveReader* self)
{
  return self ? file_table_get_count(self->file_table) : 0;
}

const char* compressed_archive_reader_get_filename(
  const CompressedArchiveReader* self, DWord index)
{
  if (self == NULL || index >= file_table_get_count(self->file_table))
  {
    return NULL;
  }

  const FileEntry* entry = file_table_get_entry(self->file_table, index);
  return entry ? entry->filename : NULL;
}
