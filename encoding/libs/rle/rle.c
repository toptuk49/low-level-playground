#include "rle.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"

#define MIN_REPEAT 4           // Минимальная длина для сжатия обычных символов
#define MIN_PREFIX_REPEAT 2    // Минимальная длина для сжатия префикса
#define MAX_REPEAT_LENGTH 255  // Максимальная длина, помещающаяся в 1 байт

RLEContext* rle_create(Byte prefix)
{
  RLEContext* context = (RLEContext*)malloc(sizeof(RLEContext));
  if (!context)
  {
    return NULL;
  }

  context->prefix = prefix;
  return context;
}

void rle_destroy(RLEContext* context)
{
  if (context)
  {
    free(context);
  }
}

Result rle_compress(const Byte* input, Size input_size, Byte** output,
                    Size* output_size, const RLEContext* context)
{
  if (!input || !output || !output_size || !context || input_size == 0)
  {
    printf("[RLE] Ошибка: неверные параметры в rle_compress\n");
    return RESULT_INVALID_ARGUMENT;
  }

  printf("[RLE] Начало RLE сжатия с префиксом 0x%02X\n", context->prefix);
  printf("[RLE] Размер входных данных: %zu байт\n", input_size);

  // Выделяем буфер размером до 2*input_size (худший случай - без сжатия)
  Byte* compressed = (Byte*)malloc(input_size * 2);
  if (!compressed)
  {
    printf("[RLE] Ошибка выделения памяти для сжатых данных\n");
    return RESULT_MEMORY_ERROR;
  }

  Size out_pos = 0;
  Size in_pos = 0;

  while (in_pos < input_size)
  {
    Byte current = input[in_pos];

    // Определяем длину последовательности одинаковых символов
    Size repeat_length = 1;
    while (in_pos + repeat_length < input_size &&
           input[in_pos + repeat_length] == current &&
           repeat_length <
             (MAX_REPEAT_LENGTH +
              (current == context->prefix ? MIN_PREFIX_REPEAT : MIN_REPEAT)))
    {
      repeat_length++;
    }

    if (current == context->prefix)
    {
      // Символ - префикс
      if (repeat_length >= MIN_PREFIX_REPEAT)
      {
        // Сжимаем последовательность префиксов
        if (repeat_length > MAX_REPEAT_LENGTH + 1)
        {
          repeat_length = MAX_REPEAT_LENGTH + 1;
        }

        Byte length = (Byte)(repeat_length - 1);

        if (out_pos + 3 > input_size * 2)
        {
          Byte* new_compressed = realloc(compressed, input_size * 2 + 256);
          if (!new_compressed)
          {
            free(compressed);
            return RESULT_MEMORY_ERROR;
          }
          compressed = new_compressed;
        }

        compressed[out_pos++] = context->prefix;
        compressed[out_pos++] = length;
        compressed[out_pos++] = context->prefix;

        in_pos += repeat_length;

        printf("[RLE] Сжато %zu символов префикса (0x%02X) в 3 байта\n",
               repeat_length, context->prefix);
      }
      else if (repeat_length == 1)
      {
        // Одиночный префикс
        if (out_pos + 2 > input_size * 2)
        {
          Byte* new_compressed = realloc(compressed, input_size * 2 + 256);
          if (!new_compressed)
          {
            free(compressed);
            return RESULT_MEMORY_ERROR;
          }
          compressed = new_compressed;
        }

        compressed[out_pos++] = context->prefix;
        compressed[out_pos++] = 0;

        in_pos++;

        printf("[RLE] Одиночный префикс (0x%02X) закодирован как 2 байта\n",
               context->prefix);
      }
      else
      {
        // 2 префикса подряд (длина 2) - тоже нужно закодировать
        Byte length = (Byte)(repeat_length - 1);

        if (out_pos + 3 > input_size * 2)
        {
          Byte* new_compressed = realloc(compressed, input_size * 2 + 256);
          if (!new_compressed)
          {
            free(compressed);
            return RESULT_MEMORY_ERROR;
          }
          compressed = new_compressed;
        }

        compressed[out_pos++] = context->prefix;
        compressed[out_pos++] = length;
        compressed[out_pos++] = context->prefix;

        in_pos += repeat_length;
      }
    }
    else
    {
      // Обычный символ
      if (repeat_length >= MIN_REPEAT)
      {
        // Сжимаем последовательность обычных символов
        if (repeat_length > MAX_REPEAT_LENGTH + 3)
        {
          repeat_length = MAX_REPEAT_LENGTH + 3;
        }

        Byte length = (Byte)(repeat_length - 3);

        if (out_pos + 3 > input_size * 2)
        {
          Byte* new_compressed = realloc(compressed, input_size * 2 + 256);
          if (!new_compressed)
          {
            free(compressed);
            return RESULT_MEMORY_ERROR;
          }
          compressed = new_compressed;
        }

        compressed[out_pos++] = context->prefix;
        compressed[out_pos++] = length;
        compressed[out_pos++] = current;

        in_pos += repeat_length;

        printf("[RLE] Сжато %zu символов 0x%02X ('%c') в 3 байта\n",
               repeat_length, current,
               (current >= 32 && current <= 126) ? current : '.');
      }
      else
      {
        // Недостаточно для сжатия - копируем как есть
        for (Size i = 0; i < repeat_length; i++)
        {
          if (out_pos + 1 > input_size * 2)
          {
            Byte* new_compressed = realloc(compressed, input_size * 2 + 256);
            if (!new_compressed)
            {
              free(compressed);
              return RESULT_MEMORY_ERROR;
            }
            compressed = new_compressed;
          }
          compressed[out_pos++] = current;
        }
        in_pos += repeat_length;
      }
    }
  }

  // Обрезаем буфер до фактического размера
  Byte* trimmed = (Byte*)realloc(compressed, out_pos);
  if (trimmed)
  {
    compressed = trimmed;
  }
  else if (out_pos > 0)
  {
    // Если realloc не удался, но данные есть - продолжаем с исходным буфером
    printf("[RLE] Предупреждение: не удалось обрезать буфер\n");
  }

  *output = compressed;
  *output_size = out_pos;

  printf("[RLE] Сжатие завершено\n");
  printf("[RLE] Исходный размер: %zu байт\n", input_size);
  printf("[RLE] Сжатый размер: %zu байт\n", out_pos);

  if (input_size > 0)
  {
    double ratio = (1.0 - (double)out_pos / (double)input_size) * 100;
    printf("[RLE] Коэффициент сжатия: %.2f%%\n", ratio);
  }

  return RESULT_OK;
}

