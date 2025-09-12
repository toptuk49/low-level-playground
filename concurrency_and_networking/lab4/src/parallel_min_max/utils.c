#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

void GenerateArray(int *array, unsigned long long array_size, unsigned long long seed) {
  srand(seed);
  for (unsigned long long i = 0; i < array_size; i++) {
    array[i] = rand();
  }
}
