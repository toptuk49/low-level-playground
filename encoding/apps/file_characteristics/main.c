#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "arguments.h"
#include "file.h"
#include "statistics.h"
#include "summary.h"
#include "types.h"

int main(int argc, char **argv) {
  ProgramArguments *args = program_arguments_create();
  if (!args) {
    printf("Failed to create arguments parser\n");
    return EXIT_FAILURE;
  }

  if (!program_arguments_parse(args, argc, argv)) {
    program_arguments_destroy(args);
    return EXIT_FAILURE;
  }

  File *file = file_create(program_arguments_get_file_path(args));
  if (!file) {
    printf("Failed to create file object\n");
    program_arguments_destroy(args);
    return EXIT_FAILURE;
  }

  if (file_open(file) != RESULT_OK) {
    printf("Failed to open file: %s\n", program_arguments_get_file_path(args));
    file_destroy(file);
    program_arguments_destroy(args);
    return EXIT_FAILURE;
  }

  if (file_read_bytes(file) != RESULT_OK) {
    printf("Failed to read file\n");
    file_close(file);
    file_destroy(file);
    program_arguments_destroy(args);
    return EXIT_FAILURE;
  }

  Statistics *stats = statistics_create();
  if (!stats) {
    printf("Failed to create statistics\n");
    file_close(file);
    file_destroy(file);
    program_arguments_destroy(args);
    return EXIT_FAILURE;
  }

  if (statistics_calculate(stats, file_get_buffer(file), file_get_size(file)) !=
      RESULT_OK) {
    printf("Failed to calculate statistics\n");
    statistics_destroy(stats);
    file_close(file);
    file_destroy(file);
    program_arguments_destroy(args);
    return EXIT_FAILURE;
  }

  Summary *summary = summary_create();
  if (!summary) {
    printf("Failed to create summary\n");
    statistics_destroy(stats);
    file_close(file);
    file_destroy(file);
    program_arguments_destroy(args);
    return EXIT_FAILURE;
  }

  if (summary_calculate(summary, stats) != RESULT_OK) {
    printf("Failed to calculate summary\n");
    summary_destroy(summary);
    statistics_destroy(stats);
    file_close(file);
    file_destroy(file);
    program_arguments_destroy(args);
    return EXIT_FAILURE;
  }

  const Byte byte = 8;
  printf("Файл: %s\n", program_arguments_get_file_path(args));
  printf("Длина файла n = %ld байт\n", statistics_get_byte_length(stats));
  printf("L(Q)[бит] = %ld\n", statistics_get_byte_length(stats) * byte);
  printf("I(Q)[бит] = %.2f\n", summary_get_information_bits(summary));
  printf("Дробная часть I(Q)[бит] = %.2e\n",
         fmod(summary_get_information_bits(summary), 1.0));
  printf("L(Q)[октетов] = %ld\n", statistics_get_byte_length(stats));
  printf("I(Q)[октетов] = %.2f\n", summary_get_information_bytes(summary));
  printf("E (нижняя граница) = %ld\n",
         summary_get_compressed_min_size(summary));
  printf("G64 = %ld\n", summary_get_archive_length_non_standard(summary));
  printf("G8 = %ldn\n", summary_get_archive_length_normalized(summary));

  summary_destroy(summary);
  statistics_destroy(stats);
  file_close(file);
  file_destroy(file);
  program_arguments_destroy(args);

  return EXIT_SUCCESS;
}
