#include <core/fmt.hpp>

#include <gtest/gtest.h>


TEST(Io, display) {
    ASSERT_TRUE(Display<std::string>::value);
}
