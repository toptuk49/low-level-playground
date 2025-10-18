#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arguments.h"
#include "coder.h"
#include "decoder.h"

#define DELIMETER "---------------\n"

typedef enum
{
  MODE_ENCODE,
  MODE_DECODE,
  MODE_UNKNOWN
} OperationMode;

static OperationMode parse_operation_mode(const char* mode_str);
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
  const char* input_file = program_arguments_get_input(args);
  const char* output_file = program_arguments_get_output(args);

  OperationMode mode = parse_operation_mode(mode_argument);
  if (mode == MODE_UNKNOWN)
  {
    printf("Неизвестный режим работы '%s'!\n", mode_argument);
    print_usage();
    program_arguments_destroy(args);
    return EXIT_FAILURE;
  }

  Result result;

  switch (mode)
  {
    case MODE_ENCODE:
      printf("Кодирование файла в архив\n%s", DELIMETER);
      result = raw_archive_encode(input_file, output_file);
      break;

    case MODE_DECODE:
      printf("Декодирование архива в файл\n%s", DELIMETER);
      result = raw_archive_decode(input_file, output_file);
      break;

    default:
      program_arguments_destroy(args);
      return EXIT_FAILURE;
  }

  if (result != RESULT_OK)
  {
    printf("Операция завершилась с ошибкой!\n");
    return EXIT_FAILURE;
  }

  printf("Операция завершена успешно!\n");

  program_arguments_destroy(args);

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

static void print_usage()
{
  printf(
    "Использование: raw_archive_codec --mode <encode/decode> --input <path> "
    "--output "
    "<path>\n");
  printf("Режимы работы:\n");
  printf("  encode, e - кодирование файла в архив\n");
  printf("  decode, d - декодирование архива в файл\n");
  printf("\nПримеры:\n");
  printf("  raw_archive_codec encode input.txt output.lolkek \n");
  printf("  raw_archive_codec decode input.lolkek output.txt\n");
}
