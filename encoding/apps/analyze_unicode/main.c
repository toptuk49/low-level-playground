#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "arguments.h"
#include "file.h"
#include "unicode.h"

typedef struct
{
  double information_bits;
  double information_bytes;
  uint64_t compressed_min_size;
  uint64_t table_size_utf8;
  uint64_t table_size_utf32;
  uint64_t archive_size_utf8;
  uint64_t archive_size_utf32;
} UnicodeAnalysisResult;

static UnicodeAnalysisResult* analyze_unicode_calculate(
  const UnicodeStatistics* stats, uint64_t total_symbols);
static void analyze_unicode_print_results(const UnicodeAnalysisResult* result,
                                          const UnicodeStatistics* stats,
                                          uint64_t total_symbols,
                                          const char* filename);

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

  UnicodeStatistics* stats = unicode_statistics_create();
  if (!stats)
  {
    printf("Произошла ошибка при создании объекта статистики!\n");
    file_close(file);
    file_destroy(file);
    program_arguments_destroy(args);
    return EXIT_FAILURE;
  }

  Result result = unicode_statistics_process_data(stats, file_get_buffer(file),
                                                  file_get_size(file));
  if (result != RESULT_OK)
  {
    printf("Произошла ошибка при вычислении статистики!\n");
    unicode_statistics_destroy(stats);
    file_close(file);
    file_destroy(file);
    program_arguments_destroy(args);
    return EXIT_FAILURE;
  }

  uint64_t total_symbols = 0;
  const Symbol* symbols = unicode_statistics_get_symbols(stats);
  for (size_t i = 0; i < unicode_statistics_get_unique_count(stats); i++)
  {
    total_symbols += symbols[i].count;
  }

  UnicodeAnalysisResult* analysis =
    analyze_unicode_calculate(stats, total_symbols);
  if (!analysis)
  {
    printf("Произошла ошибка при вычислении результатов!\n");
    unicode_statistics_destroy(stats);
    file_close(file);
    file_destroy(file);
    program_arguments_destroy(args);
    return EXIT_FAILURE;
  }

  analyze_unicode_print_results(analysis, stats, total_symbols,
                                program_arguments_get_file_path(args));

  free(analysis);
  unicode_statistics_destroy(stats);
  file_close(file);
  file_destroy(file);
  program_arguments_destroy(args);

  return EXIT_SUCCESS;
}

static UnicodeAnalysisResult* analyze_unicode_calculate(
  const UnicodeStatistics* stats, uint64_t total_symbols)
{
  if (!stats || total_symbols == 0)
  {
    return NULL;
  }

  UnicodeAnalysisResult* result =
    (UnicodeAnalysisResult*)malloc(sizeof(UnicodeAnalysisResult));
  if (!result)
  {
    return NULL;
  }

  result->information_bits = 0.0;
  const Symbol* symbols = unicode_statistics_get_symbols(stats);
  size_t unique_count = unicode_statistics_get_unique_count(stats);

  for (size_t i = 0; i < unique_count; i++)
  {
    double probability = (double)symbols[i].count / (double)total_symbols;
    double information = -log2(probability);
    result->information_bits += (double)symbols[i].count * information;
  }

  const Byte bytes = 8;
  result->information_bytes = result->information_bits / (double)(bytes);
  result->compressed_min_size = (uint64_t)ceil(result->information_bytes);

  result->table_size_utf8 = bytes;   // |A1| 64-bit
  result->table_size_utf32 = bytes;  // |A1| 64-bit

  for (size_t i = 0; i < unique_count; i++)
  {
    int symbol_length = unicode_get_symbol_length(symbols[i].codepoint);
    result->table_size_utf8 +=
      symbol_length + bytes;  // symbol + 64-bit frequency
    result->table_size_utf32 +=
      4 + bytes;  // UTF-32 symbol (4 bytes) + 64-bit frequency
  }

  result->archive_size_utf8 =
    result->compressed_min_size + result->table_size_utf8;
  result->archive_size_utf32 =
    result->compressed_min_size + result->table_size_utf32;

  return result;
}

static void analyze_unicode_print_results(const UnicodeAnalysisResult* result,
                                          const UnicodeStatistics* stats,
                                          uint64_t total_symbols,
                                          const char* filename)
{
  if (!result || !stats || !filename)
  {
    return;
  }

  const Symbol* symbols = unicode_statistics_get_symbols(stats);
  size_t unique_count = unicode_statistics_get_unique_count(stats);

  printf("Файл: %s\n", filename);
  printf("Символов Unicode: %" PRIu64 "\n", total_symbols);
  printf("Уникальных символов: %zu\n", unique_count);
  printf("I(Q)[бит] = %.2f\n", result->information_bits);
  printf("I(Q)[октетов] = %.2f\n", result->information_bytes);
  printf("E (нижняя граница) = %" PRIu64 "\n", result->compressed_min_size);
  printf("Размер таблицы UTF-8 +64bit freq = %" PRIu64 " байт\n",
         result->table_size_utf8);
  printf("Размер таблицы UTF-32 +64bit freq = %" PRIu64 " байт\n",
         result->table_size_utf32);
  printf("G(UTF-8) = E + table = %" PRIu64 "\n", result->archive_size_utf8);
  printf("G(UTF-32) = E + table = %" PRIu64 "\n", result->archive_size_utf32);

  printf("\nТаблица символов по коду:\n");
  printf(
    "Код символа | Количество | Вероятность | Информация (бит) | Длина "
    "(байт)\n");
  printf(
    "------------|------------|-------------|------------------|-------------"
    "\n");

  for (size_t i = 0; i < unique_count; i++)
  {
    double probability = (double)symbols[i].count / (double)total_symbols;
    double information = -log2(probability);
    int symbol_length = unicode_get_symbol_length(symbols[i].codepoint);

    printf("U+%04X | %10" PRIu64 " | %10.6f | %14.4f | %11d\n",
           symbols[i].codepoint, symbols[i].count, probability, information,
           symbol_length);
  }
}
