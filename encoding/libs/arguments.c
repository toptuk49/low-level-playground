#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arguments.h"

#define BASE 10
#define EQUAL 0

bool parseArguments(int *argc, char ***argv, ProgramArguments *arguments) {
  int option_index = 0;
  static struct option long_options[] = {{"file", required_argument, 0, 0},
                                         {0, 0, 0, 0}};

  while (true) {
    int current_option =
        getopt_long(*argc, *argv, "", long_options, &option_index);
    if (current_option == -1)
      break;

    switch (current_option) {
    case 0:
      switch (option_index) {
      case 0:
        arguments->file = (char *)malloc(strlen(optarg) + 1);
        if (arguments->file == NULL) {
          printf("Memory allocation error!\n");
          return 1;
        }

        strcpy(arguments->file, optarg);
        break;
      }
      break;
    default:
      printf("Invalid arguments\n");
      return false;
    }
  }

  if (arguments->file == NULL || strcmp(arguments->file, "") == EQUAL) {
    printf("Usage: --file <path>\n");
    return false;
  }

  return true;
}
