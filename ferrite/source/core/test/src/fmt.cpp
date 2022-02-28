#include <core/fmt.hpp>

#include <string>
#include <sstream>

#include <gtest/gtest.h>

TEST(Fmt, string) {
    static_assert(is_display_v<std::string>);
}

TEST(Fmt, c_str) {
    static_assert(is_display_v<const char *>);
}
