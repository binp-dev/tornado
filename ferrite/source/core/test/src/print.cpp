#include <core/print.hpp>

#include <string>

#include <gtest/gtest.h>

TEST(Print, string) {
    static_assert(Printable<std::string>);
}

TEST(Print, c_str) {
    static_assert(Printable<const char *>);
}
