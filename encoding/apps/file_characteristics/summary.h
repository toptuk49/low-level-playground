#ifndef FILE_CHARACTERISTICS_SUMMARY_H
#define FILE_CHARACTERISTICS_SUMMARY_H

#include "statistics.h"

typedef struct Summary Summary;

Summary* summary_create(void);
void summary_destroy(Summary* self);

Result summary_calculate(Summary* self, const Statistics* statistics);
void summary_print(const Summary* self, const char* filename);

double summary_get_information_bits(const Summary* self);
double summary_get_information_bytes(const Summary* self);
long summary_get_compressed_min_size(const Summary* self);
uint64_t summary_get_archive_length_non_standard(const Summary* self);
uint64_t summary_get_archive_length_normalized(const Summary* self);

#endif  // FILE_CHARACTERISTICS_SUMMARY_H
