#include "arithmetic.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"

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
    frequencies[data[i]]++;
  }

  model->cumulative[0] = 0;
  for (int i = 0; i < ARITHMETIC_MAX_SYMBOLS; i++)
  {
    model->cumulative[i + 1] = model->cumulative[i] + frequencies[i];
  }
  model->total = model->cumulative[ARITHMETIC_MAX_SYMBOLS];

  if (model->total == 0)
  {
    for (int i = 0; i <= ARITHMETIC_MAX_SYMBOLS; i++)
    {
      model->cumulative[i] = i;
    }
    model->total = ARITHMETIC_MAX_SYMBOLS;
  }

  return RESULT_OK;
}

void arithmetic_model_update(ArithmeticModel* model, Byte symbol)
{
  if (!model)
  {
    return;
  }

  for (int i = symbol + 1; i <= ARITHMETIC_MAX_SYMBOLS; i++)
  {
    model->cumulative[i]++;
  }
  model->total++;

  if (model->total > (1U << 20))
  {
    for (int i = 1; i <= ARITHMETIC_MAX_SYMBOLS; i++)
    {
      model->cumulative[i] = (model->cumulative[i] + 1) / 2;
    }
    model->total = model->cumulative[ARITHMETIC_MAX_SYMBOLS];
  }
}

static void arithmetic_encoder_init(ArithmeticEncoder* encoder)
{
  encoder->low = 0;
  encoder->high = 0xFFFFFFFFU;  // Все 32 бита установлены в 1
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
    *buffer_capacity *= 2;
    Byte* new_buffer = (Byte*)realloc(*output, *buffer_capacity);

    if (new_buffer == NULL)
    {
      return;
    }

    *output = new_buffer;
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
  while (true)
  {
    if (encoder->high < ARITHMETIC_SECOND_BIT)
    {
      arithmetic_encoder_write_bit(encoder, 0, output, output_size,
                                   buffer_position, buffer_capacity);

      for (; encoder->underflow_bits > 0; encoder->underflow_bits--)
      {
        arithmetic_encoder_write_bit(encoder, 1, output, output_size,
                                     buffer_position, buffer_capacity);
      }
    }
    else if (encoder->low >= ARITHMETIC_SECOND_BIT)
    {
      arithmetic_encoder_write_bit(encoder, 1, output, output_size,
                                   buffer_position, buffer_capacity);

      for (; encoder->underflow_bits > 0; encoder->underflow_bits--)
      {
        arithmetic_encoder_write_bit(encoder, 0, output, output_size,
                                     buffer_position, buffer_capacity);
      }

      encoder->low -= ARITHMETIC_SECOND_BIT;
      encoder->high -= ARITHMETIC_SECOND_BIT;
    }
    else if (encoder->low >= ARITHMETIC_TOP_BIT &&
             encoder->high < (3 * ARITHMETIC_TOP_BIT))
    {
      // Старшие биты 01 и 10 (близко к середине)
      encoder->underflow_bits++;
      encoder->low -= ARITHMETIC_TOP_BIT;
      encoder->high -= ARITHMETIC_TOP_BIT;
    }
    else
    {
      break;
    }

    encoder->low <<= 1;
    encoder->high <<= 1;
    encoder->high |= 1;  // Устанавливаем младший бит
  }
}

