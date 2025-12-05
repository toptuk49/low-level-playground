#include "arithmetic.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"

#define ARITHMETIC_FULL_RANGE 0x100000000ULL    // 2^32
#define ARITHMETIC_HALF_RANGE 0x80000000ULL     // 2^31
#define ARITHMETIC_QUARTER_RANGE 0x40000000ULL  // 2^30
#define ARITHMETIC_MAX_TOTAL 0x10000000UL  // Максимальная сумма частот (2^28)

ArithmeticModel* arithmetic_model_create(void)
{
  ArithmeticModel* model = malloc(sizeof(ArithmeticModel));
  if (model == NULL)
  {
    return NULL;
  }

  for (int i = 0; i <= ARITHMETIC_MAX_SYMBOLS; i++)
  {
    model->cumulative[i] = i;
  }

  model->total = ARITHMETIC_MAX_SYMBOLS;

  return model;
}

void arithmetic_model_destroy(ArithmeticModel* model)
{
  if (model)
  {
    free(model);
  }
}

Result arithmetic_model_build(ArithmeticModel* model, const Byte* data,
                              Size size)
{
  if (!model || !data || size == 0)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  DWord frequencies[ARITHMETIC_MAX_SYMBOLS] = {0};

  for (Size i = 0; i < size; i++)
  {
    if (data[i] < (Byte)ARITHMETIC_MAX_SYMBOLS)
    {
      frequencies[data[i]]++;
    }
  }

  model->cumulative[0] = 0;
  for (int i = 0; i < ARITHMETIC_MAX_SYMBOLS; i++)
  {
    DWord freq = frequencies[i];
    if (freq == 0)
    {
      freq = 1;  // Минимальная частота для всех символов
    }
    model->cumulative[i + 1] = model->cumulative[i] + freq;
  }

  model->total = model->cumulative[ARITHMETIC_MAX_SYMBOLS];

  if (model->total > ARITHMETIC_MAX_TOTAL)
  {
    for (int i = 0; i <= ARITHMETIC_MAX_SYMBOLS; i++)
    {
      model->cumulative[i] = (model->cumulative[i] + 1) >> 1;
    }
    model->total = model->cumulative[ARITHMETIC_MAX_SYMBOLS];
  }

  printf("[ARITHMETIC] Модель построена: total=%u (масштаб 1:%u)\n",
         model->total, model->total / ARITHMETIC_MAX_SYMBOLS);

  return RESULT_OK;
}

void arithmetic_model_update(ArithmeticModel* model, Byte symbol)
{
  // Не используется в этой реализации
  (void)model;
  (void)symbol;
}

static void arithmetic_encoder_init(ArithmeticEncoder* encoder)
{
  encoder->low = 0;
  encoder->high = 0xFFFFFFFFU;
  encoder->scale = 0;
  encoder->underflow_bits = 0;
}

static void arithmetic_encoder_write_bit(ArithmeticEncoder* encoder, int bit,
                                         Byte** output, Size* output_size,
                                         Size* buffer_position,
                                         Size* buffer_capacity)
{
  if (*buffer_position >= *buffer_capacity * 8)
  {
    *buffer_capacity = (*buffer_capacity == 0) ? 1024 : *buffer_capacity * 2;
    Byte* new_buffer = (Byte*)realloc(*output, *buffer_capacity);

    if (new_buffer == NULL)
    {
      return;
    }

    *output = new_buffer;
    if (*buffer_capacity > 0)
    {
      memset(*output + (*buffer_capacity / 2), 0, *buffer_capacity / 2);
    }
  }

  Size byte_index = *buffer_position / 8;
  Size bit_index = 7 - (*buffer_position % 8);

  if (bit)
  {
    (*output)[byte_index] |= (1 << bit_index);
  }

  (*buffer_position)++;

  *output_size = (*buffer_position + 7) / 8;
}

