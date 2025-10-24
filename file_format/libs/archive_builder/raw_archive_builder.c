#include "raw_archive_builder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "file.h"
#include "file_table.h"
#include "raw_archive_header.h"

struct RawArchiveBuilder
{
  File* archive_file;
  FileTable* file_table;
};

RawArchiveBuilder* raw_archive_builder_create(const char* output_filename)
{
  if (output_filename == NULL)
  {
    return NULL;
  }

  RawArchiveBuilder* builder =
    (RawArchiveBuilder*)malloc(sizeof(RawArchiveBuilder));
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

void raw_archive_builder_destroy(RawArchiveBuilder* self)
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

Result raw_archive_builder_add_file(RawArchiveBuilder* self,
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

  return file_table_add_file(self->file_table, filename, stats.st_size);
}

Result raw_archive_builder_add_directory(RawArchiveBuilder* self,
                                         const char* dirname)
{
  if (self == NULL || dirname == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  return file_table_add_directory(self->file_table, dirname);
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

Result raw_archive_builder_finalize(RawArchiveBuilder* self)
{
  if (self == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  RawArchiveHeader header;
  raw_archive_header_init(&header, file_table_get_total_size(self->file_table));

  Result result = raw_archive_header_write(&header, self->archive_file);
  if (result != RESULT_OK)
  {
    return result;
  }

  result = file_table_write(self->file_table, self->archive_file);
  if (result != RESULT_OK)
  {
    return result;
  }

  for (DWord i = 0; i < file_table_get_count(self->file_table); i++)
  {
    const FileEntry* entry = file_table_get_entry(self->file_table, i);
    result = write_file_data(self->archive_file, entry->filename);
    if (result != RESULT_OK)
    {
      return result;
    }
  }

  return RESULT_OK;
}
