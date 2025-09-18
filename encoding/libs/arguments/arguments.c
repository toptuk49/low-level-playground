#include "arguments.h"

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"

struct ProgramArguments
{
  char* file_path;
};

ProgramArguments* program_arguments_create(void)
{
  ProgramArguments* args = (ProgramArguments*)malloc(sizeof(ProgramArguments));
  if (!args)
  {
    return NULL;
  }

  args->file_path = NULL;
  return args;
}

void program_arguments_destroy(ProgramArguments* self)
{
  if (!self)
  {
    return;
  }

  free(self->file_path);
  free(self);
}

bool program_arguments_parse(ProgramArguments* self, int argc, char** argv)
{
  if (!self)
  {
    return false;
  }

  int option_index = 0;
  static struct option long_options[] = {
    {"file", required_argument, 0, 0},
  };

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
        case 0:  // file
          free(self->file_path);
          self->file_path = strdup(optarg);
          if (!self->file_path)
          {
            printf("Произошла ошибка при выделении памяти!\n");
            return false;
          }
          break;
      }
    }
  }

  if (!self->file_path || strlen(self->file_path) == 0)
  {
    printf("Usage: %s --file <path>\n", argv[0]);
    return false;
  }

  return true;
}

const char* program_arguments_get_file_path(const ProgramArguments* self)
{
  return self ? self->file_path : NULL;
}
