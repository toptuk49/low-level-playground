#include "string_converter.h"

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define BASE 10

bool ConvertStringToUI64(const char *input, uint64_t *value_to_convert)
{
  char *end = NULL;
  unsigned long long converted_value = strtoull(input, &end, BASE);
  if (errno == ERANGE)
  {
    printf("Out of uint64_t range: %s\n", input);
    return false;
  }

  if (errno != 0)
  {
    return false;
  }

  *value_to_convert = converted_value;
  return true;
}
