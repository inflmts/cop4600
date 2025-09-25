#include "gtest/gtest.h"
#include "Divisibility.h"

// 1. A is positive, B is positive, and A does divide B. The function should return true.
TEST(Divisibility, PositiveDividesPositive) {
  ASSERT_EQ(isDivisible(5, 20), true);
}

// 2. A is positive, B is positive, and A does not divide B. The function should return false.
TEST(Divisibility, PositiveNotDividesPositive) {
  ASSERT_EQ(isDivisible(5, 21), false);
}

// 3. A is positive, B is zero. The function should return true.
TEST(Divisibility, PositiveDividesZero) {
  ASSERT_EQ(isDivisible(5, 0), true);
}

// 4. A is negative, B is positive, and A does divide B. The function should return true.
TEST(Divisibility, NegativeDividesPositive) {
  ASSERT_EQ(isDivisible(-5, 20), true);
}
