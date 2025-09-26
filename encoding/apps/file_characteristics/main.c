#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arguments.h"
#include "file.h"
#include "statistics.h"
#include "summary.h"
#include "types.h"

int main(int argc, char** argv)
{
  ProgramArguments* args = program_arguments_create();
  if (!args)
  {
    printf("Произошла ошибка при создании парсера аргументов!\n");
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
    printf("Произошла ошибка при создании объекта файла!\n");
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

  Statistics* stats = statistics_create();
  if (!stats)
  {
    printf("Произошла ошибка при создании объекта статистики!\n");
    file_close(file);
    file_destroy(file);
    program_arguments_destroy(args);
    return EXIT_FAILURE;
  }

  if (statistics_calculate(stats, file_get_buffer(file), file_get_size(file)) !=
      RESULT_OK)
  {
    printf("Произошла ошибка при вычислении статистики!\n");
    statistics_destroy(stats);
    file_close(file);
    file_destroy(file);
    program_arguments_destroy(args);
    return EXIT_FAILURE;
  }

  Summary* summary = summary_create();
  if (!summary)
  {
    printf("Произошла ошибка при создании отчета!\n");
    statistics_destroy(stats);
    file_close(file);
    file_destroy(file);
    program_arguments_destroy(args);
    return EXIT_FAILURE;
  }

  if (summary_calculate(summary, stats) != RESULT_OK)
  {
    printf("Произошла ошибка при вычислении отчета!\n");
    summary_destroy(summary);
    statistics_destroy(stats);
    file_close(file);
    file_destroy(file);
    program_arguments_destroy(args);
    return EXIT_FAILURE;
  }

  const Byte byte = 8;
  printf("Файл: %s\n", program_arguments_get_file_path(args));
  printf("Длина файла n = %ld [байт]\n", statistics_get_byte_length(stats));
  printf("L(Q) = %ld [бит]\n", statistics_get_byte_length(stats) * byte);
  printf("I(Q) = %.2f [бит]\n", summary_get_information_bits(summary));
  printf("Дробная часть I(Q) = %.2e [бит]\n",
         fmod(summary_get_information_bits(summary), 1.0));
  printf("L(Q) = %ld [октетов]\n", statistics_get_byte_length(stats));
  printf("I(Q) = %.2f [октетов]\n", summary_get_information_bytes(summary));
  printf("E (нижняя граница) = %ld\n",
         summary_get_compressed_min_size(summary));
  printf("G64 = %" PRIu64 " [октетов]\n",
         summary_get_archive_length_non_standard(summary));
  printf("G8 = %" PRIu64 " [октетов]\n",
         summary_get_archive_length_normalized(summary));

  printf("Таблица (по алфавиту):\n");
  printf("Байт | Количество | Вероятность байта | Информация в байте\n");
  for (int i = 0; i < ALPHABET_SIZE; i++)
  {
    if (statistics_get_byte_count(stats, i) == 0)
    {
      continue;
    }

    printf("%02X | %9ld | %11.6f | %8.4f\n", i,
           statistics_get_byte_count(stats, i),
           statistics_get_byte_probability(stats, i),
           statistics_get_byte_information(stats, i));
  }

  printf("\nТаблица (по убыванию количества символов):\n");
  printf("Байт | Количество | Вероятность байта | Информация в байте\n");

  int sorted_indices[ALPHABET_SIZE];
  statistics_get_sorted_indices(stats, sorted_indices);

  for (int i = 0; i < ALPHABET_SIZE; i++)
  {
    int index = sorted_indices[i];
    if (statistics_get_byte_count(stats, index) == 0)
    {
      continue;
    }

    printf("%02X   | %9ld | %11.6f | %8.4f\n", index,
           statistics_get_byte_count(stats, index),
           statistics_get_byte_probability(stats, index),
           statistics_get_byte_information(stats, index));
  }

  summary_destroy(summary);
  statistics_destroy(stats);
  file_close(file);
  file_destroy(file);
  program_arguments_destroy(args);

  return EXIT_SUCCESS;
}
