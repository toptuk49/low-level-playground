#include "compressed_archive_reader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compressed_archive_header.h"
#include "file_table.h"
#include "path_utils.h"

#define PATH_LIMIT 4096

struct CompressedArchiveReader
{
  File* archive_file;
  FileTable* file_table;
  CompressedArchiveHeader header;
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
    printf("Произошла ошибка при выделении памяти!\n");
    return NULL;
  }

  reader->archive_file = file_create(input_filename);
  if (reader->archive_file == NULL)
  {
    printf("Произошла ошибка при создании файла архива!\n");
    free(reader);
    return NULL;
  }

  reader->file_table = file_table_create();
  if (reader->file_table == NULL)
  {
    printf("Произошла ошибка при создании объекта таблицы файлов!\n");
    file_destroy(reader->archive_file);
    free(reader);
    return NULL;
  }

  Result result = file_open_for_read(reader->archive_file);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при открытии архива для чтения!\n");
    goto error;
  }

  result = file_read_bytes(reader->archive_file);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при получении байтов архива!\n");
    goto error;
  }

  const Byte* data = file_get_buffer(reader->archive_file);
  memcpy(&reader->header, data, COMPRESSED_ARCHIVE_HEADER_SIZE);

  if (!compressed_archive_header_is_valid(&reader->header))
  {
    printf("Заголовок архива некорректный!\n");
    result = RESULT_ERROR;
    goto error;
  }

  result =
    file_seek(reader->archive_file, COMPRESSED_ARCHIVE_HEADER_SIZE, SEEK_SET);
  if (result != RESULT_OK)
  {
    printf("Ошибка позиционирования к таблице файлов!\n");
    goto error;
  }

  result = file_table_read(reader->file_table, reader->archive_file);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при чтении таблицы файлов!\n");
    goto error;
  }

  return reader;

error:
  file_table_destroy(reader->file_table);
  file_destroy(reader->archive_file);
  free(reader);
  return NULL;
}

void compressed_archive_reader_destroy(CompressedArchiveReader* self)
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

static Result extract_single_file(CompressedArchiveReader* self,
                                  DWord file_index, const char* output_path)
{
  const FileEntry* entry = file_table_get_entry(self->file_table, file_index);
  if (entry == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  Byte* file_data = (Byte*)malloc(entry->original_size);
  if (file_data == NULL)
  {
    printf("Произошла ошибка при выделении памяти!\n");
    return RESULT_MEMORY_ERROR;
  }

  Result result = file_read_at(self->archive_file, file_data,
                               entry->original_size, entry->offset);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при чтении конкретного места в архиве!\n");
    free(file_data);
    return result;
  }
  File* output_file = file_create(output_path);
  if (output_file == NULL)
  {
    printf("Произошла ошибка при создании выходного файла!\n");
    free(file_data);
    return RESULT_MEMORY_ERROR;
  }

  result = file_open_for_write(output_file);
  if (result == RESULT_OK)
  {
    result = file_write_bytes(output_file, file_data, entry->original_size);
  }

  free(file_data);
  file_close(output_file);
  file_destroy(output_file);
  return result;
}

Result compressed_archive_reader_extract_all(CompressedArchiveReader* self,
                                             const char* output_path)
{
  if (self == NULL || output_path == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  if (self->header.flags & FLAG_DIRECTORY)
  {
    if (!path_utils_exists(output_path))
    {
      Result result = path_utils_create_directory(output_path);
      if (result != RESULT_OK)
      {
        return result;
      }
    }
  }

  for (DWord i = 0; i < file_table_get_count(self->file_table); i++)
  {
    const FileEntry* entry = file_table_get_entry(self->file_table, i);

    char output_file_path[PATH_LIMIT];
    if (self->header.flags & FLAG_DIRECTORY)
    {
      Size bytes_written = snprintf(output_file_path, sizeof(output_file_path),
                                    "%s/%s", output_path, entry->filename);

      char* parent = path_utils_get_parent(output_file_path);
      if (parent != NULL)
      {
        if (!path_utils_exists(parent))
        {
          Result result = path_utils_create_directory_recursive(parent);
          if (result != RESULT_OK)
          {
            free(parent);
            return result;
          }
        }
        free(parent);
      }

      printf("Извлечение файла: %s -> %s\n", entry->filename, output_file_path);
    }
    else
    {
      strncpy(output_file_path, output_path, sizeof(output_file_path));
    }

    Result result = extract_single_file(self, i, output_file_path);
    if (result != RESULT_OK)
    {
      printf("Ошибка извлечения файла: %s\n", output_file_path);
      return result;
    }
  }

  return RESULT_OK;
}

DWord compressed_archive_reader_get_file_count(
  const CompressedArchiveReader* self)
{
  return self ? file_table_get_count(self->file_table) : 0;
}

const char* compressed_archive_reader_get_filename(
  const CompressedArchiveReader* self, DWord index)
{
  if (self == NULL)
  {
    return NULL;
  }

  const FileEntry* entry = file_table_get_entry(self->file_table, index);
  return entry ? entry->filename : NULL;
}
