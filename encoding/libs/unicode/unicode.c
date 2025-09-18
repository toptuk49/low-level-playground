#include "unicode.h"

#include <stdio.h>
#include <stdlib.h>

#include "types.h"

#define UNICODE_FIRST_BYTE_LIMIT 0x7F
#define UNICODE_SECOND_BYTE_LIMIT 0x7FF
#define UNICODE_THIRD_BYTE_LIMIT 0xFFFF

#define UNICODE_TWO_BYTE_MASK 0xE0
#define UNICODE_TWO_BYTE_PATTERN 0xC0
#define UNICODE_THREE_BYTE_MASK 0xF0
#define UNICODE_THREE_BYTE_PATTERN 0xE0
#define UNICODE_FOUR_BYTE_MASK 0xF8
#define UNICODE_FOUR_BYTE_PATTERN 0xF0
#define UNICODE_CONTINUATION_MASK 0xC0
#define UNICODE_CONTINUATION_PATTERN 0x80

#define UNICODE_FIRST_BYTE_DATA_MASK_2 0x1F
#define UNICODE_FIRST_BYTE_DATA_MASK_3 0x0F
#define UNICODE_FIRST_BYTE_DATA_MASK_4 0x07
#define UNICODE_CONTINUATION_DATA_MASK 0x3F

#define UNICODE_SHIFT_SIX_BITS 6
#define UNICODE_SHIFT_TWELVE_BITS 12
#define UNICODE_SHIFT_EIGHTEEN_BITS 18

struct UnicodeStatistics
{
  Symbol symbols[UNICODE_MAX_UNIQUE_SYMBOLS];
  Size unique_count;
  Size total_count;
};

struct UnicodeDecoder
{
  // Можем добавить состояние декодера при необходимости
};

static uint32_t decode_utf8_byte_sequence(const Byte* current_byte,
                                          Size bytes_remaining,
                                          int* symbol_length)
{
  if (bytes_remaining == 0)
  {
    *symbol_length = 0;
    return 0;
  }

  Byte first_byte = current_byte[0];

  if (first_byte <= UNICODE_FIRST_BYTE_LIMIT)
  {
    *symbol_length = 1;
    return first_byte;
  }

  if ((first_byte & UNICODE_TWO_BYTE_MASK) == UNICODE_TWO_BYTE_PATTERN &&
      bytes_remaining >= 2)
  {
    Byte second_byte = current_byte[1];
    if ((second_byte & UNICODE_CONTINUATION_MASK) !=
        UNICODE_CONTINUATION_PATTERN)
    {
      *symbol_length = 1;
      return UNICODE_REPLACEMENT_CHAR;
    }
    *symbol_length = 2;
    return ((first_byte & UNICODE_FIRST_BYTE_DATA_MASK_2)
            << UNICODE_SHIFT_SIX_BITS) |
           (second_byte & UNICODE_CONTINUATION_DATA_MASK);
  }

  if ((first_byte & UNICODE_THREE_BYTE_MASK) == UNICODE_THREE_BYTE_PATTERN &&
      bytes_remaining >= 3)
  {
    Byte second_byte = current_byte[1];
    Byte third_byte = current_byte[2];
    if ((second_byte & UNICODE_CONTINUATION_MASK) !=
          UNICODE_CONTINUATION_PATTERN ||
        (third_byte & UNICODE_CONTINUATION_MASK) !=
          UNICODE_CONTINUATION_PATTERN)
    {
      *symbol_length = 1;
      return UNICODE_REPLACEMENT_CHAR;
    }
    *symbol_length = 3;
    return ((first_byte & UNICODE_FIRST_BYTE_DATA_MASK_3)
            << UNICODE_SHIFT_TWELVE_BITS) |
           ((second_byte & UNICODE_CONTINUATION_DATA_MASK)
            << UNICODE_SHIFT_SIX_BITS) |
           (third_byte & UNICODE_CONTINUATION_DATA_MASK);
  }

  if ((first_byte & UNICODE_FOUR_BYTE_MASK) == UNICODE_FOUR_BYTE_PATTERN &&
      bytes_remaining >= 4)
  {
    Byte second_byte = current_byte[1];
    Byte third_byte = current_byte[2];
    Byte fourth_byte = current_byte[3];
    if ((second_byte & UNICODE_CONTINUATION_MASK) !=
          UNICODE_CONTINUATION_PATTERN ||
        (third_byte & UNICODE_CONTINUATION_MASK) !=
          UNICODE_CONTINUATION_PATTERN ||
        (fourth_byte & UNICODE_CONTINUATION_MASK) !=
          UNICODE_CONTINUATION_PATTERN)
    {
      *symbol_length = 1;
      return UNICODE_REPLACEMENT_CHAR;
    }
    *symbol_length = 4;
    return ((first_byte & UNICODE_FIRST_BYTE_DATA_MASK_4)
            << UNICODE_SHIFT_EIGHTEEN_BITS) |
           ((second_byte & UNICODE_CONTINUATION_DATA_MASK)
            << UNICODE_SHIFT_TWELVE_BITS) |
           ((third_byte & UNICODE_CONTINUATION_DATA_MASK)
            << UNICODE_SHIFT_SIX_BITS) |
           (fourth_byte & UNICODE_CONTINUATION_DATA_MASK);
  }

  *symbol_length = 1;
  return UNICODE_REPLACEMENT_CHAR;
}

