#include "find_min_max.h"

#include <limits.h>

struct MinMax GetMinMax(int *array, unsigned int begin, unsigned int end)
{
  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (unsigned int i = begin; i < end; i++)
  {
    int currentNumber = array[i];

    if (currentNumber > min_max.max)
    {
      min_max.max = currentNumber;
    }

    if (currentNumber < min_max.min)
    {
      min_max.min = currentNumber;
    }
  }

  return min_max;
}
