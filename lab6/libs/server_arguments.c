#include "server_arguments.h"

#include <getopt.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define BASE 10

bool parseArguments(int *argc, char ***argv, ProgramArguments *arguments)
{
  while (true)
  {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"port", required_argument, 0, 0},
                                      {"tnum", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int option_char = getopt_long(*argc, *argv, "", options, &option_index);

    if (option_char == -1)
    {
      break;
    }

    switch (option_char)
    {
      case 0:
      {
        switch (option_index)
        {
          case 0:
            arguments->port = (int)strtol(optarg, NULL, BASE);
            break;
          case 1:
            arguments->tnum = (int)strtol(optarg, NULL, BASE);
            break;
          default:
            printf("Index %d is out of options\n", option_index);
        }
      }
      break;

      case '?':
        printf("Unknown argument\n");
        break;
      default:
        printf("getopt returned character code 0%o?\n", option_char);
    }
  }

  if (arguments->port == -1 || arguments->tnum == -1)
  {
    printf("Using: %s --port <num> --tnum <num>\n", *argv[0]);
    return false;
  }

  return true;
}
