#include "lz77.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"

LZ77Context* lz77_create(Byte prefix)
{
  LZ77Context* ctx = (LZ77Context*)malloc(sizeof(LZ77Context));
  if (!ctx)
    return NULL;

  ctx->prefix = prefix;
  ctx->window = (Byte*)malloc(LZ77_WINDOW_SIZE);
  if (!ctx->window)
  {
    free(ctx);
    return NULL;
  }

  memset(ctx->window, 0, LZ77_WINDOW_SIZE);
  ctx->window_pos = 0;

  return ctx;
}

void lz77_destroy(LZ77Context* context)
{
  if (!context)
    return;

  if (context->window)
    free(context->window);

  free(context);
}

static void find_best_match(const Byte* window, Size window_len,
                            const Byte* data, Size data_len, Size* best_offset,
                            Size* best_length)
{
  *best_offset = 0;
  *best_length = 0;

  if (window_len == 0 || data_len < LZ77_MIN_MATCH)
    return;

  Size max_offset = window_len;
  if (max_offset > 1024)
    max_offset = 1024;

  // Ищем от конца окна к началу
  for (Size offset = 1; offset <= max_offset; offset++)
  {
    Size len = 0;
    Size window_pos = window_len - offset;

    while (len < data_len && len < LZ77_MAX_MATCH &&
           window_pos + len < window_len &&
           window[window_pos + len] == data[len])
    {
      len++;
    }

    if (len >= LZ77_MIN_MATCH && len > *best_length)
    {
      *best_length = len;
      *best_offset = offset;
      if (len == LZ77_MAX_MATCH)
        break;
    }
  }
}

Result lz77_compress(const Byte* input, Size input_size, Byte** output,
                     Size* output_size, Byte prefix)
{
  if (!input || !output || !output_size || input_size == 0)
    return RESULT_INVALID_ARGUMENT;

  printf("[LZ77] Сжатие: %zu байт, префикс=0x%02X\n", input_size, prefix);

  Byte* window = (Byte*)malloc(LZ77_WINDOW_SIZE);
  if (!window)
    return RESULT_MEMORY_ERROR;

  memset(window, 0, LZ77_WINDOW_SIZE);
  Size window_len = 0;

  Size max_out = input_size * 2 + 1024;
  Byte* out_buf = (Byte*)malloc(max_out);
  if (!out_buf)
  {
    free(window);
    return RESULT_MEMORY_ERROR;
  }

  Size out_pos = 0;
  Size in_pos = 0;

  while (in_pos < input_size)
  {
    Size remaining = input_size - in_pos;
    Size lookahead =
      (remaining > LZ77_LOOKAHEAD_SIZE) ? LZ77_LOOKAHEAD_SIZE : remaining;

    Size match_offset = 0;
    Size match_len = 0;

    if (window_len >= LZ77_MIN_MATCH && lookahead >= LZ77_MIN_MATCH)
    {
      find_best_match(window, window_len, input + in_pos, lookahead,
                      &match_offset, &match_len);
    }

    if (match_len >= LZ77_MIN_MATCH)
    {
      Size S = match_offset;  // 1-1024
      Size L = match_len;     // 3-65

      if (S > 1024)
        S = 1024;
      if (L > LZ77_MAX_MATCH)
        L = LZ77_MAX_MATCH;

      // Кодируем L-2 вместо L-3, чтобы L_encoded был от 1 до 63
      // Это предотвращает конфликт combined=0 с символом префикса
      Size L_enc = L - 2;  // 1-63 (было 0-62)
      Size S_enc = S - 1;  // 0-1023

      Byte S_high = (S_enc >> 8) & 0x03;
      Byte S_low = S_enc & 0xFF;
      Byte combined = (S_high << 6) | (L_enc & 0x3F);

      // Теперь combined никогда не будет 0 (т.к. L_enc >= 1)
      // Это решает конфликт с символом префикса (prefix, 0)

      if (out_pos + 3 > max_out)
      {
        max_out *= 2;
        Byte* tmp = (Byte*)realloc(out_buf, max_out);
        if (!tmp)
        {
          free(window);
          free(out_buf);
          return RESULT_MEMORY_ERROR;
        }
        out_buf = tmp;
      }

      out_buf[out_pos++] = prefix;
      out_buf[out_pos++] = combined;
      out_buf[out_pos++] = S_low;

      for (Size i = 0; i < L; i++)
      {
        Byte b = input[in_pos + i];

        if (window_len < LZ77_WINDOW_SIZE)
        {
          window[window_len++] = b;
        }
        else
        {
          memmove(window, window + 1, LZ77_WINDOW_SIZE - 1);
          window[LZ77_WINDOW_SIZE - 1] = b;
        }
      }

      in_pos += L;
    }
    else
    {
      Byte b = input[in_pos];

      if (b == prefix)
      {
        if (out_pos + 2 > max_out)
        {
          max_out *= 2;
          Byte* tmp = (Byte*)realloc(out_buf, max_out);
          if (!tmp)
          {
            free(window);
            free(out_buf);
            return RESULT_MEMORY_ERROR;
          }
          out_buf = tmp;
        }

        out_buf[out_pos++] = prefix;
        out_buf[out_pos++] = 0;
      }
      else
      {
        if (out_pos + 1 > max_out)
        {
          max_out *= 2;
          Byte* tmp = (Byte*)realloc(out_buf, max_out);
          if (!tmp)
          {
            free(window);
            free(out_buf);
            return RESULT_MEMORY_ERROR;
          }
          out_buf = tmp;
        }

        out_buf[out_pos++] = b;
      }

      if (window_len < LZ77_WINDOW_SIZE)
      {
        window[window_len++] = b;
      }
      else
      {
        memmove(window, window + 1, LZ77_WINDOW_SIZE - 1);
        window[LZ77_WINDOW_SIZE - 1] = b;
      }

      in_pos++;
    }
  }

  free(window);

  if (out_pos == 0)
  {
    free(out_buf);
    *output = NULL;
    *output_size = 0;
  }
  else
  {
    Byte* trimmed = (Byte*)realloc(out_buf, out_pos);
    if (trimmed)
      out_buf = trimmed;

    *output = out_buf;
    *output_size = out_pos;
  }

  printf("[LZ77] Сжатие завершено: %zu -> %zu байт\n", input_size, out_pos);

  return RESULT_OK;
}

