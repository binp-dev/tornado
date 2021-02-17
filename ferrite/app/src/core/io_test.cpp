#include <gtest/gtest.h>

#include "io.hpp"

TEST(Io, is_writable) {
    ASSERT_TRUE(is_writable<std::string>);
}
