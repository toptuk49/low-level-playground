#include "rle_analysis.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "markov_model.h"
#include "rle.h"

static void analyze_repetitive_patterns(const MarkovModel* markov);

Result analyze_rle_efficiency(const Byte* data, Size data_size)
{
  // 1. Создаем модель Маркова
  MarkovModel* markov = markov_model_create();
  markov_model_process_data(markov, data, data_size);

  // 2. Выбираем оптимальный префикс для RLE
  Byte rle_prefix = rle_analyze_prefix(data, data_size);
  RLEContext* rle_ctx = rle_create(rle_prefix);

  // 3. Выполняем RLE сжатие
  Byte* rle_output = NULL;
  Size rle_size = 0;
  rle_compress(data, data_size, &rle_output, &rle_size, rle_ctx);

  // 4. Рассчитываем размеры таблиц
  Size rle_table_size = 256;  // Таблица частот (1 байт на символ)

  Size markov_pairs = 0;
  for (int i = 0; i < ALPHABET_SIZE; i++)
  {
    for (int j = 0; j < ALPHABET_SIZE; j++)
    {
      if (markov_model_get_pair_count(markov, i, j) > 0)
      {
        markov_pairs++;
      }
    }
  }
  Size markov_table_size = markov_pairs * 6;  // 2 байта пара + 4 байта частота

  // 5. Общие размеры
  Size total_rle = rle_size + rle_table_size;
  double markov_info_bytes = markov_model_get_total_information_bytes(markov);
  Size total_markov = (Size)ceil(markov_info_bytes) + markov_table_size;

  // 6. Вывод сравнения
  printf("\n=== Сравнение RLE с моделью Маркова ===\n");
  printf("Исходный размер: %zu байт\n", data_size);
  printf("\nRLE сжатие:\n");
  printf("  Сжатые данные: %zu байт\n", rle_size);
  printf("  Таблица частот: %zu байт\n", rle_table_size);
  printf("  Итого: %zu байт (сжатие %.1f%%)\n", total_rle,
         (1.0 - (double)total_rle / data_size) * 100);

  printf("\nОценка по Маркову (CM1):\n");
  printf("  Теор. информация: %.1f байт\n", markov_info_bytes);
  printf("  Таблица пар: %zu байт\n", markov_table_size);
  printf("  Итого: %zu байт (предел %.1f%%)\n", total_markov,
         (1.0 - (double)total_markov / data_size) * 100);

  printf("\nРазница: %+zd байт (RLE %s теоретического предела)\n",
         (long)total_rle - (long)total_markov,
         total_rle > total_markov ? "превышает" : "в пределах");

  // Анализ повторяющихся последовательностей
  analyze_repetitive_patterns(markov);

  // Очистка
  free(rle_output);
  rle_destroy(rle_ctx);
  markov_model_destroy(markov);

  return RESULT_OK;
}

static void analyze_repetitive_patterns(const MarkovModel* markov)
{
  printf("\n=== Анализ для RLE эффективности ===\n");

  // Символы с высокой вероятностью повторения
  printf("Символы с P(x|x) > 0.5:\n");
  for (int i = 0; i < ALPHABET_SIZE; i++)
  {
    double prob = markov_model_get_conditional_probability(markov, i, i);
    if (prob > 0.5 && markov_model_get_prefix_count(markov, i) > 0)
    {
      printf("  '%c' (0x%02X): P=%.3f, встречается %" PRIu64 " раз\n",
             (i >= 32 && i < 127) ? i : '.', i, prob,
             markov_model_get_prefix_count(markov, i));
    }
  }
}
