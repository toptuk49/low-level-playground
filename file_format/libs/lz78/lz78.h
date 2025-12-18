#ifndef LZ78_LZ78_H
#define LZ78_LZ78_H

#include "types.h"

#define LZ78_DICT_SIZE 4096  // 12-битные коды (как в LZW)
#define LZ78_MAX_CODE_LENGTH 12
#define LZ78_DICT_START 256  // Начинаем с кодов после байтов

typedef struct LZ78DictEntry
{
  Byte* data;
  Size length;
  Size code;
} LZ78DictEntry;

typedef struct LZ78Context
{
  LZ78DictEntry* dictionary;
  Size dict_size;
  Size dict_capacity;
  Size next_code;
  Byte current_phrase[LZ78_DICT_SIZE];
  Size current_length;
} LZ78Context;

LZ78Context* lz78_create(void);
void lz78_destroy(LZ78Context* context);
Result lz78_compress(const Byte* input, Size input_size, Byte** output,
                     Size* output_size);
Result lz78_decompress(const Byte* input, Size input_size, Byte** output,
                       Size* output_size);

void lz78_reset(LZ78Context* context);

#endif  // LZ78_LZ78_H
