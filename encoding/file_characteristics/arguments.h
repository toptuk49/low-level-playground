#ifndef ARGUMENTS_H
#define ARGUMENTS_H

#include <stdbool.h>

typedef struct {
  char *file;
} ProgramArguments;

bool parseArguments(int *argc, char ***argv, ProgramArguments *arguments);

#endif
