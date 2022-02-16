#include <core/io.hpp>

#include <gtest/gtest.h>


TEST(Io, is_writable) {
    ASSERT_TRUE(is_writable<std::string>);
}
