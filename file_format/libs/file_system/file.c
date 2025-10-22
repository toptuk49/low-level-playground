#include "file.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"

#define BYTES_AMOUNT 1

struct File
{
  FILE* descriptor;
  Byte* buffer;
  Size size;
  char* path;
};

File* file_create(const char* path)
{
  File* file = (File*)malloc(sizeof(File));
  if (file == NULL)
  {
    return NULL;
  }

  file->path = (char*)malloc(strlen(path) + 1);
  if (file->path == NULL)
  {
    printf("Произошла ошибка при выделении памяти!\n");
    free(file);
    return NULL;
  }
  strcpy(file->path, path);

  file->descriptor = NULL;
  file->buffer = NULL;
  file->size = 0;

  return file;
}

void file_destroy(File* self)
{
  if (self == NULL)
  {
    return;
  }

  if (self->descriptor)
  {
    int close_status = fclose(self->descriptor);
  }

  free(self->buffer);
  free(self->path);
  free(self);
}

Result file_open_for_read(File* self)
{
  if (self == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  self->descriptor = fopen(self->path, "rb");
  if (self->descriptor == NULL)
  {
    return RESULT_IO_ERROR;
  }

  return RESULT_OK;
}

Result file_close(File* self)
{
  if (self == NULL || self->descriptor == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  int close_status = fclose(self->descriptor);
  if (close_status != 0)
  {
    printf("Произошла ошибка при закрытии файла!\n");
    return RESULT_ERROR;
  }
  self->descriptor = NULL;
  return RESULT_OK;
}

Result file_read_bytes(File* self)
{
  if (self == NULL || self->descriptor == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  int seek_status = fseek(self->descriptor, 0, SEEK_END);
  if (seek_status != 0)
  {
    printf("Произошла ошибка при перемещении указателя файла в конец!\n");
    return RESULT_ERROR;
  }
  self->size = ftell(self->descriptor);
  int rewind_status = fseek(self->descriptor, 0L, SEEK_SET);
  if (rewind_status != 0)
  {
    printf("Произошла ошибка при возвращении указателя файла в начало!\n");
    return RESULT_ERROR;
  }

  self->buffer = (Byte*)malloc(self->size * sizeof(Byte));
  if (!self->buffer)
  {
    return RESULT_MEMORY_ERROR;
  }

  size_t bytes_read =
    fread(self->buffer, BYTES_AMOUNT, self->size, self->descriptor);
  if (bytes_read != self->size)
  {
    free(self->buffer);
    self->buffer = NULL;
    return RESULT_IO_ERROR;
  }

  return RESULT_OK;
}

Result file_read_bytes_size(File* self, Byte* buffer, Size size_to_read)
{
  if (self == NULL || self->descriptor == NULL || buffer == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  size_t bytes_read = fread(buffer, 1, size_to_read, self->descriptor);
  if (bytes_read != size_to_read)
  {
    return RESULT_IO_ERROR;
  }

  return RESULT_OK;
}

Result file_open_for_write(File* self)
{
  if (self == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  self->descriptor = fopen(self->path, "wb");
  if (self->descriptor == NULL)
  {
    return RESULT_IO_ERROR;
  }

  return RESULT_OK;
}

Result file_write_bytes(File* self, const Byte* data, Size data_size)
{
  if (self == NULL || self->descriptor == NULL || data == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  size_t bytes_written = fwrite(data, 1, data_size, self->descriptor);
  if (bytes_written != data_size)
  {
    return RESULT_IO_ERROR;
  }

  return RESULT_OK;
}

Result file_write_from_file(File* self, const File* source)
{
  if (self == NULL || self->descriptor == NULL || source == NULL ||
      source->buffer == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  size_t bytes_written =
    fwrite(source->buffer, 1, source->size, self->descriptor);
  if (bytes_written != source->size)
  {
    return RESULT_IO_ERROR;
  }

  return RESULT_OK;
}

Result file_seek(File* self, long offset, int whence)
{
  if (self == NULL || self->descriptor == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  if (fseek(self->descriptor, offset, whence) != 0)
  {
    return RESULT_IO_ERROR;
  }

  return RESULT_OK;
}

long file_tell(File* self)
{
  if (self == NULL || self->descriptor == NULL)
  {
    return -1;
  }

  return ftell(self->descriptor);
}

Result file_read_at(File* self, Byte* buffer, Size size, uint64_t offset)
{
  if (self == NULL || self->descriptor == NULL || buffer == NULL)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  long current_pos = ftell(self->descriptor);
  if (current_pos == -1)
  {
    return RESULT_IO_ERROR;
  }

  if (fseek(self->descriptor, (long)offset, SEEK_SET) != 0)
  {
    return RESULT_IO_ERROR;
  }

  size_t bytes_read = fread(buffer, 1, size, self->descriptor);

  if (fseek(self->descriptor, current_pos, SEEK_SET) != 0)
  {
    return RESULT_IO_ERROR;
  }

  if (bytes_read != size)
  {
    return RESULT_IO_ERROR;
  }

  return RESULT_OK;
}

const Byte* file_get_buffer(const File* self)
{
  return self ? self->buffer : NULL;
}

Size file_get_size(const File* self)
{
  return self ? self->size : 0;
}

const char* file_get_path(const File* self)
{
  return self ? self->path : NULL;
}
