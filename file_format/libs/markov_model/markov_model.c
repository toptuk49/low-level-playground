#include "markov_model.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"

struct MarkovModel
{
  uint64_t pair_counts[ALPHABET_SIZE][ALPHABET_SIZE];
  uint64_t prefix_counts[ALPHABET_SIZE];
  uint64_t symbol_counts[ALPHABET_SIZE];
  uint64_t total_pairs;
  double total_information_bits;
};

MarkovModel* markov_model_create(void)
{
  MarkovModel* model = malloc(sizeof(MarkovModel));
  if (!model)
  {
    return NULL;
  }

  memset(model->pair_counts, 0, sizeof(model->pair_counts));
  memset(model->prefix_counts, 0, sizeof(model->prefix_counts));
  memset(model->symbol_counts, 0, sizeof(model->prefix_counts));
  model->total_pairs = 0;
  model->total_information_bits = 0.0;

  return model;
}

void markov_model_destroy(MarkovModel* self)
{
  free(self);
}

Result markov_model_process_data(MarkovModel* self, const Byte* data,
                                 Size data_size)
{
  if (!self || !data || data_size < 2)
  {
    return RESULT_INVALID_ARGUMENT;
  }

  memset(self->pair_counts, 0, sizeof(self->pair_counts));
  memset(self->prefix_counts, 0, sizeof(self->prefix_counts));
  memset(self->symbol_counts, 0, sizeof(self->symbol_counts));
  self->total_pairs = 0;
  self->total_information_bits = 0.0;

  for (Size i = 0; i < data_size - 1; i++)
  {
    Byte first = data[i];
    Byte second = data[i + 1];

    self->pair_counts[first][second]++;
    self->prefix_counts[first]++;
    self->total_pairs++;
  }

  for (Size i = 0; i < data_size; i++)
  {
    self->symbol_counts[data[i]]++;
  }

  for (int first = 0; first < ALPHABET_SIZE; first++)
  {
    for (int second = 0; second < ALPHABET_SIZE; second++)
    {
      if (self->pair_counts[first][second] > 0 &&
          self->prefix_counts[first] > 0)
      {
        double probability = (double)self->pair_counts[first][second] /
                             (double)self->prefix_counts[first];
        double information = -log2(probability);
        self->total_information_bits +=
          information * (double)self->pair_counts[first][second];

        // Для отладки:
        const int ascii_beginning = 32;
        const int ascii_end = 127;
        const int amount = 10;
        if (data_size < amount)
        {  // выводим только для маленьких тестов
          printf(
            "  Пара %c%c: count=%" PRIu64 ", p=%.3f, I=%.2f, сумма=%.2f\n",
            (first >= ascii_beginning && first < ascii_end) ? first : '.',
            (second >= ascii_beginning && second < ascii_end) ? second : '.',
            self->pair_counts[first][second], probability, information,
            information * (double)self->pair_counts[first][second]);
        }
      }
    }
  }
  // Добавляем информацию для первого символа (безусловная вероятность 1/256)
  if (data_size > 0)
  {
    double first_symbol_info = -log2(1.0 / ALPHABET_SIZE);
    self->total_information_bits += first_symbol_info;
  }

  return RESULT_OK;
}

uint64_t markov_model_get_pair_count(const MarkovModel* self, Byte first,
                                     Byte second)
{
  return self ? self->pair_counts[first][second] : 0;
}

uint64_t markov_model_get_prefix_count(const MarkovModel* self, Byte prefix)
{
  return self ? self->prefix_counts[prefix] : 0;
}

uint64_t markov_model_get_symbol_count(const MarkovModel* self, Byte symbol)
{
  return self ? self->symbol_counts[symbol] : 0;
}

double markov_model_get_conditional_probability(const MarkovModel* self,
                                                Byte first, Byte second)
{
  if (!self || self->prefix_counts[first] == 0)
  {
    return 0.0;
  }
  return (double)self->pair_counts[first][second] /
         (double)self->prefix_counts[first];
}