Result rle_decompress(const Byte* input, Size input_size, Byte** output,
                      Size* output_size, const RLEContext* context)
{
  if (!input || !output || !output_size || !context || input_size == 0)
  {
    printf("[RLE] Ошибка: неверные параметры в rle_decompress\n");
    return RESULT_INVALID_ARGUMENT;
  }

  printf("[RLE] Начало RLE декомпрессии с префиксом 0x%02X\n", context->prefix);
  printf("[RLE] Размер входных данных: %zu байт\n", input_size);
  printf("[RLE] Ожидаемый выходной размер: %zu байт\n", *output_size);

  // Первый проход: определяем размер выходного буфера
  Size estimated_size = 0;
  Size in_pos = 0;

  while (in_pos < input_size)
  {
    Byte current = input[in_pos];

    if (current == context->prefix && in_pos + 1 < input_size)
    {
      // Возможная RLE-последовательность
      Byte length = input[in_pos + 1];

      if (length == 0)
      {
        // Одиночный префикс
        estimated_size++;
        in_pos += 2;
      }
      else if (in_pos + 2 < input_size)
      {
        // Полная RLE-последовательность
        Byte symbol = input[in_pos + 2];

        if (symbol == context->prefix)
        {
          // Последовательность префиксов: длина + 1
          estimated_size += length + 1;
        }
        else
        {
          // Последовательность обычных символов: длина + 3
          estimated_size += length + 3;
        }
        in_pos += 3;
      }
      else
      {
        // Некорректная последовательность - трактуем как обычный байт
        estimated_size++;
        in_pos++;
      }
    }
    else
    {
      // Обычный байт
      estimated_size++;
      in_pos++;
    }
  }

  printf("[RLE] Расчетный размер после декомпрессии: %zu байт\n",
         estimated_size);

  if (*output_size == 0)
  {
    *output_size = estimated_size;
  }
  else if (estimated_size != *output_size)
  {
    printf(
      "[RLE] Предупреждение: расчетный размер (%zu) не совпадает с ожидаемым "
      "(%zu)\n",
      estimated_size, *output_size);
    // Используем больший из размеров для безопасности
    if (estimated_size > *output_size)
    {
      *output_size = estimated_size;
    }
  }

  // Выделяем буфер для выходных данных
  Byte* decompressed = (Byte*)malloc(*output_size);
  if (!decompressed)
  {
    printf("[RLE] Ошибка выделения памяти для декомпрессированных данных\n");
    return RESULT_MEMORY_ERROR;
  }

  // Второй проход: декомпрессия
  in_pos = 0;
  Size out_pos = 0;

  while (in_pos < input_size && out_pos < *output_size)
  {
    Byte current = input[in_pos];

    if (current == context->prefix && in_pos + 1 < input_size)
    {
      // Возможная RLE-последовательность
      Byte length = input[in_pos + 1];

      if (length == 0)
      {
        // Одиночный префикс
        if (out_pos < *output_size)
        {
          decompressed[out_pos++] = context->prefix;
        }
        in_pos += 2;

        printf("[RLE] Декомпрессирован одиночный префикс (0x%02X)\n",
               context->prefix);
      }
      else if (in_pos + 2 < input_size)
      {
        // Полная RLE-последовательность
        Byte symbol = input[in_pos + 2];

        Size repeat_count;
        if (symbol == context->prefix)
        {
          // Последовательность префиксов
          repeat_count = length + 1;
        }
        else
        {
          // Последовательность обычных символов
          repeat_count = length + 3;
        }

        for (Size i = 0; i < repeat_count && out_pos < *output_size; i++)
        {
          decompressed[out_pos++] = symbol;
        }
        in_pos += 3;

        if (symbol == context->prefix)
        {
          printf("[RLE] Декомпрессировано %zu символов префикса (0x%02X)\n",
                 repeat_count, symbol);
        }
        else
        {
          printf("[RLE] Декомпрессировано %zu символов 0x%02X ('%c')\n",
                 repeat_count, symbol,
                 (symbol >= 32 && symbol <= 126) ? symbol : '.');
        }
      }
      else
      {
        // Некорректная последовательность - трактуем как обычный байт
        if (out_pos < *output_size)
        {
          decompressed[out_pos++] = current;
        }
        in_pos++;
      }
    }
    else
    {
      // Обычный байт
      if (out_pos < *output_size)
      {
        decompressed[out_pos++] = current;
      }
      in_pos++;
    }
  }

  // Проверяем, все ли декомпрессировано
  if (out_pos != *output_size)
  {
    printf("[RLE] ВНИМАНИЕ: декомпрессировано %zu байт из %zu ожидаемых\n",
           out_pos, *output_size);

    if (out_pos == 0)
    {
      printf(
        "[RLE] КРИТИЧЕСКАЯ ОШИБКА: не декомпрессировано ни одного байта!\n");
      free(decompressed);
      return RESULT_ERROR;
    }

    // Обрезаем буфер до фактического размера
    Byte* trimmed = (Byte*)realloc(decompressed, out_pos);
    if (trimmed)
    {
      decompressed = trimmed;
      *output_size = out_pos;
    }
    else
    {
      printf("[RLE] Предупреждение: не удалось обрезать буфер\n");
    }
  }

  printf("[RLE] Декомпрессия завершена\n");
  printf("[RLE] Первые 32 байт декомпрессированных данных: ");
  for (Size i = 0; i < 32 && i < *output_size; i++)
  {
    Byte b = decompressed[i];
    printf("%02X ", b);
  }
  printf("\n");
  printf("[RLE] Первые 32 байт как текст: ");
  for (Size i = 0; i < 32 && i < *output_size; i++)
  {
    Byte b = decompressed[i];
    printf("%c", (b >= 32 && b <= 126) ? b : '.');
  }
  printf("\n");

  *output = decompressed;
  return RESULT_OK;
}