UnicodeDecoder* unicode_decoder_create(void)
{
  UnicodeDecoder* decoder = malloc(sizeof(UnicodeDecoder));
  return decoder;
}

void unicode_decoder_destroy(UnicodeDecoder* self)
{
  free(self);
}

Result unicode_decoder_decode(UnicodeDecoder* self, const Byte* data,
                              Size data_size, uint32_t* codepoint,
                              int* symbol_length)
{
  if (!self || !data || !codepoint || !symbol_length)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  *codepoint = decode_utf8_byte_sequence(data, data_size, symbol_length);

  if (*symbol_length == 0)
  {
    return RESULT_ERROR;
  }

  return RESULT_OK;
}

UnicodeStatistics* unicode_statistics_create(void)
{
  UnicodeStatistics* stats = malloc(sizeof(UnicodeStatistics));
  if (!stats)
  {
    return NULL;
  }

  stats->unique_count = 0;
  stats->total_count = 0;
  return stats;
}

void unicode_statistics_destroy(UnicodeStatistics* self)
{
  free(self);
}

Result unicode_statistics_add_symbol(UnicodeStatistics* self,
                                     uint32_t codepoint)
{
  if (!self)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  for (Size i = 0; i < self->unique_count; i++)
  {
    if (self->symbols[i].codepoint == codepoint)
    {
      self->symbols[i].count++;
      self->total_count++;
      return RESULT_OK;
    }
  }

  if (self->unique_count >= UNICODE_MAX_UNIQUE_SYMBOLS)
  {
    return RESULT_ERROR;
  }

  self->symbols[self->unique_count].codepoint = codepoint;
  self->symbols[self->unique_count].count = 1;
  self->unique_count++;
  self->total_count++;

  return RESULT_OK;
}

Result unicode_statistics_process_data(UnicodeStatistics* self,
                                       const Byte* data, Size data_size)
{
  if (!self || !data)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  UnicodeDecoder* decoder = unicode_decoder_create();
  if (!decoder)
  {
    return RESULT_MEMORY_ERROR;
  }

  Size position = 0;
  while (position < data_size)
  {
    uint32_t codepoint;
    int symbol_length;

    Result result =
      unicode_decoder_decode(decoder, data + position, data_size - position,
                             &codepoint, &symbol_length);
    if (result != RESULT_OK)
    {
      unicode_decoder_destroy(decoder);
      return result;
    }

    result = unicode_statistics_add_symbol(self, codepoint);
    if (result != RESULT_OK)
    {
      unicode_decoder_destroy(decoder);
      return result;
    }

    position += symbol_length;
  }

  unicode_decoder_destroy(decoder);
  return RESULT_OK;
}

Size unicode_statistics_get_unique_count(const UnicodeStatistics* self)
{
  return self ? self->unique_count : 0;
}

const Symbol* unicode_statistics_get_symbols(const UnicodeStatistics* self)
{
  return self ? self->symbols : NULL;
}

const Symbol* unicode_statistics_find_symbol(const UnicodeStatistics* self,
                                             uint32_t codepoint)
{
  if (!self)
  {
    return NULL;
  }

  for (Size i = 0; i < self->unique_count; i++)
  {
    if (self->symbols[i].codepoint == codepoint)
    {
      return &self->symbols[i];
    }
  }

  return NULL;
}

int unicode_get_symbol_length(uint32_t codepoint)
{
  if (codepoint <= UNICODE_FIRST_BYTE_LIMIT)
  {
    return 1;
  }
  if (codepoint <= UNICODE_SECOND_BYTE_LIMIT)
  {
    return 2;
  }
  if (codepoint <= UNICODE_THIRD_BYTE_LIMIT)
  {
    return 3;
  }
  return 4;
}

bool unicode_is_valid_utf8(const Byte* data, Size data_size)
{
  UnicodeDecoder* decoder = unicode_decoder_create();
  if (!decoder)
  {
    return false;
  }

  Size position = 0;
  bool valid = true;

  while (position < data_size && valid)
  {
    uint32_t codepoint;
    int symbol_length;

    Result result =
      unicode_decoder_decode(decoder, data + position, data_size - position,
                             &codepoint, &symbol_length);
    if (result != RESULT_OK || codepoint == UNICODE_REPLACEMENT_CHAR)
    {
      valid = false;
    }

    position += symbol_length;
  }

  unicode_decoder_destroy(decoder);
  return valid && (position == data_size);
}
