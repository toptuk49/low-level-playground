#ifndef LZ77_LZ77_H
#define LZ77_LZ77_H

#include "types.h"

#define LZ77_WINDOW_SIZE 1024  // Размер окна (S может быть до 1023)
#define LZ77_LOOKAHEAD_SIZE \
  66                       // Размер буфера предпросмотра (L может быть до 65)
#define LZ77_MIN_MATCH 3   // Минимальная длина совпадения
#define LZ77_MAX_MATCH 65  // Максимальная длина совпадения (3 + 63)

typedef struct LZ77Context
{
  Byte prefix;      // Префикс для кодирования
  Byte* window;     // Плавающее окно
  Size window_pos;  // Текущая позиция в окне
} LZ77Context;

LZ77Context* lz77_create(Byte prefix);
void lz77_destroy(LZ77Context* context);

Result lz77_compress(const Byte* input, Size input_size, Byte** output,
                     Size* output_size, Byte prefix);
Result lz77_decompress(const Byte* input, Size input_size, Byte** output,
                       Size* output_size, Byte prefix);

Byte lz77_analyze_prefix(const Byte* data, Size size);

#endif  // LZ77_LZ77_H
