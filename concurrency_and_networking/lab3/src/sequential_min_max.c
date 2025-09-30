#include <stdio.h>
#include <stdlib.h>

#include "../libs/find_min_max.h"
#include "../libs/utils.h"

#define BASE 10

int main(int argc, char **argv)
{
  if (argc != 3)
  {
    printf("Usage: %s seed arraysize\n", argv[0]);
    return 1;
  }

  int seed = (int)strtol(argv[1], NULL, BASE);
  if (seed <= 0)
  {
    printf("seed is a positive number\n");
    return 1;
  }

  int array_size = (int)strtol(argv[2], NULL, BASE);
  if (array_size <= 0)
  {
    printf("array_size is a positive number\n");
    return 1;
  }

  int *array = malloc(array_size * sizeof(int));
  GenerateArray(array, array_size, seed);
  struct MinMax min_max = GetMinMax(array, 0, array_size);
  free(array);

  printf("min: %d\n", min_max.min);
  printf("max: %d\n", min_max.max);

  return 0;
}