static void arithmetic_encoder_normalize(ArithmeticEncoder* encoder,
                                         Byte** output, Size* output_size,
                                         Size* buffer_position,
                                         Size* buffer_capacity)
{
  while ((encoder->high - encoder->low) < ARITHMETIC_QUARTER_RANGE)
  {
    if (encoder->high < ARITHMETIC_HALF_RANGE)
    {
      arithmetic_encoder_write_bit(encoder, 0, output, output_size,
                                   buffer_position, buffer_capacity);

      while (encoder->underflow_bits > 0)
      {
        arithmetic_encoder_write_bit(encoder, 1, output, output_size,
                                     buffer_position, buffer_capacity);
        encoder->underflow_bits--;
      }
    }
    else if (encoder->low >= ARITHMETIC_HALF_RANGE)
    {
      arithmetic_encoder_write_bit(encoder, 1, output, output_size,
                                   buffer_position, buffer_capacity);

      while (encoder->underflow_bits > 0)
      {
        arithmetic_encoder_write_bit(encoder, 0, output, output_size,
                                     buffer_position, buffer_capacity);
        encoder->underflow_bits--;
      }

      encoder->low -= ARITHMETIC_HALF_RANGE;
      encoder->high -= ARITHMETIC_HALF_RANGE;
    }
    else if (encoder->low >= ARITHMETIC_QUARTER_RANGE &&
             encoder->high < (ARITHMETIC_HALF_RANGE | ARITHMETIC_QUARTER_RANGE))
    {
      encoder->underflow_bits++;
      encoder->low -= ARITHMETIC_QUARTER_RANGE;
      encoder->high -= ARITHMETIC_QUARTER_RANGE;
    }
    else
    {
      break;
    }

    encoder->low <<= 1;
    encoder->high <<= 1;
    encoder->high |= 1;
  }
}

static void arithmetic_encoder_finish(ArithmeticEncoder* encoder, Byte** output,
                                      Size* output_size, Size* buffer_position,
                                      Size* buffer_capacity)
{
  encoder->underflow_bits++;

  if (encoder->low < ARITHMETIC_QUARTER_RANGE)
  {
    arithmetic_encoder_write_bit(encoder, 0, output, output_size,
                                 buffer_position, buffer_capacity);
    while (encoder->underflow_bits > 0)
    {
      arithmetic_encoder_write_bit(encoder, 1, output, output_size,
                                   buffer_position, buffer_capacity);
      encoder->underflow_bits--;
    }
  }
  else
  {
    arithmetic_encoder_write_bit(encoder, 1, output, output_size,
                                 buffer_position, buffer_capacity);
    while (encoder->underflow_bits > 0)
    {
      arithmetic_encoder_write_bit(encoder, 0, output, output_size,
                                   buffer_position, buffer_capacity);
      encoder->underflow_bits--;
    }
  }

  while (*buffer_position % 8 != 0)
  {
    arithmetic_encoder_write_bit(encoder, 0, output, output_size,
                                 buffer_position, buffer_capacity);
  }
}

