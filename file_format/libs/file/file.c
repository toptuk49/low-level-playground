#include "file.h"

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
  if (!file)
  {
    return NULL;
  }

  file->path = strdup(path);
  file->descriptor = NULL;
  file->buffer = NULL;
  file->size = 0;

  return file;
}

void file_destroy(File* self)
{
  if (!self)
  {
    return;
  }

  if (self->descriptor)
  {
    int close_status = fclose(self->descriptor);
    if (close_status != 0)
    {
      printf("Произошла ошибка при закрытии файла!\n");
    }
  }
  free(self->buffer);
  free(self->path);
  free(self);
}

Result file_open(File* self)
{
  if (!self)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  self->descriptor = fopen(self->path, "rb");
  if (!self->descriptor)
  {
    return RESULT_IO_ERROR;
  }

  return RESULT_OK;
}

Result file_close(File* self)
{
  if (!self || !self->descriptor)
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
  if (!self || !self->descriptor)
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