Result rle_serialize_context(const RLEContext* context, Byte** data, Size* size)
{
  if (!context || !data || !size)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  *size = 1;  // Только префикс
  *data = (Byte*)malloc(*size);
  if (!*data)
  {
    return RESULT_MEMORY_ERROR;
  }

  (*data)[0] = context->prefix;
  return RESULT_OK;
}

Result rle_deserialize_context(RLEContext* context, const Byte* data, Size size)
{
  if (!context || !data || size != 1)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  context->prefix = data[0];
  return RESULT_OK;
}

Byte rle_get_prefix(const RLEContext* context)
{
  return context ? context->prefix : 0;
}

Result rle_set_prefix(RLEContext* context, Byte prefix)
{
  if (!context)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  context->prefix = prefix;
  return RESULT_OK;
}

// Анализ данных для выбора оптимального префикса
Byte rle_analyze_prefix(const Byte* data, Size size)
{
  if (!data || size == 0)
  {
    return 0;  // Возвращаем 0 как префикс по умолчанию
  }

  // Подсчитываем частоты символов
  DWord frequencies[256] = {0};
  for (Size i = 0; i < size; i++)
  {
    frequencies[data[i]]++;
  }

  // Ищем наименее частый символ (он будет лучшим префиксом)
  DWord min_frequency = (DWord)-1;
  Byte best_prefix = 0;

  // Начинаем поиск с символа 1, чтобы избежать 0 (нулевой байт)
  for (int i = 1; i < 256; i++)
  {
    if (frequencies[i] < min_frequency)
    {
      min_frequency = frequencies[i];
      best_prefix = (Byte)i;
    }
  }

  // Если все символы одинаково часты или best_prefix == 0, выбираем символ,
  // который точно не встречается в данных
  if (min_frequency > 0 || best_prefix == 0)
  {
    // Ищем символ, который вообще не встречается в данных
    for (int i = 1; i < 256; i++)
    {
      if (frequencies[i] == 0)
      {
        best_prefix = (Byte)i;
        printf(
          "[RLE] Найден неиспользуемый символ 0x%02X в качестве префикса\n",
          best_prefix);
        break;
      }
    }

    // Если все символы от 1 до 255 используются (маловероятно),
    // используем 0xFF как префикс
    if (best_prefix == 0)
    {
      best_prefix = 0xFF;
      printf("[RLE] Все символы используются, выбран префикс 0xFF\n");
    }
  }

  printf("[RLE] Анализ данных: выбран префикс 0x%02X ", best_prefix);
  if (best_prefix >= 32 && best_prefix <= 126)
  {
    printf("('%c') ", best_prefix);
  }
  printf("(встречается %u раз)\n", frequencies[best_prefix]);

  return best_prefix;
}