Result arithmetic_compress(const Byte* input, Size input_size, Byte** output,
                           Size* output_size, const ArithmeticModel* model)
{
  if (!input || !output || !output_size || !model || input_size == 0)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  if (model->total == 0)
  {
    printf("[ARITHMETIC] Ошибка: модель имеет нулевую сумму частот\n");
    return RESULT_INVALID_ARGUMENT;
  }

  printf("[ARITHMETIC] Начало арифметического кодирования\n");
  printf("[ARITHMETIC] Размер входных данных: %zu байт\n", input_size);
  printf("[ARITHMETIC] Общее количество символов в модели: %u\n", model->total);

  ArithmeticEncoder encoder;
  arithmetic_encoder_init(&encoder);

  Size buffer_capacity = 1024;
  *output = malloc(buffer_capacity);
  if (*output == NULL)
  {
    printf("[ARITHMETIC] Ошибка: не удалось выделить память\n");
    return RESULT_MEMORY_ERROR;
  }

  memset(*output, 0, buffer_capacity);
  Size buffer_position = 0;
  *output_size = 0;

  for (Size i = 0; i < input_size; i++)
  {
    Byte symbol = input[i];

    if (symbol >= (Byte)ARITHMETIC_MAX_SYMBOLS)
    {
      printf("[ARITHMETIC] Ошибка: некорректный символ %u\n", symbol);
      free(*output);
      *output = NULL;
      return RESULT_INVALID_ARGUMENT;
    }

    DWord symbol_low = model->cumulative[symbol];
    DWord symbol_high = model->cumulative[symbol + 1];

    QWord range = (QWord)(encoder.high - encoder.low) + 1;

    QWord new_low = encoder.low + (range * symbol_low) / model->total;
    QWord new_high = encoder.low + (range * symbol_high) / model->total - 1;

    encoder.low = (DWord)new_low;
    encoder.high = (DWord)new_high;

    arithmetic_encoder_normalize(&encoder, output, output_size,
                                 &buffer_position, &buffer_capacity);
  }

  arithmetic_encoder_finish(&encoder, output, output_size, &buffer_position,
                            &buffer_capacity);

  if (*output_size > 0)
  {
    Byte* trimmed_output = (Byte*)realloc(*output, *output_size);
    if (trimmed_output != NULL)
    {
      *output = trimmed_output;
    }
  }

  printf("[ARITHMETIC] Кодирование завершено\n");
  printf("[ARITHMETIC] Размер выходных данных: %zu байт\n", *output_size);
  if (input_size > 0)
  {
    double ratio = (1.0 - (double)*output_size / (double)input_size) * 100;
    printf("[ARITHMETIC] Коэффициент сжатия: %.2f%%\n", ratio);
  }

  return RESULT_OK;
}

static void arithmetic_decoder_init(ArithmeticDecoder* decoder,
                                    const Byte* data, Size size)
{
  decoder->value = 0;
  decoder->low = 0;
  decoder->high = 0xFFFFFFFFU;
  decoder->data = data;
  decoder->position = 0;
  decoder->size = size;

  for (int i = 0; i < 32; i++)
  {
    if (decoder->position < size * 8)
    {
      Size byte_index = decoder->position / 8;
      Size bit_index = 7 - (decoder->position % 8);
      int bit = (data[byte_index] >> bit_index) & 1;
      decoder->value = (decoder->value << 1) | bit;
      decoder->position++;
    }
    else
    {
      decoder->value <<= 1;
    }
  }
}

static int arithmetic_decoder_read_bit(ArithmeticDecoder* decoder)
{
  if (decoder->position >= decoder->size * 8)
  {
    return 0;
  }

  Size byte_index = decoder->position / 8;
  Size bit_index = 7 - (decoder->position % 8);
  int bit = (decoder->data[byte_index] >> bit_index) & 1;
  decoder->position++;

  return bit;
}

static void arithmetic_decoder_normalize(ArithmeticDecoder* decoder)
{
  while ((decoder->high - decoder->low) < ARITHMETIC_QUARTER_RANGE)
  {
    if (decoder->high < ARITHMETIC_HALF_RANGE)
    {
      // Оба старших бита 0 - ничего не делаем с value
    }
    else if (decoder->low >= ARITHMETIC_HALF_RANGE)
    {
      // Оба старших бита 1
      decoder->value -= ARITHMETIC_HALF_RANGE;
      decoder->low -= ARITHMETIC_HALF_RANGE;
      decoder->high -= ARITHMETIC_HALF_RANGE;
    }
    else if (decoder->low >= ARITHMETIC_QUARTER_RANGE &&
             decoder->high < (ARITHMETIC_HALF_RANGE | ARITHMETIC_QUARTER_RANGE))
    {
      // Старшие биты 01 и 10
      decoder->value -= ARITHMETIC_QUARTER_RANGE;
      decoder->low -= ARITHMETIC_QUARTER_RANGE;
      decoder->high -= ARITHMETIC_QUARTER_RANGE;
    }
    else
    {
      break;
    }

    // Сдвигаем диапазон и читаем новый бит
    decoder->low <<= 1;
    decoder->high <<= 1;
    decoder->high |= 1;

    int bit = arithmetic_decoder_read_bit(decoder);
    decoder->value = (decoder->value << 1) | bit;
  }
}

