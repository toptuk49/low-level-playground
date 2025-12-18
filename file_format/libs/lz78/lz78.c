#include "lz78.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"

#define BYTES_IN_CODE \
  2  // 12 бит = 2 байта (один байт для длины кода, один для данных)
#define CODE_MASK 0xFFF  // 12-битная маска

static void write_bits(Byte* buffer, Size* bit_pos, Size code, Size bits);
static Size read_bits(const Byte* buffer, Size* bit_pos, Size bits);
static Size bits_to_bytes(Size bits);

LZ78Context* lz78_create(void)
{
  LZ78Context* context = (LZ78Context*)malloc(sizeof(LZ78Context));
  if (!context)
  {
    printf("[LZ78] Ошибка выделения памяти для контекста\n");
    return NULL;
  }

  context->dict_capacity = LZ78_DICT_SIZE;
  context->dictionary =
    (LZ78DictEntry*)malloc(sizeof(LZ78DictEntry) * context->dict_capacity);
  if (!context->dictionary)
  {
    printf("[LZ78] Ошибка выделения памяти для словаря\n");
    free(context);
    return NULL;
  }

  // Инициализация словаря
  for (Size i = 0; i < 256; i++)
  {
    context->dictionary[i].data = (Byte*)malloc(1);
    if (context->dictionary[i].data)
    {
      context->dictionary[i].data[0] = (Byte)i;
      context->dictionary[i].length = 1;
      context->dictionary[i].code = i;
    }
  }

  for (Size i = 256; i < context->dict_capacity; i++)
  {
    context->dictionary[i].data = NULL;
    context->dictionary[i].length = 0;
    context->dictionary[i].code = 0;
  }

  context->dict_size = 256;  // Начальный словарь содержит все байты
  context->next_code = 256;  // Следующий код для добавления
  context->current_length = 0;

  printf("[LZ78] Создан контекст LZ78 с размером словаря %zu\n",
         context->dict_capacity);
  return context;
}

void lz78_destroy(LZ78Context* context)
{
  if (!context)
  {
    return;
  }

  if (context->dictionary)
  {
    for (Size i = 0; i < context->dict_capacity; i++)
    {
      if (context->dictionary[i].data)
      {
        free(context->dictionary[i].data);
      }
    }
    free(context->dictionary);
  }

  free(context);
}

void lz78_reset(LZ78Context* context)
{
  if (!context)
  {
    return;
  }

  for (Size i = 256; i < context->dict_size; i++)
  {
    if (context->dictionary[i].data)
    {
      free(context->dictionary[i].data);
      context->dictionary[i].data = NULL;
      context->dictionary[i].length = 0;
    }
  }

  context->dict_size = 256;
  context->next_code = 256;
  context->current_length = 0;
}

static Size find_in_dictionary(LZ78Context* context, const Byte* data,
                               Size length)
{
  // Ищем фразу в словаре
  for (Size i = 0; i < context->dict_size; i++)
  {
    if (context->dictionary[i].length == length)
    {
      if (memcmp(context->dictionary[i].data, data, length) == 0)
      {
        return i;  // Нашли существующую фразу
      }
    }
  }
  return LZ78_DICT_SIZE;  // Не найдено
}

static Result add_to_dictionary(LZ78Context* context, const Byte* data,
                                Size length)
{
  if (context->next_code >= context->dict_capacity)
  {
    printf("[LZ78] Словарь полон, сбрасываем\n");
    lz78_reset(context);
  }

  if (context->next_code >= context->dict_capacity)
  {
    return RESULT_ERROR;
  }

  Byte* phrase = (Byte*)malloc(length);
  if (!phrase)
  {
    printf("[LZ78] Ошибка выделения памяти для новой фразы\n");
    return RESULT_MEMORY_ERROR;
  }

  memcpy(phrase, data, length);

  context->dictionary[context->next_code].data = phrase;
  context->dictionary[context->next_code].length = length;
  context->dictionary[context->next_code].code = context->next_code;

  context->dict_size++;
  context->next_code++;

  return RESULT_OK;
}

