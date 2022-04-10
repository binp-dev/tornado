#include <core/format.hpp>

#include <string>
#include <sstream>

#include <gtest/gtest.h>

TEST(Fmt, string) {
    static_assert(Printable<std::string>);
}

TEST(Fmt, c_str) {
    static_assert(Printable<const char *>);
}
