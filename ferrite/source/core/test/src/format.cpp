#include <core/print.hpp>
#include <core/format.hpp>
#include <core/log.hpp>

#include <string>
#include <sstream>

#include <gtest/gtest.h>

TEST(Print, string) {
    static_assert(Printable<std::string>);
}

TEST(Print, c_str) {
    static_assert(Printable<const char *>);
}

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

TEST(Log, info) {
    core_log_info("test {}", 123);
}
