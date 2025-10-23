#include "calculations.h"

#include "unity/unity.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_sum_positive_numbers(void)
{
  TEST_ASSERT_EQUAL(5, sum(2, 3));
}

void test_sum_negative_numbers(void)
{
  TEST_ASSERT_EQUAL(-1, sum(2, -3));
}

int main(void)
{
  UNITY_BEGIN();

  RUN_TEST(test_sum_positive_numbers);
  RUN_TEST(test_sum_negative_numbers);

  return UNITY_END();
}
