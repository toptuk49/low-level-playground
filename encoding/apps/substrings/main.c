#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "arguments.h"
#include "file.h"
#include "markov_model.h"

static void print_results(const MarkovModel* model, const char* filename,
                          Size file_size);
static void print_pair_table(const MarkovModel* model);
static void analyze_compression_efficiency(const MarkovModel* model,
                                           Size file_size);

int main(int argc, char** argv)
{
  ProgramArguments* args = program_arguments_create();
  if (!args)
  {
    printf("Ошибка при создании парсера аргументов командной строки!\n");
    return EXIT_FAILURE;
  }

  if (!program_arguments_parse(args, argc, argv))
  {
    program_arguments_destroy(args);
    return EXIT_FAILURE;
  }

  File* file = file_create(program_arguments_get_file_path(args));
  if (!file)
  {
    printf("Произошла ошибк при создании объекта файла!\n");
    program_arguments_destroy(args);
    return EXIT_FAILURE;
  }

  if (file_open(file) != RESULT_OK)
  {
    printf("Произошла ошибка при открытии файла: %s\n",
           program_arguments_get_file_path(args));
    file_destroy(file);
    program_arguments_destroy(args);
    return EXIT_FAILURE;
  }

  if (file_read_bytes(file) != RESULT_OK)
  {
    printf("Произошла ошибка при чтении файла!\n");
    file_close(file);
    file_destroy(file);
    program_arguments_destroy(args);
    return EXIT_FAILURE;
  }

  // Create and process Markov model
  MarkovModel* model = markov_model_create();
  if (!model)
  {
    printf("Произошла ошибка при создании источника Маркова!\n");
    file_close(file);
    file_destroy(file);
    program_arguments_destroy(args);
    return EXIT_FAILURE;
  }

  Result result = markov_model_process_data(model, file_get_buffer(file),
                                            file_get_size(file));
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при обработке данных источника Маркова!\n");
    markov_model_destroy(model);
    file_close(file);
    file_destroy(file);
    program_arguments_destroy(args);
    return EXIT_FAILURE;
  }

  print_results(model, program_arguments_get_file_path(args),
                file_get_size(file));

  analyze_compression_efficiency(model, file_get_size(file));

  printf("\nВалидация ---\n");
  if (markov_model_validate_test_cases())
  {
    printf("Все тестовые сценарии пройдены!\n");
  }
  else
  {
    printf("Некоторые тестовые сценарии провалились!\n");
  }

  markov_model_destroy(model);
  file_close(file);
  file_destroy(file);
  program_arguments_destroy(args);

  return EXIT_SUCCESS;
}

static void print_results(const MarkovModel* model, const char* filename,
                          Size file_size)
{
  printf("Анализ источника Маркова ---\n\n");
  printf("Файл: %s\n", filename);
  printf("Размер файла: %zu Байт\n", file_size);
  printf("Всего символов: %" PRIu64 "\n", file_size);
  printf("Всего двухсимвольных подстрок: %" PRIu64 "\n",
         markov_model_get_total_pairs(model));
  printf("I_CM1(Q)[бит] = %.2f\n",
         markov_model_get_total_information_bits(model));
  printf("I_CM1(Q)[Байт] = %.2f\n",
         markov_model_get_total_information_bytes(model));
  printf(
    "Количество информации в первом символе: %.2f бит (безусловная "
    "вероятность)\n",
    -log2(1.0 / ALPHABET_SIZE));
}

static void print_symbol_stats(const MarkovModel* model)
{
  printf("\nСтатистика по символам (первые 10 печатных ASCII):\n");
  printf("Символ | Общее count(a_j) | count(a_j*) | Разница\n");
  printf("-------|------------------|-------------|---------\n");

  int printed = 0;
  const int ascii_beginning = 32;
  const int ascii_end = 127;
  const int amount = 10;
  for (int i = ascii_beginning; i < ascii_end && printed < amount; i++)
  {  // Печатные ASCII
    uint64_t total_count = markov_model_get_symbol_count(model, (Byte)i);
    uint64_t prefix_count = markov_model_get_prefix_count(model, (Byte)i);

    if (total_count > 0)
    {
      printf("  '%c'   | %17" PRIu64 " | %11" PRIu64 " | %8" PRId64 "\n",
             (char)i, total_count, prefix_count,
             (int64_t)(total_count - prefix_count));
      printed++;
    }
  }
}

