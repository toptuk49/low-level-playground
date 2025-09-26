#include "summary.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "statistics.h"
#include "types.h"

struct Summary
{
  double information_bits;
  double information_bytes;
  long compressed_min_size;
  uint64_t archive_non_standard;
  uint64_t archive_normalized;
};

Summary* summary_create(void)
{
  Summary* summary = (Summary*)malloc(sizeof(Summary));
  if (!summary)
  {
    return NULL;
  }

  summary->information_bits = 0.0;
  summary->information_bytes = 0.0;
  summary->compressed_min_size = 0;
  summary->archive_non_standard = 0;
  summary->archive_normalized = 0;

  return summary;
}

void summary_destroy(Summary* self)
{
  free(self);
}

Result summary_calculate(Summary* self, const Statistics* statistics)
{
  if (!self || !statistics)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  self->information_bits = 0.0;

  for (int i = 0; i < ALPHABET_SIZE; i++)
  {
    long count = statistics_get_byte_count(statistics, i);
    if (count > 0)
    {
      double information = statistics_get_byte_information(statistics, i);
      self->information_bits += (double)(count)*information;
    }
  }

  const Byte byte = 8;
  self->information_bytes = self->information_bits / (double)(byte);
  self->compressed_min_size = (long)ceil(self->information_bytes);
  self->archive_non_standard = self->compressed_min_size + (uint64_t)(256 * 8);
  self->archive_normalized = self->compressed_min_size + (uint64_t)(256 * 1);

  return RESULT_OK;
}

double summary_get_information_bits(const Summary* self)
{
  if (!self)
  {
    return 0.0;
  }
  return self->information_bits;
}

double summary_get_information_bytes(const Summary* self)
{
  if (!self)
  {
    return 0.0;
  }
  return self->information_bytes;
}

long summary_get_compressed_min_size(const Summary* self)
{
  if (!self)
  {
    return 0;
  }
  return self->compressed_min_size;
}

uint64_t summary_get_archive_length_non_standard(const Summary* self)
{
  if (!self)
  {
    return 0;
  }
  return (uint64_t)self->archive_non_standard;
}

uint64_t summary_get_archive_length_normalized(const Summary* self)
{
  if (!self)
  {
    return 0;
  }
  return (uint64_t)self->archive_normalized;
}

void summary_print(const Summary* self, const char* filename)
{
  if (!self || !filename)
  {
    return;
  }

  printf("Файл: %s\n", filename);
  printf("I(Q) = %.2f [бит]\n", self->information_bits);
  printf("I(Q) = %.2f [октетов]\n", self->information_bytes);
  printf("E (нижняя граница) = %ld [октетов]\n", self->compressed_min_size);
  printf("G64 = %" PRIu64 " [октетов]\n", self->archive_non_standard);
  printf("G8 = %" PRIu64 " [октетов]\n", self->archive_normalized);
  printf("Дробная часть I(Q) = %.2e [бит]\n",
         fmod(self->information_bits, 1.0));
}
