#include <core/numeric.hpp>

#include <gtest/gtest.h>

#include <cstdint>

using namespace core;

TEST(Numeric, next_power_of_two) {
    ASSERT_EQ(next_power_of_two(1), 1);
    ASSERT_EQ(next_power_of_two(2), 2);
    ASSERT_EQ(next_power_of_two(3), 4);
    ASSERT_EQ(next_power_of_two(129), 256);
    ASSERT_EQ(next_power_of_two(1610612736u), 2147483648u);
}
