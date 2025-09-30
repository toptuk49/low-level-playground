#include "client_arguments.h"

#include <getopt.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "string_converter.h"

bool parseArguments(int *argc, char ***argv, ProgramArguments *arguments)
{
  while (true)
  {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
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
            ConvertStringToUI64(optarg, &arguments->k);
            break;
          case 1:
            ConvertStringToUI64(optarg, &arguments->mod);
            break;
          case 2:
            memcpy(&arguments->servers, optarg, strlen(optarg));
            break;
          default:
            printf("Index %d is out of options\n", option_index);
        }
      }
      break;

      case '?':
        printf("Arguments error\n");
        break;
      default:
        printf("getopt returned character code 0%o?\n", option_char);
    }
  }

  if (arguments->k == -1 || arguments->mod == -1 ||
      !strlen(&arguments->servers))
  {
    printf("Using: %s --k <num> --mod <num> --servers /path/to/file\n",
           *argv[0]);
    return false;
  }

  return true;
}
