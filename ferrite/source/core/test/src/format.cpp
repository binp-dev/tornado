#include <core/format.hpp>

#include <sstream>

#include <gtest/gtest.h>

TEST(Format, empty) {
    ASSERT_EQ(format(""), "");
}

TEST(Format, text) {
    ASSERT_EQ(format("abc"), "abc");
}

TEST(Format, one_arg) {
    ASSERT_EQ(format("a{}", 1), "a1");
}

TEST(Format, two_args) {
    ASSERT_EQ(format("a{} {}2", 1, "b"), "a1 b2");
}

TEST(Format, escape) {
    ASSERT_EQ(format("}}{{"), "}{");
}
