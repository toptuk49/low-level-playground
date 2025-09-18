#ifndef UNICODE_UNICODE_H
#define UNICODE_UNICODE_H

#include <inttypes.h>
#include <stdlib.h>

#include "types.h"

#define UNICODE_MAX_UNIQUE_SYMBOLS 65536
#define UNICODE_REPLACEMENT_CHAR 0xFFFDu

typedef struct
{
  uint32_t codepoint;
  uint64_t count;
} Symbol;

typedef struct UnicodeDecoder UnicodeDecoder;
typedef struct UnicodeStatistics UnicodeStatistics;

UnicodeDecoder* unicode_decoder_create(void);
void unicode_decoder_destroy(UnicodeDecoder* self);

Result unicode_decoder_decode(UnicodeDecoder* self, const Byte* data,
                              Size data_size, uint32_t* codepoint,
                              int* symbol_length);

UnicodeStatistics* unicode_statistics_create(void);
void unicode_statistics_destroy(UnicodeStatistics* self);

Result unicode_statistics_add_symbol(UnicodeStatistics* self,
                                     uint32_t codepoint);
Result unicode_statistics_process_data(UnicodeStatistics* self,
                                       const Byte* data, Size data_size);

Size unicode_statistics_get_unique_count(const UnicodeStatistics* self);
const Symbol* unicode_statistics_get_symbols(const UnicodeStatistics* self);
const Symbol* unicode_statistics_find_symbol(const UnicodeStatistics* self,
                                             uint32_t codepoint);

int unicode_get_symbol_length(uint32_t codepoint);
bool unicode_is_valid_utf8(const Byte* data, Size data_size);

#endif  // UNICODE_UNICODE_H