Result lz77_decompress(const Byte* input, Size input_size, Byte** output,
                       Size* output_size, Byte prefix)
{
  if (!input || !output || !output_size || input_size == 0)
    return RESULT_INVALID_ARGUMENT;

  printf("[LZ77] Декомпрессия: вход=%zu, ожидаемый выход=%zu, префикс=0x%02X\n",
         input_size, *output_size, prefix);

  Byte* window = (Byte*)malloc(LZ77_WINDOW_SIZE);
  if (!window)
    return RESULT_MEMORY_ERROR;

  memset(window, 0, LZ77_WINDOW_SIZE);
  Size window_len = 0;

  Byte* out_buf = (Byte*)malloc(*output_size);
  if (!out_buf)
  {
    free(window);
    return RESULT_MEMORY_ERROR;
  }

  Size out_pos = 0;
  Size in_pos = 0;

  while (in_pos < input_size && out_pos < *output_size)
  {
    Byte b = input[in_pos];

    if (b == prefix)
    {
      if (in_pos + 1 >= input_size)
      {
        printf("[LZ77] Ошибка: неполный префикс\n");
        free(window);
        free(out_buf);
        return RESULT_ERROR;
      }

      Byte next = input[in_pos + 1];

      if (next == 0)
      {
        // Символ = префиксу: (p, 0)
        in_pos += 2;

        if (out_pos >= *output_size)
        {
          printf("[LZ77] Ошибка: выход за пределы буфера\n");
          free(window);
          free(out_buf);
          return RESULT_ERROR;
        }

        out_buf[out_pos++] = prefix;

        if (window_len < LZ77_WINDOW_SIZE)
        {
          window[window_len++] = prefix;
        }
        else
        {
          memmove(window, window + 1, LZ77_WINDOW_SIZE - 1);
          window[LZ77_WINDOW_SIZE - 1] = prefix;
        }
      }
      else
      {
        // Ссылка: (p, combined, S_low)
        if (in_pos + 2 >= input_size)
        {
          printf("[LZ77] Ошибка: неполная ссылка\n");
          free(window);
          free(out_buf);
          return RESULT_ERROR;
        }

        Byte combined = input[in_pos + 1];
        Byte S_low = input[in_pos + 2];
        in_pos += 3;

        // Декодируем L и S
        Byte S_high = (combined >> 6) & 0x03;
        Byte L_enc = combined & 0x3F;

        // Декодируем L = L_encoded + 2 (было +3)
        Size S = (((Size)S_high << 8) | S_low) + 1;  // 1-1024
        Size L = (Size)L_enc + 2;                    // 3-65 (было +3)

        // Проверяем корректность
        if (L < LZ77_MIN_MATCH || L > LZ77_MAX_MATCH || S == 0 || S > 1024)
        {
          printf("[LZ77] Ошибка: некорректная ссылка S=%zu, L=%zu\n", S, L);
          free(window);
          free(out_buf);
          return RESULT_ERROR;
        }

        // Проверяем, что есть место в выходном буфере
        if (out_pos + L > *output_size)
        {
          printf("[LZ77] Ошибка: ссылка выходит за пределы буфера\n");
          free(window);
          free(out_buf);
          return RESULT_ERROR;
        }

        // Копируем L символов из окна
        // S отсчитывается от конца окна (1 = последний символ)
        for (Size i = 0; i < L; i++)
        {
          Byte symbol = 0;

          if (S <= window_len)
          {
            // Вычисляем позицию в окне
            if (i < S)
            {
              // Берем из существующего окна
              Size pos = window_len - S + i;
              if (pos < window_len)
                symbol = window[pos];
            }
            else
            {
              // Копируем из только что декодированных символов
              // Это важно для случая, когда L > S
              symbol = out_buf[out_pos + i - S];
            }
          }
          // Если S > window_len, symbol остается 0

          out_buf[out_pos + i] = symbol;
        }

        for (Size i = 0; i < L; i++)
        {
          Byte symbol = out_buf[out_pos + i];

          if (window_len < LZ77_WINDOW_SIZE)
          {
            window[window_len++] = symbol;
          }
          else
          {
            memmove(window, window + 1, LZ77_WINDOW_SIZE - 1);
            window[LZ77_WINDOW_SIZE - 1] = symbol;
          }
        }

        out_pos += L;
      }
    }
    else
    {
      in_pos++;

      if (out_pos >= *output_size)
      {
        printf("[LZ77] Ошибка: выход за пределы буфера\n");
        free(window);
        free(out_buf);
        return RESULT_ERROR;
      }

      out_buf[out_pos++] = b;

      if (window_len < LZ77_WINDOW_SIZE)
      {
        window[window_len++] = b;
      }
      else
      {
        memmove(window, window + 1, LZ77_WINDOW_SIZE - 1);
        window[LZ77_WINDOW_SIZE - 1] = b;
      }
    }
  }

  free(window);

  if (out_pos != *output_size)
  {
    printf(
      "[LZ77] ВНИМАНИЕ: размер не совпадает! Ожидалось %zu, получено %zu\n",
      *output_size, out_pos);

    *output_size = out_pos;

    if (out_pos > 0)
    {
      Byte* trimmed = (Byte*)realloc(out_buf, out_pos);
      if (trimmed)
        out_buf = trimmed;
    }
    else
    {
      free(out_buf);
      *output = NULL;
      return RESULT_OK;
    }
  }

  *output = out_buf;

  printf("[LZ77] Декомпрессия завершена: %zu байт\n", out_pos);

  return RESULT_OK;
}

Byte lz77_analyze_prefix(const Byte* data, Size size)
{
  if (!data || size == 0)
    return 0x01;  // Значение по умолчанию

  DWord freq[256] = {0};
  for (Size i = 0; i < size; i++)
  {
    freq[data[i]]++;
  }

  // Ищем наименее частый символ (кроме 0)
  Byte best = 0x01;
  DWord min_freq = (DWord)-1;

  for (int i = 1; i < 256; i++)
  {
    if (freq[i] < min_freq)
    {
      min_freq = freq[i];
      best = (Byte)i;
    }
  }

  // Если все символы встречаются, пытаемся найти неиспользуемый
  if (min_freq > 0)
  {
    for (int i = 1; i < 256; i++)
    {
      if (freq[i] == 0)
      {
        best = (Byte)i;
        break;
      }
    }
  }

  printf("[LZ77] Выбран префикс: 0x%02X (встречается %u раз)\n", best,
         freq[best]);

  return best;
}
