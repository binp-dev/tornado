#include <core/format.hpp>

#include <sstream>

#include <gtest/gtest.h>

TEST(Format, empty) {
    std::stringstream ss;
    constexpr Literal LIT("");
    format_impl<LIT>(ss);
    ASSERT_EQ(ss.str(), "");
}