Result arithmetic_decompress(const Byte* input, Size input_size, Byte** output,
                             Size* output_size, const ArithmeticModel* model)
{
  if (!input || !output || !output_size || !model || input_size == 0)
  {
    printf("[ARITHMETIC] Ошибка: неверные параметры в arithmetic_decompress\n");
    return RESULT_INVALID_ARGUMENT;
  }

  if (model->total == 0)
  {
    printf("[ARITHMETIC] Ошибка: модель имеет нулевую сумму частот\n");
    return RESULT_INVALID_ARGUMENT;
  }

  printf("[ARITHMETIC] Начало арифметического декодирования\n");
  printf("[ARITHMETIC] Размер входных данных: %zu байт\n", input_size);
  printf("[ARITHMETIC] Ожидаемый выходной размер: %zu байт\n", *output_size);
  printf("[ARITHMETIC] Общее количество символов в модели: %u\n", model->total);

  ArithmeticDecoder decoder;
  arithmetic_decoder_init(&decoder, input, input_size);

  *output = malloc(*output_size);
  if (*output == NULL)
  {
    printf(
      "[ARITHMETIC] Ошибка: не удалось выделить память для выходных данных\n");
    return RESULT_MEMORY_ERROR;
  }

  memset(*output, 0, *output_size);

  for (Size i = 0; i < *output_size; i++)
  {
    QWord range = (QWord)(decoder.high - decoder.low) + 1;

    QWord temp = (QWord)(decoder.value - decoder.low + 1) * model->total - 1;
    QWord scaled_value = temp / range;

    Byte symbol = 0;
    for (int current_symbol = 0; current_symbol < ARITHMETIC_MAX_SYMBOLS;
         current_symbol++)
    {
      if (scaled_value < model->cumulative[current_symbol + 1])
      {
        symbol = (Byte)current_symbol;
        break;
      }
    }

    (*output)[i] = symbol;

    DWord symbol_low = model->cumulative[symbol];
    DWord symbol_high = model->cumulative[symbol + 1];

    QWord new_low = decoder.low + (range * symbol_low) / model->total;
    QWord new_high = decoder.low + (range * symbol_high) / model->total - 1;

    decoder.low = (DWord)new_low;
    decoder.high = (DWord)new_high;

    arithmetic_decoder_normalize(&decoder);
  }

  printf("[ARITHMETIC] Декодирование завершено\n");

  return RESULT_OK;
}

Result arithmetic_serialize_model(const ArithmeticModel* model, Byte** data,
                                  Size* size)
{
  if (!model || !data || !size)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  // Размер: 256 частот по 4 байта + 4 байта для total
  *size = 256 * sizeof(DWord) + sizeof(DWord);
  *data = malloc(*size);
  if (*data == NULL)
  {
    printf(
      "[ARITHMETIC] Ошибка: не удалось выделить память для сериализации "
      "модели\n");
    return RESULT_MEMORY_ERROR;
  }

  Byte* pointer = *data;

  for (int i = 0; i < 256; i++)
  {
    DWord freq = model->cumulative[i + 1] - model->cumulative[i];
    memcpy(pointer, &freq, sizeof(DWord));
    pointer += sizeof(DWord);
  }

  memcpy(pointer, &model->total, sizeof(DWord));

  return RESULT_OK;
}

Result arithmetic_deserialize_model(ArithmeticModel* model, const Byte* data,
                                    Size size)
{
  if (!model || !data || size != (256 * sizeof(DWord) + sizeof(DWord)))
  {
    printf("[ARITHMETIC] Ошибка: неверный размер данных модели\n");
    return RESULT_INVALID_ARGUMENT;
  }

  const Byte* pointer = data;

  model->cumulative[0] = 0;
  for (int i = 0; i < 256; i++)
  {
    DWord frequency;
    memcpy(&frequency, pointer, sizeof(DWord));
    pointer += sizeof(DWord);

    if (frequency == 0)
    {
      frequency = 1;
    }

    model->cumulative[i + 1] = model->cumulative[i] + frequency;
  }

  memcpy(&model->total, pointer, sizeof(DWord));

  return RESULT_OK;
}
