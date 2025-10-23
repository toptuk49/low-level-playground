#include "calculations.h"

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "cmocka.h"

static void test_sum_positive_numbers(void **state)
{
  (void)state;

  assert_int_equal(sum(2, 3), 5);
  assert_int_equal(sum(10, 20), 30);
}

static void test_sum_negative_numbers(void **state)
{
  (void)state;

  assert_int_equal(sum(-2, -3), -5);
  assert_int_equal(sum(5, -3), 2);
}

static const struct CMUnitTest calculations_tests[] = {
  cmocka_unit_test(test_sum_positive_numbers),
  cmocka_unit_test(test_sum_negative_numbers),
};

int main(void)
{
  return cmocka_run_group_tests(calculations_tests, NULL, NULL);
}
