#include "arguments.h"
#include "file.h"

ProgramArguments args = {.file = NULL};
File file;

int main(int argc, char **argv) {
  if (!parseArguments(&argc, &argv, &args))
    return 1;

  openFile(&file, args.file);
  readBytes(&file);

  return 0;
}
