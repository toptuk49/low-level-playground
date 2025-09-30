#ifndef SERVER_ARGUMENTS_H
#define SERVER_ARGUMENTS_H

#include <inttypes.h>
#include <stdbool.h>

typedef struct {
  int tnum;
  int port;
  uint64_t mod;
} ProgramArguments;

bool parseArguments(int *argc, char ***argv, ProgramArguments *arguments);

#endif