// Тестирование RLE на данных
void rle_test_compression(const Byte* data, Size size, Byte prefix)
{
  if (!data || size == 0)
  {
    return;
  }

  printf("\n=== Тестирование RLE сжатия ===\n");
  printf("Размер данных: %zu байт\n", size);
  printf("Используемый префикс: 0x%02X", prefix);
  if (prefix >= 32 && prefix <= 126)
  {
    printf(" ('%c')", prefix);
  }
  printf("\n");

  RLEContext* context = rle_create(prefix);
  if (!context)
  {
    printf("Ошибка создания контекста RLE\n");
    return;
  }

  Byte* compressed = NULL;
  Size compressed_size = 0;

  Result result =
    rle_compress(data, size, &compressed, &compressed_size, context);
  if (result == RESULT_OK && compressed)
  {
    printf("Сжатие успешно: %zu -> %zu байт (%.2f%%)\n", size, compressed_size,
           (1.0 - (double)compressed_size / (double)size) * 100);

    // Анализ сжатых данных
    printf("Первые 32 байт сжатых данных: ");
    for (Size i = 0; i < 32 && i < compressed_size; i++)
    {
      printf("%02X ", compressed[i]);
    }
    printf("\n");

    // Тест декомпрессии
    Byte* decompressed = NULL;
    Size decompressed_size = size;

    result = rle_decompress(compressed, compressed_size, &decompressed,
                            &decompressed_size, context);

    if (result == RESULT_OK && decompressed)
    {
      // Проверяем корректность
      bool correct = true;
      if (decompressed_size != size)
      {
        printf("Ошибка: размер не совпадает (ожидалось %zu, получено %zu)\n",
               size, decompressed_size);
        correct = false;
      }
      else
      {
        for (Size i = 0; i < size; i++)
        {
          if (data[i] != decompressed[i])
          {
            printf("Ошибка: несовпадение в позиции %zu: 0x%02X != 0x%02X\n", i,
                   data[i], decompressed[i]);
            correct = false;
            break;
          }
        }
      }

      if (correct)
      {
        printf("Декомпрессия успешна, данные корректны\n");
      }
      else
      {
        printf("Ошибка: данные после декомпрессии не совпадают\n");

        printf("Первые 64 символа оригинала: ");
        for (Size i = 0; i < 64 && i < size; i++)
        {
          Byte b = data[i];
          printf("%c", (b >= 32 && b <= 126) ? b : '.');
        }
        printf("\n");

        printf("Первые 64 символа результата: ");
        for (Size i = 0; i < 64 && i < decompressed_size; i++)
        {
          Byte b = decompressed[i];
          printf("%c", (b >= 32 && b <= 126) ? b : '.');
        }
        printf("\n");
      }
      free(decompressed);
    }
    else
    {
      printf("Ошибка декомпрессии!\n");
    }

    free(compressed);
  }
  else
  {
    printf("Ошибка сжатия!\n");
  }

  rle_destroy(context);
}
