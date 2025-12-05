#include "entropy.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"
#include "types.h"

double calculate_entropy(const Byte* data, Size size)
{
  if (!data || size == 0)
  {
    return 0.0;
  }

  DWord frequencies[256] = {0};

  for (Size i = 0; i < size; i++)
  {
    frequencies[data[i]]++;
  }

  double entropy = 0.0;
  for (int i = 0; i < 256; i++)
  {
    if (frequencies[i] > 0)
    {
      double probability = (double)frequencies[i] / (double)size;
      entropy -= probability * log2(probability);
    }
  }

  return entropy;
}

double calculate_information_lower_bound(const Byte* data, Size size)
{
  double entropy = calculate_entropy(data, size);
  return entropy * (double)size;
}

double calculate_compression_ratio(Size original_size, Size compressed_size)
{
  if (original_size == 0)
  {
    return 0.0;
  }

  return (1.0 - (double)compressed_size / (double)original_size) * 100.0;
}

void analyze_file_entropy(const char* filename, Size compressed_size)
{
  if (filename == NULL)
  {
    return;
  }

  File* file = file_create(filename);

  if (file == NULL)
  {
    return;
  }

  if (file_open_for_read(file) != RESULT_OK)
  {
    file_destroy(file);
    return;
  }

  if (file_read_bytes(file) != RESULT_OK)
  {
    file_close(file);
    file_destroy(file);
    return;
  }

  const Byte* data = file_get_buffer(file);
  Size size = file_get_size(file);

  if (size == 0)
  {
    file_close(file);
    file_destroy(file);
    return;
  }

  double entropy = calculate_entropy(data, size);
  double info_bits = calculate_information_lower_bound(data, size);
  double info_bytes = info_bits / 8.0;

  printf("\n=== Анализ энтропии ===\n");
  printf("Размер файла: %zu байт\n", size);
  printf("Энтропия: %.4f бит/символ\n", entropy);
  printf("Оценка снизу количества информации: %.2f бит (%.2f байт)\n",
         info_bits, info_bytes);

  if (compressed_size > 0)
  {
    double compression_ratio =
      calculate_compression_ratio(size, compressed_size);
    double actual_bits = (double)compressed_size * 8.0;

    printf("Размер сжатых данных: %zu байт (%.2f бит)\n", compressed_size,
           actual_bits);
    printf("Коэффициент сжатия: %.2f%%\n", compression_ratio);
    printf("Отношение L/H: %.4f (L=%.2f, H=%.2f)\n", actual_bits / info_bits,
           actual_bits, info_bits);

    if (actual_bits >= info_bits)
    {
      printf("Сжатие близко к оптимальному (L ≥ H)\n");
    }
    else
    {
      printf("Теоретически невозможно (L < H) - проверьте расчеты\n");
    }
  }

  DWord frequencies[256] = {0};
  int unique_symbols = 0;

  for (Size i = 0; i < size; i++)
  {
    if (frequencies[data[i]] == 0)
    {
      unique_symbols++;
    }

    frequencies[data[i]]++;
  }

  printf("Уникальных символов: %d\n", unique_symbols);
  printf("5 наиболее частых символов:\n");
  for (int top = 0; top < 5 && top < unique_symbols; top++)
  {
    DWord max_frequency = 0;
    Byte max_symbol = 0;

    for (int i = 0; i < 256; i++)
    {
      if (frequencies[i] > max_frequency)
      {
        max_frequency = frequencies[i];
        max_symbol = (Byte)i;
      }
    }

    if (max_frequency > 0)
    {
      double prob = (double)max_frequency / (double)size;
      printf("  '%c' (0x%02X): %u раз (%.4f)\n",
             isprint(max_symbol) ? max_symbol : '.', max_symbol, max_frequency,
             prob);
      frequencies[max_symbol] = 0;  // Убираем для поиска следующего
    }
  }

  file_close(file);
  file_destroy(file);
}