static void arithmetic_encoder_finish(ArithmeticEncoder* encoder, Byte** output,
                                      Size* output_size, Size* buffer_position,
                                      Size* buffer_capacity)
{
  // Выбираем окончательное значение в диапазоне [low, high]
  // Обычно выбираем low или что-то близкое к low

  encoder->underflow_bits++;

  if (encoder->low < ARITHMETIC_TOP_BIT)
  {
    arithmetic_encoder_write_bit(encoder, 0, output, output_size,
                                 buffer_position, buffer_capacity);
    for (; encoder->underflow_bits > 0; encoder->underflow_bits--)
    {
      arithmetic_encoder_write_bit(encoder, 1, output, output_size,
                                   buffer_position, buffer_capacity);
    }
  }
  else
  {
    arithmetic_encoder_write_bit(encoder, 1, output, output_size,
                                 buffer_position, buffer_capacity);
    for (; encoder->underflow_bits > 0; encoder->underflow_bits--)
    {
      arithmetic_encoder_write_bit(encoder, 0, output, output_size,
                                   buffer_position, buffer_capacity);
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

  printf("[ARITHMETIC] Начало арифметического кодирования\n");
  printf("[ARITHMETIC] Размер входных данных: %zu байт\n", input_size);
  printf("[ARITHMETIC] Общее количество символов в модели: %u\n", model->total);

  ArithmeticEncoder encoder;
  arithmetic_encoder_init(&encoder);

  Size buffer_capacity = (input_size * 2) + 1024;  // Начальная емкость

  *output = malloc(buffer_capacity);
  if (*output == NULL)
  {
    return RESULT_MEMORY_ERROR;
  }

  memset(*output, 0, buffer_capacity);
  Size buffer_position = 0;
  *output_size = 0;

  for (Size i = 0; i < input_size; i++)
  {
    Byte symbol = input[i];

    DWord symbol_low = model->cumulative[symbol];
    DWord symbol_high = model->cumulative[symbol + 1];

    DWord range = encoder.high - encoder.low + 1;

    DWord new_low = encoder.low + (range * symbol_low) / model->total;
    DWord new_high = encoder.low + (range * symbol_high) / model->total - 1;

    encoder.low = new_low;
    encoder.high = new_high;

    arithmetic_encoder_normalize(&encoder, output, output_size,
                                 &buffer_position, &buffer_capacity);

    if (i < 10)
    {
      printf("[ARITHMETIC] Закодирован символ %u (0x%02X) [%u-%u]\n", symbol,
             symbol, symbol_low, symbol_high);
    }
  }

  arithmetic_encoder_finish(&encoder, output, output_size, &buffer_position,
                            &buffer_capacity);

  printf("[ARITHMETIC] Кодирование завершено\n");
  printf("[ARITHMETIC] Размер выходных данных: %zu байт\n", *output_size);
  printf("[ARITHMETIC] Коэффициент сжатия: %.2f%%\n",
         (1.0 - (double)*output_size / (double)input_size) * 100);

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
  while (true)
  {
    if (decoder->high < ARITHMETIC_SECOND_BIT)
    {
      // Оба старших бита 0
      // Ничего не делаем
    }
    else if (decoder->low >= ARITHMETIC_SECOND_BIT)
    {
      // Оба старших бита 1
      decoder->value -= ARITHMETIC_SECOND_BIT;
      decoder->low -= ARITHMETIC_SECOND_BIT;
      decoder->high -= ARITHMETIC_SECOND_BIT;
    }
    else if (decoder->low >= ARITHMETIC_TOP_BIT &&
             decoder->high < (3 * ARITHMETIC_TOP_BIT))
    {
      // Старшие биты 01 и 10
      decoder->value -= ARITHMETIC_TOP_BIT;
      decoder->low -= ARITHMETIC_TOP_BIT;
      decoder->high -= ARITHMETIC_TOP_BIT;
    }
    else
    {
      break;
    }

    // Сдвигаем диапазон и читаем новый бит
    decoder->low <<= 1;
    decoder->high <<= 1;
    decoder->high |= 1;
    decoder->value =
      (decoder->value << 1) | arithmetic_decoder_read_bit(decoder);
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

  printf("[ARITHMETIC] Начало арифметического декодирования\n");
  printf("[ARITHMETIC] Размер входных данных: %zu байт\n", input_size);
  printf("[ARITHMETIC] Ожидаемый выходной размер: %zu байт\n", *output_size);
  printf("[ARITHMETIC] Общее количество символов в модели: %u\n", model->total);

  ArithmeticDecoder decoder;
  arithmetic_decoder_init(&decoder, input, input_size);

  *output = malloc(*output_size);
  if (*output == NULL)
  {
    printf("Произошла ошибка при выделении памяти!\n");
    return RESULT_MEMORY_ERROR;
  }

  for (Size i = 0; i < *output_size; i++)
  {
    DWord range = decoder.high - decoder.low + 1;
    DWord scaled_value =
      ((decoder.value - decoder.low + 1) * model->total - 1) / range;

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

    DWord new_low = decoder.low + (range * symbol_low) / model->total;
    DWord new_high = decoder.low + (range * symbol_high) / model->total - 1;

    decoder.low = new_low;
    decoder.high = new_high;

    arithmetic_decoder_normalize(&decoder);

    if (i < 10)
    {
      printf(
        "[ARITHMETIC] Декодирован символ %u (0x%02X '%c') на позиции %zu\n",
        symbol, symbol, isprint(symbol) ? symbol : '.', i);
    }
  }

  printf("[ARITHMETIC] Декодирование завершено\n");

  printf("[ARITHMETIC] Первые 16 байт декодированных данных: ");
  for (int i = 0; i < 16 && i < *output_size; i++)
  {
    printf("%02X ", (*output)[i]);
  }
  printf("\n");

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
    printf("Произошла ошибка при выделении памяти!\n");
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
    return RESULT_INVALID_ARGUMENT;
  }

  const Byte* pointer = data;

  model->cumulative[0] = 0;
  for (int i = 0; i < 256; i++)
  {
    DWord frequency;
    memcpy(&frequency, pointer, sizeof(DWord));
    pointer += sizeof(DWord);
    model->cumulative[i + 1] = model->cumulative[i] + frequency;
  }

  memcpy(&model->total, pointer, sizeof(DWord));

  return RESULT_OK;
}
