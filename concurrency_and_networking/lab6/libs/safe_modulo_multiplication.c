#include "safe_modulo_multiplication.h"

#include <inttypes.h>

uint64_t SafeModuloMultiplication(uint64_t left, uint64_t right, uint64_t mod)
{
  uint64_t result = 0;
  left = left % mod;
  while (right > 0)
  {
    if (right % 2 == 1)
    {
      result = (result + left) % mod;
    }
    left = (left * 2) % mod;
    right /= 2;
  }

  return result % mod;
}