double markov_model_get_information(const MarkovModel* self, Byte first,
                                    Byte second)
{
  double probability =
    markov_model_get_conditional_probability(self, first, second);
  if (probability == 0.0)
  {
    return 0.0;
  }
  return -log2(probability);
}

double markov_model_get_total_information_bits(const MarkovModel* self)
{
  return self ? self->total_information_bits : 0.0;
}

double markov_model_get_total_information_bytes(const MarkovModel* self)
{
  const Byte bytes = 8;
  return self ? self->total_information_bits / (double)bytes : 0.0;
}

uint64_t markov_model_get_total_pairs(const MarkovModel* self)
{
  return self ? self->total_pairs : 0;
}

bool markov_model_validate_test_cases(void)
{
  const char* test_cases[] = {
    "ab",      // 8 бит
    "abcd",    // 8 бит
    "abab",    // 8 бит
    "abac",    // 10 бит
    "aabacad"  // 16 бит
  };

  const double expected_bits[] = {8.0, 8.0, 8.0, 10.0, 16.0};
  const char* descriptions[] = {
    "Два разных символа", "Четыре разных символа", "Повторяющаяся пара",
    "Три символа с частичным повтором", "Несколько символов с повторами"};
  const int num_cases = 5;

  bool all_passed = true;

  for (int i = 0; i < num_cases; i++)
  {
    printf("\nТест %d: %s ('%s')\n", i + 1, descriptions[i], test_cases[i]);

    MarkovModel* model = markov_model_create();
    if (!model)
    {
      printf("  ОШИБКА: Не удалось создать модель!\n");
      all_passed = false;
      continue;
    }

    Size data_size = strlen(test_cases[i]);
    Result result =
      markov_model_process_data(model, (const Byte*)test_cases[i], data_size);
    if (result != RESULT_OK)
    {
      printf("  ОШИБКА: Не удалось обработать данные!\n");
      markov_model_destroy(model);
      all_passed = false;
      continue;
    }

    double actual_bits = markov_model_get_total_information_bits(model);
    const double tolerance = 0.1;

    // Детальная информация о тесте
    printf("  Ожидалось: %.1f бит\n", expected_bits[i]);
    printf("  Получено:  %.1f бит\n", actual_bits);
    printf("  Размер данных: %zu символов\n", data_size);
    printf("  Всего пар: %" PRIu64 "\n", markov_model_get_total_pairs(model));

    if (data_size > 0)
    {
      printf("  Первый символ: '%c' (0x%02X)\n", test_cases[i][0],
             test_cases[i][0]);
    }

    if (data_size > 1)
    {
      printf("  Пары в данных:\n");
      for (Size j = 0; j < data_size - 1; j++)
      {
        Byte first = test_cases[i][j];
        Byte second = test_cases[i][j + 1];
        uint64_t count = markov_model_get_pair_count(model, first, second);
        double prob =
          markov_model_get_conditional_probability(model, first, second);
        double info = markov_model_get_information(model, first, second);
        const int ascii_beginning = 32;
        const int ascii_end = 127;
        printf("    '%c%c': count=%" PRIu64 ", p=%.3f, I=%.2f бит\n",
               (first >= ascii_beginning && first < ascii_end) ? first : '.',
               (second >= ascii_beginning && second < ascii_end) ? second : '.',
               count, prob, info);
      }
    }

    if (fabs(actual_bits - expected_bits[i]) <= tolerance)
    {
      printf("  ✅ ТЕСТ ПРОЙДЕН\n");
    }
    else
    {
      printf("  ❌ ТЕСТ ПРОВАЛЕН (разница: %.2f)\n",
             fabs(actual_bits - expected_bits[i]));
      all_passed = false;
    }

    markov_model_destroy(model);
    printf("  ---\n");
  }

  printf("\nРезультат валидации:\n");
  if (all_passed)
  {
    printf("✅ Все тестовые сценарии пройдены!\n");
  }
  else
  {
    printf("❌ Есть провалившиеся тестовые сценарии!\n");
  }

  return all_passed;
}
