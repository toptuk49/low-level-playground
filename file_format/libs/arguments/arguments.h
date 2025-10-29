#ifndef ARGUMENTS_ARGUMENTS_H
#define ARGUMENTS_ARGUMENTS_H

#include <stdbool.h>

typedef struct ProgramArguments ProgramArguments;

ProgramArguments* program_arguments_create(void);
void program_arguments_destroy(ProgramArguments* self);

bool program_arguments_parse(ProgramArguments* self, int argc, char** argv);
const char* program_arguments_get_mode(const ProgramArguments* self);
const char* program_arguments_get_input(const ProgramArguments* self);
const char* program_arguments_get_output(const ProgramArguments* self);

#endif  // ARGUMENTS_ARGUMENTS_H
