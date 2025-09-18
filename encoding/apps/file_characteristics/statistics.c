#include "statistics.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "types.h"

void statistics_reset(Statistics* self);

struct Statistics
{
  long byte_counts[ALPHABET_SIZE];
  unsigned long byte_length;
  double byte_probabilities[ALPHABET_SIZE];
  double byte_information[ALPHABET_SIZE];
};

Statistics* statistics_create(void)
{
  Statistics* stats = malloc(sizeof(Statistics));
  if (!stats)
  {
    return NULL;
  }

  statistics_reset(stats);
  return stats;
}

void statistics_destroy(Statistics* self)
{
  free(self);
}

void statistics_reset(Statistics* self)
{
  for (int i = 0; i < ALPHABET_SIZE; i++)
  {
    self->byte_counts[i] = 0;
    self->byte_probabilities[i] = 0.0;
    self->byte_information[i] = 0.0;
  }
  self->byte_length = 0;
}

Result statistics_calculate(Statistics* self, const Byte* data, Size data_size)
{
  if (!self || !data)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  statistics_reset(self);
  self->byte_length = data_size;

  for (Size i = 0; i < data_size; i++)
  {
    self->byte_counts[data[i]]++;
  }

  if (self->byte_length == 0)
  {
    return RESULT_ERROR;
  }

  for (int i = 0; i < ALPHABET_SIZE; i++)
  {
    if (self->byte_counts[i] > 0)
    {
      self->byte_probabilities[i] =
        (double)(self->byte_counts[i]) / (double)(self->byte_length);
      self->byte_information[i] = -log2(self->byte_probabilities[i]);
    }
  }

  return RESULT_OK;
}

unsigned long statistics_get_byte_length(const Statistics* self)
{
  return self ? self->byte_length : 0;
}

long statistics_get_byte_count(const Statistics* self, Byte byte)
{
  return self ? self->byte_counts[byte] : 0;
}

double statistics_get_byte_probability(const Statistics* self, Byte byte)
{
  return self ? self->byte_probabilities[byte] : 0.0;
}

double statistics_get_byte_information(const Statistics* self, Byte byte)
{
  return self ? self->byte_information[byte] : 0.0;
}
