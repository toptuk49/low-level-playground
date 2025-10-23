#include "gtest/gtest.h"

extern "C"
{
#include "calculations.h"
}

TEST(CalculationsTest, sumPositiveNumbers)
{
  EXPECT_EQ(sum(2, 3), 5);
  EXPECT_EQ(sum(10, 20), 30);
  EXPECT_EQ(sum(100, 200), 300);
}

TEST(CalculationsTest, sumNegativeNumbers)
{
  EXPECT_EQ(sum(-2, -3), -5);
  EXPECT_EQ(sum(5, -3), 2);
  EXPECT_EQ(sum(-10, 15), 5);
}

class CalculationsAdvancedTest : public ::testing::Test
{
  protected:
  void SetUp() override
  {
  }

  void TearDown() override
  {
  }
};
