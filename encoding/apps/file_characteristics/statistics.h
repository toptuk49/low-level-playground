#ifndef FILE_CHARACTERISTICS_STATISTICS_H
#define FILE_CHARACTERISTICS_STATISTICS_H

#include "types.h"

#define ALPHABET_SIZE 256

typedef struct Statistics Statistics;

Statistics* statistics_create(void);
Statistics* statistics_copy(const Statistics* src);
void statistics_destroy(Statistics* self);

Result statistics_calculate(Statistics* self, const Byte* data, Size data_size);
void statistics_print_ordered(const Statistics* self);

unsigned long statistics_get_byte_length(const Statistics* self);
long statistics_get_byte_count(const Statistics* self, Byte byte);
double statistics_get_byte_probability(const Statistics* self, Byte byte);
double statistics_get_byte_information(const Statistics* self, Byte byte);
void statistics_get_sorted_indices(const Statistics* self, int* indices_out);

#endif  // FILE_CHARACTERISTICS_STATISTICS_H
