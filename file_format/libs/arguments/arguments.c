#include "arguments.h"

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"

struct ProgramArguments
{
  char* mode;
  char* input;
  char* output;
};

ProgramArguments* program_arguments_create(void)
{
  ProgramArguments* args = (ProgramArguments*)malloc(sizeof(ProgramArguments));
  if (args == NULL)
  {
    printf("Произошла ошибка при выделении памяти!\n");
    return NULL;
  }

  args->mode = NULL;
  args->input = NULL;
  args->output = NULL;

  return args;
}

void program_arguments_destroy(ProgramArguments* self)
{
  if (self == NULL)
  {
    return;
  }

  free(self->mode);
  free(self->input);
  free(self->output);
  free(self);
}

bool program_arguments_parse(ProgramArguments* self, int argc, char** argv)
{
  if (self == NULL)
  {
    return false;
  }

  int option_index = 0;
  static struct option long_options[] = {{"mode", required_argument, 0, 0},
                                         {"input", required_argument, 0, 0},
                                         {"output", required_argument, 0, 0},
                                         {0, 0, 0, 0}};

  optind = 1;  // Reset getopt

  while (true)
  {
    int current_option =
      getopt_long(argc, argv, "", long_options, &option_index);
    if (current_option == -1)
    {
      break;
    }

    if (current_option == 0)
    {
      switch (option_index)
      {
        case 0:  // --mode
          free(self->mode);

          self->mode = (char*)malloc(strlen(optarg) + 1);
          if (self->mode == NULL)
          {
            printf("Произошла ошибка выделения памяти для аргумента mode!\n");
            return false;
          }
          strcpy(self->mode, optarg);
          break;

        case 1:  // --input
          free(self->input);

          self->input = (char*)malloc(strlen(optarg) + 1);
          if (self->input == NULL)
          {
            printf("Произошла ошибка выделения памяти для аргумента input!\n");
            return false;
          }
          strcpy(self->input, optarg);
          break;

        case 2:  // --output
          free(self->output);

          self->output = (char*)malloc(strlen(optarg) + 1);
          if (self->output == NULL)
          {
            printf("Произошла ошибка выделения памяти для аргумента output!\n");
            return false;
          }
          strcpy(self->output, optarg);
          break;
        default:
          printf("Обнаружен неизвестный аргумент командной строки!\n");
          return false;
      }
    }
    else
    {
      printf("Введены некорректные аргументы командной строки!\n");
      return false;
    }
  }

  bool is_arguments_correct = true;

  if (!self->mode || strlen(self->mode) == 0)
  {
    printf("Ошибка: не указан обязательный аргумент --mode\n");
    is_arguments_correct = false;
  }

  if (!self->input || strlen(self->input) == 0)
  {
    printf("Ошибка: не указан обязательный аргумент --input\n");
    is_arguments_correct = false;
  }

  if (!self->output || strlen(self->output) == 0)
  {
    printf("Ошибка: не указан обязательный аргумент --output\n");
    is_arguments_correct = false;
  }

  if (self->mode != NULL && strcmp(self->mode, "encode") != 0 &&
      strcmp(self->mode, "decode") != 0 && strcmp(self->mode, "e") != 0 &&
      strcmp(self->mode, "d") != 0)
  {
    printf("Ошибка: недопустимое значение для --mode: %s\n", self->mode);
    is_arguments_correct = false;
  }

  return is_arguments_correct;
}

const char* program_arguments_get_mode(const ProgramArguments* self)
{
  return self ? self->mode : NULL;
}

const char* program_arguments_get_input(const ProgramArguments* self)
{
  return self ? self->input : NULL;
}

const char* program_arguments_get_output(const ProgramArguments* self)
{
  return self ? self->output : NULL;
}
