#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arguments.h"
#include "coder.h"
#include "decoder.h"
#include "types.h"

#define DELIMETER "---------------\n"

typedef enum
{
  MODE_ENCODE,
  MODE_DECODE,
  MODE_UNKNOWN
} OperationMode;

typedef enum
{
  ALGORITHM_AUTO,  // Автовыбор (по умолчанию)
  ALGORITHM_HUFFMAN,
  ALGORITHM_ARITHMETIC,
  ALGORITHM_SHANNON,
  ALGORITHM_NONE,
  ALGORITHM_UNKNOWN,
} CompressionAlgorithmChoice;

static OperationMode parse_operation_mode(const char* mode_str);
static CompressionAlgorithmChoice parse_algorithm(const char* algorithm);
static const char* algorithm_to_string(CompressionAlgorithmChoice algo);
static void print_usage();

int main(int argc, char** argv)
{
  ProgramArguments* args = program_arguments_create();
  if (args == NULL)
  {
    printf("Произошла ошибка при создании парсера аргументов!\n");
    return EXIT_FAILURE;
  }

  if (!program_arguments_parse(args, argc, argv))
  {
    print_usage();
    program_arguments_destroy(args);
    return EXIT_FAILURE;
  }

  const char* mode_argument = program_arguments_get_mode(args);
  const char* input_path = program_arguments_get_input(args);
  const char* output_path = program_arguments_get_output(args);
  const char* algorithm_argument = program_arguments_get_algorithm(args);

  OperationMode mode = parse_operation_mode(mode_argument);
  if (mode == MODE_UNKNOWN)
  {
    printf("Неизвестный режим работы '%s'!\n", mode_argument);
    print_usage();
    program_arguments_destroy(args);
    return EXIT_FAILURE;
  }

  CompressionAlgorithmChoice algorithm = ALGORITHM_AUTO;
  const char* algorithm_str = NULL;

  if (algorithm_argument)
  {
    algorithm = parse_algorithm(algorithm_argument);
    if (algorithm == ALGORITHM_UNKNOWN)
    {
      printf("Неизвестный алгоритм сжатия '%s'!\n", algorithm_argument);
      print_usage();
      program_arguments_destroy(args);
      return EXIT_FAILURE;
    }

    switch (algorithm)
    {
      case ALGORITHM_HUFFMAN:
        algorithm_str = "huffman";
        break;
      case ALGORITHM_ARITHMETIC:
        algorithm_str = "arithmetic";
        break;
      case ALGORITHM_SHANNON:
        algorithm_str = "shannon";
        break;
      case ALGORITHM_NONE:
        algorithm_str = "none";
        break;
      case ALGORITHM_AUTO:
      default:
        algorithm_str = NULL;  // Автоматический выбор
        break;
    }
  }

  Result result;

  switch (mode)
  {
    case MODE_ENCODE:
      printf("Создание сжатого архива\n%s", DELIMETER);
      printf("Алгоритм: %s\n", algorithm_to_string(algorithm));
      result = compressed_archive_encode_extended(input_path, output_path,
                                                  algorithm_str);
      break;

    case MODE_DECODE:
      printf("Извлечение из сжатого архива\n%s", DELIMETER);
      result = compressed_archive_decode(input_path, output_path);
      break;

    default:
      program_arguments_destroy(args);
      return EXIT_FAILURE;
  }

  program_arguments_destroy(args);

  if (result != RESULT_OK)
  {
    printf("Операция завершилась с ошибкой!\n");
    return EXIT_FAILURE;
  }

  printf("Операция завершена успешно!\n");
  return EXIT_SUCCESS;
}

static OperationMode parse_operation_mode(const char* mode_str)
{
  if (strcmp(mode_str, "encode") == 0 || strcmp(mode_str, "e") == 0)
  {
    return MODE_ENCODE;
  }

  if (strcmp(mode_str, "decode") == 0 || strcmp(mode_str, "d") == 0)
  {
    return MODE_DECODE;
  }

  return MODE_UNKNOWN;
}

static CompressionAlgorithmChoice parse_algorithm(const char* algorithm)
{
  if (!algorithm)
  {
    return ALGORITHM_AUTO;
  }

  if (strcmp(algorithm, "auto") == 0 || strcmp(algorithm, "a") == 0)
  {
    return ALGORITHM_AUTO;
  }

  if (strcmp(algorithm, "huffman") == 0 || strcmp(algorithm, "huff") == 0 ||
      strcmp(algorithm, "h") == 0)
  {
    return ALGORITHM_HUFFMAN;
  }

  if (strcmp(algorithm, "arithmetic") == 0 || strcmp(algorithm, "arith") == 0)
  {
    return ALGORITHM_ARITHMETIC;
  }

  if (strcmp(algorithm, "shannon") == 0 || strcmp(algorithm, "shan") == 0 ||
      strcmp(algorithm, "s") == 0)
  {
    return ALGORITHM_SHANNON;
  }

  if (strcmp(algorithm, "none") == 0 || strcmp(algorithm, "n") == 0)
  {
    return ALGORITHM_NONE;
  }

  return ALGORITHM_UNKNOWN;
}

static const char* algorithm_to_string(CompressionAlgorithmChoice algorithm)
{
  switch (algorithm)
  {
    case ALGORITHM_HUFFMAN:
      return "HUFFMAN";
    case ALGORITHM_ARITHMETIC:
      return "ARITHMETIC";
    case ALGORITHM_SHANNON:
      return "SHANNON";
    case ALGORITHM_NONE:
      return "NONE (без сжатия)";
    case ALGORITHM_AUTO:
    default:
      return "АВТОВЫБОР";
  }
}

static void print_usage()
{
  printf(
    "Использование: compressed_archive_codec --mode <encode/decode> --input "
    "<path> --output <path> [--algorithm <algorithm>]\n");
  printf("Режимы работы:\n");
  printf("  encode, e - создание сжатого архива из файла/папки\n");
  printf("  decode, d - извлечение файлов из сжатого архива\n");
  printf("\nАлгоритмы сжатия (только для encode):\n");
  printf("  auto, a     - автоматический выбор (по умолчанию)\n");
  printf("  huffman, huff, h  - алгоритм Хаффмана\n");
  printf("  arithmetic, arith - арифметическое кодирование\n");
  printf("  shannon, shan, s  - алгоритм Шеннона\n");
  printf("  none, n     - без сжатия\n");
  printf("\nПримеры:\n");
  printf(
    "  compressed_archive_codec --mode encode --algorithm huffman --input "
    "document.txt --output archive.compressed\n");
  printf(
    "  compressed_archive_codec --mode encode --algorithm arithmetic --input "
    "data.bin --output data.arith\n");
  printf(
    "  compressed_archive_codec --mode encode --algorithm shannon --input "
    "text.txt --output text.shannon\n");
  printf(
    "  compressed_archive_codec --mode decode --input archive.compressed "
    "--output extracted\n");
}
