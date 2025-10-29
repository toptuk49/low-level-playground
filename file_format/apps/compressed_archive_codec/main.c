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
  const char* input_path = program_arguments_get_input(args);
  const char* output_path = program_arguments_get_output(args);

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
      printf("Создание сжатого архива\n%s", DELIMETER);
      result = compressed_archive_encode(input_path, output_path);
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

static void print_usage()
{
  printf(
    "Использование: compressed_archive_codec --mode <encode/decode> --input "
    "<path> --output <path>\n");
  printf("Режимы работы:\n");
  printf("  encode, e - создание сжатого архива из файла/папки\n");
  printf("  decode, d - извлечение файлов из сжатого архива\n");
  printf("\nПримеры:\n");
  printf(
    "  compressed_archive_codec --mode encode --input document.txt --output "
    "archive.compressed\n");
  printf(
    "  compressed_archive_codec --mode encode --input my_folder --output "
    "folder_archive.compressed\n");
  printf(
    "  compressed_archive_codec --mode decode --input archive.compressed "
    "--output extracted_file.txt\n");
}
