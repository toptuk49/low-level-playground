#ifndef ARITHMETIC_ARITHMETIC_H
#define ARITHMETIC_ARITHMETIC_H

#include "types.h"

#define ARITHMETIC_MAX_SYMBOLS 256
#define ARITHMETIC_PRECISION 32           // 32-битная арифметика
#define ARITHMETIC_TOP_BIT (1U << 31)     // Старший бит
#define ARITHMETIC_SECOND_BIT (1U << 30)  // Второй старший бит
#define ARITHMETIC_MASK ((1U << 30) - 1)  // Маска для 30 битов

typedef struct
{
  DWord low;
  DWord high;
  DWord scale;           // Масштабирующий коэффициент
  DWord underflow_bits;  // Биты для отложенного переноса
} ArithmeticEncoder;

typedef struct
{
  DWord value;
  DWord low;
  DWord high;
  const Byte* data;
  Size position;
  Size size;
} ArithmeticDecoder;

typedef struct
{
  DWord cumulative[ARITHMETIC_MAX_SYMBOLS + 1];  // Кумулятивные частоты
  DWord total;                                   // Общее количество символов
} ArithmeticModel;

// Модель и кодировщик/декодировщик
ArithmeticModel* arithmetic_model_create(void);
void arithmetic_model_destroy(ArithmeticModel* model);
Result arithmetic_model_build(ArithmeticModel* model, const Byte* data,
                              Size size);
void arithmetic_model_update(ArithmeticModel* model, Byte symbol);

// Арифметическое кодирование
Result arithmetic_compress(const Byte* input, Size input_size, Byte** output,
                           Size* output_size, const ArithmeticModel* model);

// Арифметическое декодирование
Result arithmetic_decompress(const Byte* input, Size input_size, Byte** output,
                             Size* output_size, const ArithmeticModel* model);

// Сериализация/десериализация модели
Result arithmetic_serialize_model(const ArithmeticModel* model, Byte** data,
                                  Size* size);
Result arithmetic_deserialize_model(ArithmeticModel* model, const Byte* data,
                                    Size size);

#endif  // ARITHMETIC_ARITHMETIC_H
