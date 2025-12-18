#include "compressed_archive_reader.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arithmetic.h"
#include "compressed_archive_header.h"
#include "file_table.h"
#include "huffman.h"
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
  printf("Алгоритм сжатия: %u\n", reader->header.primary_compression);
  printf("Размер дерева Хаффмана: %u байт\n", reader->header.huffman_tree_size);
  printf("Размер арифметической модели: %u байт\n",
         reader->header.arithmetic_model_size);
  printf("Размер дерева Шеннона: %u байт\n", reader->header.shannon_tree_size);
  printf("Размер контекста RLE: %u байт\n", reader->header.rle_context_size);
  printf("Размер контекста LZ78: %u байт\n", reader->header.lz78_context_size);

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

  if ((reader->header.flags &
       (FLAG_HUFFMAN_TREE | FLAG_ARITHMETIC_MODEL | FLAG_SHANNON_TREE |
        FLAG_RLE_CONTEXT | FLAG_LZ78_CONTEXT)) &&
      (reader->header.huffman_tree_size > 0 ||
       reader->header.arithmetic_model_size > 0 ||
       reader->header.shannon_tree_size > 0 ||
       reader->header.rle_context_size > 0 ||
       reader->header.lz78_context_size > 0))
  {
    QWord model_offset = COMPRESSED_ARCHIVE_HEADER_SIZE;
    model_offset += sizeof(DWord);  // file_count
    model_offset +=
      file_table_get_count(reader->file_table) * sizeof(FileEntry);

    Size model_size = 0;
    Byte* model_data = NULL;

    if ((reader->header.flags & FLAG_HUFFMAN_TREE) &&
        reader->header.huffman_tree_size > 0 &&
        reader->header.primary_compression == COMPRESSION_HUFFMAN)
    {
      printf("\n=== Чтение дерева Хаффмана ===\n");
      printf("Смещение дерева: %llu байт\n", model_offset);
      printf("Размер дерева: %u байт\n", reader->header.huffman_tree_size);

      model_size = reader->header.huffman_tree_size;
      model_data = (Byte*)malloc(model_size * sizeof(Byte));
      if (model_data == NULL)
      {
        printf("Произошла ошибка выделения памяти!\n");
        goto error;
      }

      result = file_read_at(reader->archive_file, model_data, model_size,
                            model_offset);
      if (result != RESULT_OK)
      {
        printf("Произошла ошибка при чтении дерева Хаффмана!\n");
        free(model_data);
        goto error;
      }

      printf("Дерево прочитано успешно\n");

      reader->huffman_tree = huffman_tree_create();

      if (reader->huffman_tree == NULL)
      {
        printf("Произошла ошибка при создании дерева Хаффмана!\n");
        free(model_data);
        goto error;
      }

      result =
        huffman_deserialize_tree(reader->huffman_tree, model_data, model_size);

      free(model_data);

      if (result != RESULT_OK)
      {
        printf("Произошла ошибка при десериализации дерева Хаффмана!\n");
        huffman_tree_destroy(reader->huffman_tree);
        reader->huffman_tree = NULL;
        goto error;
      }

      printf("Дерево Хаффмана десериализовано успешно\n");
    }
    else if ((reader->header.flags & FLAG_ARITHMETIC_MODEL) &&
             reader->header.arithmetic_model_size > 0 &&
             reader->header.primary_compression == COMPRESSION_ARITHMETIC)
    {
      printf("\n=== Чтение арифметической модели ===\n");
      printf("Смещение модели: %llu байт\n", model_offset);
      printf("Размер модели: %u байт\n", reader->header.arithmetic_model_size);

      model_size = reader->header.arithmetic_model_size;
      model_data = (Byte*)malloc(model_size * sizeof(Byte));

      if (model_data == NULL)
      {
        printf("Произошла ошибка при выделении памяти!\n");
        goto error;
      }

      result = file_read_at(reader->archive_file, model_data, model_size,
                            model_offset);
      if (result != RESULT_OK)
      {
        printf("Произошла ошибка при чтении арифметической модели!\n");
        free(model_data);
        goto error;
      }

      printf("Модель прочитана успешно\n");

      reader->arithmetic_model = arithmetic_model_create();

      if (reader->arithmetic_model == NULL)
      {
        printf("Произошла ошибка при создании арифметической модели!\n");
        free(model_data);
        goto error;
      }

      result = arithmetic_deserialize_model(reader->arithmetic_model,
                                            model_data, model_size);

      free(model_data);

      if (result != RESULT_OK)
      {
        printf("Произошла ошибка при десериализации арифметической модели!\n");
        arithmetic_model_destroy(reader->arithmetic_model);
        reader->arithmetic_model = NULL;
        goto error;
      }

      printf("Арифметическая модель десериализована успешно\n");
    }
    else if ((reader->header.flags & FLAG_SHANNON_TREE) &&
             reader->header.shannon_tree_size > 0 &&
             reader->header.primary_compression == COMPRESSION_SHANNON)
    {
      printf("\n=== Чтение дерева Шеннона ===\n");
      printf("Смещение дерева: %llu байт\n", model_offset);
      printf("Размер дерева: %u байт\n", reader->header.shannon_tree_size);

      model_size = reader->header.shannon_tree_size;
      model_data = (Byte*)malloc(model_size * sizeof(Byte));
      if (model_data == NULL)
      {
        printf("Произошла ошибка выделения памяти!\n");
        goto error;
      }

      result = file_read_at(reader->archive_file, model_data, model_size,
                            model_offset);
      if (result != RESULT_OK)
      {
        printf("Произошла ошибка при чтении дерева Шеннона!\n");
        free(model_data);
        goto error;
      }

      printf("Дерево прочитано успешно\n");

      reader->shannon_tree = shannon_tree_create();

      if (reader->shannon_tree == NULL)
      {
        printf("Произошла ошибка при создании дерева Шеннона!\n");
        free(model_data);
        goto error;
      }

      result =
        shannon_deserialize_tree(reader->shannon_tree, model_data, model_size);

      free(model_data);

      if (result != RESULT_OK)
      {
        printf("Произошла ошибка при десериализации дерева Шеннона!\n");
        shannon_tree_destroy(reader->shannon_tree);
        reader->shannon_tree = NULL;
        goto error;
      }

      printf("Дерево Шеннона десериализовано успешно\n");
    }
    else if ((reader->header.flags & FLAG_RLE_CONTEXT) &&
             reader->header.rle_context_size > 0 &&
             reader->header.primary_compression == COMPRESSION_RLE)
    {
      printf("\n=== Чтение контекста RLE ===\n");
      printf("Смещение контекста: %llu байт\n", model_offset);
      printf("Размер контекста: %u байт\n", reader->header.rle_context_size);

      model_size = reader->header.rle_context_size;
      model_data = (Byte*)malloc(model_size * sizeof(Byte));
      if (model_data == NULL)
      {
        printf("Произошла ошибка выделения памяти!\n");
        goto error;
      }

      result = file_read_at(reader->archive_file, model_data, model_size,
                            model_offset);
      if (result != RESULT_OK)
      {
        printf("Произошла ошибка при чтении контекста RLE!\n");
        free(model_data);
        goto error;
      }

      printf("Контекст прочитан успешно\n");

      reader->rle_context = rle_create(0);  // Создаем с временным префиксом

      if (reader->rle_context == NULL)
      {
        printf("Произошла ошибка при создании контекста RLE!\n");
        free(model_data);
        goto error;
      }

      result =
        rle_deserialize_context(reader->rle_context, model_data, model_size);

      free(model_data);

      if (result != RESULT_OK)
      {
        printf("Произошла ошибка при десериализации контекста RLE!\n");
        rle_destroy(reader->rle_context);
        reader->rle_context = NULL;
        goto error;
      }

      printf("Контекст RLE десериализован успешно\n");
      printf("Префикс RLE: 0x%02X\n", rle_get_prefix(reader->rle_context));
    }
    else if ((reader->header.flags & FLAG_LZ78_CONTEXT) &&
             reader->header.lz78_context_size > 0 &&
             reader->header.primary_compression == COMPRESSION_LZ78)
    {
      printf("\n=== Чтение контекста LZ78 ===\n");
      printf("Смещение контекста: %llu байт\n", model_offset);
      printf("Размер контекста: %u байт\n", reader->header.lz78_context_size);

      // LZ78 не имеет сериализуемого контекста, пропускаем данные
      model_size = reader->header.lz78_context_size;
      model_data = (Byte*)malloc(model_size * sizeof(Byte));
      if (model_data == NULL)
      {
        printf("Произошла ошибка выделения памяти!\n");
        goto error;
      }

      result = file_read_at(reader->archive_file, model_data, model_size,
                            model_offset);
      if (result != RESULT_OK)
      {
        printf("Произошла ошибка при чтении контекста LZ78!\n");
        free(model_data);
        goto error;
      }

      // LZ78 не требует десериализации контекста, но создаем пустой контекст
      // для единообразия
      reader->lz78_context = lz78_create();
      if (reader->lz78_context == NULL)
      {
        printf("Произошла ошибка при создании контекста LZ78!\n");
        free(model_data);
        goto error;
      }

      free(model_data);
      printf("Контекст LZ78 создан (LZ78 не требует сериализации контекста)\n");
    }
  }
  else
  {
    printf("\n=== Модель/дерево сжатия отсутствует или не используется ===\n");
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

  file_table_destroy(self->file_table);
  file_close(self->archive_file);
  file_destroy(self->archive_file);
  free(self);
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
  Size final_size = 0;

  bool needs_decompression = (self->header.flags & FLAG_COMPRESSED);

  if (needs_decompression)
  {
    printf("Требуется декомпрессия...\n");
    printf("Алгоритм сжатия: %s\n",
           self->header.primary_compression == COMPRESSION_HUFFMAN ? "HUFFMAN"
           : self->header.primary_compression == COMPRESSION_ARITHMETIC
             ? "ARITHMETIC"
           : self->header.primary_compression == COMPRESSION_SHANNON ? "SHANNON"
           : self->header.primary_compression == COMPRESSION_RLE     ? "RLE"
           : self->header.primary_compression == COMPRESSION_LZ78    ? "LZ78"
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