static void write_bits(Byte* buffer, Size* bit_pos, Size code, Size bits)
{
  for (Size i = 0; i < bits; i++)
  {
    Size byte_index = *bit_pos / 8;
    Size bit_index = 7 - (*bit_pos % 8);

    if (code & (1 << (bits - i - 1)))
    {
      buffer[byte_index] |= (1 << bit_index);
    }
    else
    {
      buffer[byte_index] &= ~(1 << bit_index);
    }

    (*bit_pos)++;
  }
}

static Size read_bits(const Byte* buffer, Size* bit_pos, Size bits)
{
  Size code = 0;
  for (Size i = 0; i < bits; i++)
  {
    Size byte_index = *bit_pos / 8;
    Size bit_index = 7 - (*bit_pos % 8);

    if (buffer[byte_index] & (1 << bit_index))
    {
      code |= (1 << (bits - i - 1));
    }

    (*bit_pos)++;
  }
  return code;
}

static Size bits_to_bytes(Size bits)
{
  return (bits + 7) / 8;
}

Result lz78_compress(const Byte* input, Size input_size, Byte** output,
                     Size* output_size)
{
  if (!input || !output || !output_size || input_size == 0)
  {
    printf("[LZ78] Ошибка: неверные параметры в lz78_compress\n");
    return RESULT_INVALID_ARGUMENT;
  }

  printf("[LZ78] Начало сжатия LZ78, размер данных: %zu байт\n", input_size);

  // Создаем контекст
  LZ78Context* context = lz78_create();
  if (!context)
  {
    return RESULT_MEMORY_ERROR;
  }

  // Максимальный размер выходных данных: каждый символ может стать парой (код +
  // байт) Формат: 12-битный код + 8-битный символ
  Size max_output_bits = input_size * (LZ78_MAX_CODE_LENGTH + 8);
  Size max_output_bytes = bits_to_bytes(max_output_bits);

  Byte* compressed = (Byte*)malloc(max_output_bytes);
  if (!compressed)
  {
    printf("[LZ78] Ошибка выделения памяти для сжатых данных\n");
    lz78_destroy(context);
    return RESULT_MEMORY_ERROR;
  }

  memset(compressed, 0, max_output_bytes);
  Size bit_pos = 0;

  Byte current_phrase[LZ78_DICT_SIZE];
  Size current_len = 0;

  for (Size i = 0; i < input_size; i++)
  {
    current_phrase[current_len++] = input[i];

    Size found_code = find_in_dictionary(context, current_phrase, current_len);

    if (found_code == LZ78_DICT_SIZE || i == input_size - 1)
    {
      // Фраза не найдена в словаре или это последний символ
      Size code_to_output;
      Byte next_char;

      if (current_len == 1)
      {
        // Одиночный символ, код 0
        code_to_output = 0;
        next_char = current_phrase[0];
      }
      else
      {
        // Берем предыдущую фразу (без последнего символа)
        code_to_output =
          find_in_dictionary(context, current_phrase, current_len - 1);
        next_char = current_phrase[current_len - 1];

        add_to_dictionary(context, current_phrase, current_len);
      }

      // Записываем код (12 бит) и следующий символ (8 бит)
      write_bits(compressed, &bit_pos, code_to_output, LZ78_MAX_CODE_LENGTH);
      write_bits(compressed, &bit_pos, next_char, 8);

      // Начинаем новую фразу
      current_len = 0;
    }
    // Иначе продолжаем накапливать фразу
  }

  Size compressed_bytes = bits_to_bytes(bit_pos);
  Byte* trimmed = (Byte*)realloc(compressed, compressed_bytes);
  if (trimmed)
  {
    compressed = trimmed;
  }

  *output = compressed;
  *output_size = compressed_bytes;

  printf("[LZ78] Сжатие завершено\n");
  printf("[LZ78] Исходный размер: %zu байт\n", input_size);
  printf("[LZ78] Сжатый размер: %zu байт\n", compressed_bytes);

  if (input_size > 0)
  {
    double ratio = (1.0 - (double)compressed_bytes / (double)input_size) * 100;
    printf("[LZ78] Коэффициент сжатия: %.2f%%\n", ratio);
  }

  lz78_destroy(context);
  return RESULT_OK;
}

