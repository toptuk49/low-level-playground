#ifndef ARGUMENTS_H
#define ARGUMENTS_H

#include <stdbool.h>

typedef struct {
    unsigned long long k;
    int pnum;
    unsigned long long mod;
} ProgramArguments;

bool parseArguments(int* argc, char ***argv, ProgramArguments *arguments);

#endif
