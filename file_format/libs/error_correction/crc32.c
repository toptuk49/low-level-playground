#include "crc32.h"

#include <stdio.h>
#include <stdlib.h>

#include "types.h"

#define ALL_POSSIBLE_BYTES 256

struct CRC32Table
{
  DWord cells[ALL_POSSIBLE_BYTES];
  DWord crc32;
};

CRC32Table* crc32_table_create()
{
  CRC32Table* table = (CRC32Table*)malloc(sizeof(CRC32Table));
  if (table == NULL)
  {
    printf("Произошла ошибка при выделении памяти!\n");
    return NULL;
  }

  const DWord polynomial = 0xEDB88320;
  for (DWord byte = 0; byte < ALL_POSSIBLE_BYTES; byte++)
  {
    DWord current_byte = byte;
    const Size bits_in_byte = 8;
    for (Size bit = 0; bit < bits_in_byte; bit++)
    {
      if (current_byte & 1)
      {
        current_byte = polynomial ^ (current_byte >> 1);
      }
      else
      {
        current_byte >>= 1;
      }
    }

    table->cells[byte] = current_byte;
  }

  return table;
}

void crc32_table_destroy(CRC32Table* self)
{
  free(self);
}

Result crc32_table_calculate(CRC32Table* self, const Byte* data, Size size)
{
  if (data == NULL || size == 0)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  const DWord initial_value = 0xFFFFFFFF;
  const DWord least_significant_byte = 0xFF;
  const DWord bits_in_byte = 8;

  DWord crc = initial_value;

  for (Size i = 0; i < size; i++)
  {
    crc = self->cells[(crc ^ data[i]) & least_significant_byte] ^
          (crc >> bits_in_byte);
  }

  self->crc32 = crc ^ initial_value;

  return RESULT_OK;
}

DWord crc32_table_get_crc32(CRC32Table* self)
{
  return self->crc32 ? self->crc32 : 0;
}
