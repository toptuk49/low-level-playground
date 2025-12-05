#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "analyzer.h"

static void print_usage()
{
  printf("Использование:\n");
  printf(
    "  huffman_analyzer --file <filename>           # Анализ одного файла\n");
  printf(
    "  huffman_analyzer --plot <file1> <file2> ...  # Генерация данных для "
    "графиков\n");
  printf("\nПримеры:\n");
  printf("  huffman_analyzer --file test.txt\n");
  printf(
    "  huffman_analyzer --plot file1.txt file2.bin file3.jpg > "
    "huffman_data.csv\n");
}

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    print_usage();
    return EXIT_FAILURE;
  }

  if (strcmp(argv[1], "--file") == 0 && argc >= 3)
  {
    analyze_file_all_bit_depths(argv[2]);
  }
  else if (strcmp(argv[1], "--plot") == 0 && argc >= 3)
  {
    const char** files = (const char**)&argv[2];
    create_plots_for_files(files, argc - 2);
  }
  else
  {
    print_usage();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