static void print_pair_table(const MarkovModel* model)
{
  printf("\nТаблица частот двухсимвольных подстрок первые 20) ---\n");
  printf(
    "Подстрока | Количество | Вероятность | Количество информации (бит)\n");
  printf(
    "----------|------------|-------------|----------------------------\n");

  typedef struct
  {
    Byte first;
    Byte second;
    uint64_t count;
    double probability;
    double information;
  } PairInfo;

  PairInfo pairs[ALPHABET_SIZE * ALPHABET_SIZE];
  int pair_count = 0;

  for (int first = 0; first < ALPHABET_SIZE; first++)
  {
    for (int second = 0; second < ALPHABET_SIZE; second++)
    {
      uint64_t count = markov_model_get_pair_count(model, first, second);
      if (count > 0)
      {
        pairs[pair_count].first = first;
        pairs[pair_count].second = second;
        pairs[pair_count].count = count;
        pairs[pair_count].probability =
          markov_model_get_conditional_probability(model, first, second);
        pairs[pair_count].information =
          markov_model_get_information(model, first, second);
        pair_count++;
      }
    }
  }

  // Сортируем по убыванию частоты
  for (int i = 0; i < pair_count - 1; i++)
  {
    for (int j = i + 1; j < pair_count; j++)
    {
      if (pairs[j].count > pairs[i].count)
      {
        PairInfo temp = pairs[i];
        pairs[i] = pairs[j];
        pairs[j] = temp;
      }
    }
  }

  const int ascii_beginning = 32;
  const int ascii_end = 127;
  const int amount = 20;
  int limit = pair_count < amount ? pair_count : amount;
  for (int i = 0; i < limit; i++)
  {
    unsigned char first_char =
      (pairs[i].first >= ascii_beginning && pairs[i].first < ascii_end)
        ? (unsigned char)pairs[i].first
        : '.';
    unsigned char second_char =
      (pairs[i].second >= ascii_beginning && pairs[i].second < ascii_end)
        ? (unsigned char)pairs[i].second
        : '.';

    printf("  %c%c     | %10" PRIu64 " | %10.6f | %14.2f\n", first_char,
           second_char, pairs[i].count, pairs[i].probability,
           pairs[i].information);
  }

  printf("\nВсего уникальных пар: %d\n", pair_count);
}

static void analyze_compression_efficiency(const MarkovModel* model,
                                           Size file_size)
{
  printf("\nАнализ сжатия ---\n\n");

  double information_bytes = markov_model_get_total_information_bytes(model);
  uint64_t compressed_size = (uint64_t)ceil(information_bytes);

  // Размер таблицы частот (256x256 пар, каждая по 4 байта)
  uint64_t non_zero_pairs = 0;
  for (int i = 0; i < ALPHABET_SIZE; i++)
  {
    for (int j = 0; j < ALPHABET_SIZE; j++)
    {
      if (markov_model_get_pair_count(model, i, j) > 0)
      {
        non_zero_pairs++;
      }
    }
  }
  uint64_t frequency_table_size =
    non_zero_pairs * (sizeof(Byte) + sizeof(Byte) + sizeof(uint32_t));
  uint64_t total_archive_size = compressed_size + frequency_table_size;

  printf("Размер сжатого текста: %" PRIu64 " Байт\n", compressed_size);
  printf("Размер таблицы частот: %" PRIu64 " Байт\n", frequency_table_size);
  printf("Итоговый размер архива: %" PRIu64 " Байт\n", total_archive_size);
  printf("Исходный размер текста: %zu Байт\n", file_size);
  printf("Степень сжатия: %.2f:1\n",
         (double)file_size / (double)total_archive_size);

  if (total_archive_size < file_size)
  {
    printf("Сжатие выгодно (сохраняет %zu Байт)\n",
           file_size - total_archive_size);
  }
  else
  {
    printf("Сжатие не выгодно (добавляет %" PRIu64 " Байт)\n",
           total_archive_size - file_size);
  }
}
