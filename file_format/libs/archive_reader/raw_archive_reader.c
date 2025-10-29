#include "raw_archive_reader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"
#include "file_table.h"
#include "path_utils.h"
#include "raw_archive_header.h"

#define PATH_LIMIT 4096

struct RawArchiveReader
{
  File* archive_file;
  FileTable* file_table;
  RawArchiveHeader header;
};

RawArchiveReader* raw_archive_reader_create(const char* input_filename)
{
  if (input_filename == NULL)
  {
    return NULL;
  }

  RawArchiveReader* reader =
    (RawArchiveReader*)malloc(sizeof(RawArchiveReader));
  if (reader == NULL)
  {
    printf("Произошла ошибка при выделении памяти!\n");
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

  Result result = file_open_for_read(reader->archive_file);
  if (result != RESULT_OK)
  {
    goto error;
  }

  result = file_read_bytes(reader->archive_file);
  if (result != RESULT_OK)
  {
    goto error;
  }

  const Byte* data = file_get_buffer(reader->archive_file);
  memcpy(&reader->header, data, RAW_ARCHIVE_HEADER_SIZE);

  if (!raw_archive_header_is_valid(&reader->header))
  {
    printf("Неверная сигнатура или версия raw архива!\n");
    result = RESULT_ERROR;
    goto error;
  }

  result = file_seek(reader->archive_file, RAW_ARCHIVE_HEADER_SIZE, SEEK_SET);
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при переходе к таблице файлов!\n");
    goto error;
  }

  result = file_table_read(reader->file_table, reader->archive_file);
  if (result != RESULT_OK)
  {
    goto error;
  }

  return reader;

error:
  file_table_destroy(reader->file_table);
  file_destroy(reader->archive_file);
  free(reader);
  return NULL;
}

void raw_archive_reader_destroy(RawArchiveReader* self)
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

static Result extract_single_file(RawArchiveReader* self, DWord file_index,
                                  const char* output_path)
{
  const FileEntry* entry = file_table_get_entry(self->file_table, file_index);
  if (entry == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  // Используем методы File для чтения данных
  Byte* file_data = (Byte*)malloc(entry->original_size);
  if (file_data == NULL)
  {
    printf("Произошла ошибка при выделении памяти!\n");
    return RESULT_MEMORY_ERROR;
  }

  // Читаем данные файла из архива
  Size data_offset =
    RAW_ARCHIVE_HEADER_SIZE + sizeof(DWord) +
    (file_table_get_count(self->file_table) * sizeof(FileEntry)) +
    entry->offset;

  Result result = file_read_at(self->archive_file, file_data,
                               entry->original_size, entry->offset);

  if (result != RESULT_OK)
  {
    free(file_data);
    return result;
  }

  File* output_file = file_create(output_path);
  if (output_file == NULL)
  {
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

Result raw_archive_reader_extract_all(RawArchiveReader* self,
                                      const char* output_path)
{
  if (self == NULL || output_path == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  if (file_table_get_count(self->file_table) > 1)
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
    if (file_table_get_count(self->file_table) > 1)
    {
      Size bytes_written = snprintf(output_file_path, sizeof(output_file_path),
                                    "%s/%s", output_path, entry->filename);

      char* parent = path_utils_get_parent(output_file_path);
      if (parent != NULL)
      {
        if (!path_utils_exists(parent))
        {
          path_utils_create_directory(parent);
        }
        free(parent);
      }
    }
    else
    {
      strncpy(output_file_path, output_path, sizeof(output_file_path));
    }

    Result result = extract_single_file(self, i, output_file_path);
    if (result != RESULT_OK)
    {
      return result;
    }
  }

  return RESULT_OK;
}

DWord raw_archive_reader_get_file_count(const RawArchiveReader* self)
{
  return self ? file_table_get_count(self->file_table) : 0;
}

const char* raw_archive_reader_get_filename(const RawArchiveReader* self,
                                            DWord index)
{
  if (self == NULL)
  {
    return NULL;
  }

  const FileEntry* entry = file_table_get_entry(self->file_table, index);
  return entry ? entry->filename : NULL;
}
