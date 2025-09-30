#ifndef CLIENT_ARGUMENTS_H
#define CLIENT_ARGUMENTS_H

#include <inttypes.h>
#include <stdbool.h>

typedef struct {
  uint64_t k;
  uint64_t mod;
  char servers;
} ProgramArguments;

bool parseArguments(int *argc, char ***argv, ProgramArguments *arguments);

#endif
