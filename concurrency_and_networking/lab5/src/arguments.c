#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>

#include "arguments.h"

#define true 1
#define false 0
#define BASE 10

bool parseArguments(int* argc, char ***argv, ProgramArguments *arguments) {
    int option_index = 0;
    static struct option long_options[] = {
        {"k", required_argument, 0, 0},
        {"pnum", required_argument, 0, 0},
        {"mod", required_argument, 0, 0},
        {0, 0, 0, 0}
    };

    while (true) {
        int current_option = getopt_long(*argc, *argv, "", long_options, &option_index);
        if (current_option == -1) break;

        switch (current_option) {
            case 0:
                switch (option_index) {
                    case 0:
                        arguments->k = strtoull(optarg, NULL, BASE);
                        break;
                    case 1:
                        arguments->pnum = atoi(optarg);
                        break;
                    case 2:
                        arguments->mod = strtoull(optarg, NULL, BASE);
                        break;
                }
                break;
            default:
                printf("Invalid arguments\n");
                return false;
        }
    }

    if (arguments->k <= 0 || arguments->pnum <= 0 || arguments->mod <= 0) {
        printf("Usage: --k <num> --pnum <num> --mod <num>\n");
        return false;
    }

    return true;
}