Result lz78_decompress(const Byte* input, Size input_size, Byte** output,
                       Size* output_size)
{
  if (!input || !output || !output_size || input_size == 0)
  {
    printf("[LZ78] Ошибка: неверные параметры в lz78_decompress\n");
    return RESULT_INVALID_ARGUMENT;
  }

  printf("[LZ78] Начало декомпрессии LZ78\n");
  printf("[LZ78] Размер входных данных: %zu байт\n", input_size);
  printf("[LZ78] Ожидаемый выходной размер: %zu байт\n", *output_size);

  LZ78Context* context = lz78_create();
  if (!context)
  {
    return RESULT_MEMORY_ERROR;
  }

  Byte* decompressed = (Byte*)malloc(*output_size);
  if (!decompressed)
  {
    printf("[LZ78] Ошибка выделения памяти для декомпрессированных данных\n");
    lz78_destroy(context);
    return RESULT_MEMORY_ERROR;
  }

  Size out_pos = 0;
  Size bit_pos = 0;
  Size total_bits = input_size * 8;

  while (bit_pos < total_bits && out_pos < *output_size)
  {
    if (bit_pos + LZ78_MAX_CODE_LENGTH > total_bits)
    {
      break;
    }

    Size code = read_bits(input, &bit_pos, LZ78_MAX_CODE_LENGTH);

    if (bit_pos + 8 > total_bits)
    {
      break;
    }

    Byte next_char = (Byte)read_bits(input, &bit_pos, 8);

    Byte* phrase = NULL;
    Size phrase_len = 0;

    if (code == 0)
    {
      phrase = &next_char;
      phrase_len = 1;
    }
    else
    {
      if (code < context->dict_size)
      {
        phrase = context->dictionary[code].data;
        phrase_len = context->dictionary[code].length;
      }
      else
      {
        printf("[LZ78] Ошибка: код %zu не найден в словаре\n", code);
        free(decompressed);
        lz78_destroy(context);
        return RESULT_ERROR;
      }

      Byte* new_phrase = (Byte*)malloc(phrase_len + 1);
      if (new_phrase)
      {
        memcpy(new_phrase, phrase, phrase_len);
        new_phrase[phrase_len] = next_char;

        // Добавляем в словарь
        add_to_dictionary(context, new_phrase, phrase_len + 1);

        free(new_phrase);
      }
    }

    // Добавляем следующую фразу к выходу
    if (phrase)
    {
      // Добавляем старую фразу
      if (out_pos + phrase_len <= *output_size)
      {
        memcpy(decompressed + out_pos, phrase, phrase_len);
        out_pos += phrase_len;
      }

      // Добавляем следующий символ
      if (out_pos < *output_size)
      {
        decompressed[out_pos++] = next_char;
      }
    }
  }

  // Обрезаем буфер до фактического размера
  if (out_pos != *output_size)
  {
    Byte* trimmed = (Byte*)realloc(decompressed, out_pos);
    if (trimmed)
    {
      decompressed = trimmed;
      *output_size = out_pos;
    }
  }

  printf("[LZ78] Декомпрессия завершена\n");
  printf("[LZ78] Декомпрессировано байт: %zu\n", out_pos);

  printf("[LZ78] Первые 32 байта декомпрессированных данных: ");
  for (Size i = 0; i < 32 && i < out_pos; i++)
  {
    printf("%02X ", decompressed[i]);
  }
  printf("\n");

  printf("[LZ78] Первые 32 байта как текст: ");
  for (Size i = 0; i < 32 && i < out_pos; i++)
  {
    Byte b = decompressed[i];
    printf("%c", (b >= 32 && b <= 126) ? b : '.');
  }
  printf("\n");

  *output = decompressed;
  lz78_destroy(context);
  return RESULT_OK;
}
