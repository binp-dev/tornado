#include "cast.hpp"

#ifdef UNITTEST

#include <gtest/gtest.h>

#include <cstdint>

TEST(Cast, i8_to_i32) {
    ASSERT_TRUE(safe_cast<int32_t>(int8_t(0)) == int32_t(0));
    ASSERT_TRUE(safe_cast<int32_t>(int8_t(0x7f)) == int32_t(0x7f));
    ASSERT_TRUE(safe_cast<int32_t>(int8_t(-0x80)) == int32_t(-0x80));
}

TEST(Cast, u8_to_u32) {
    ASSERT_TRUE(safe_cast<uint32_t>(uint8_t(0)) == uint32_t(0));
    ASSERT_TRUE(safe_cast<uint32_t>(uint8_t(0xff)) == uint32_t(0xff));
}

TEST(Cast, i32_to_i8) {
    ASSERT_TRUE(safe_cast<int8_t>(int32_t(0)) == int8_t(0));
    ASSERT_TRUE(safe_cast<int8_t>(int32_t(0x7f)) == int8_t(0x7f));
    ASSERT_TRUE(safe_cast<int8_t>(int32_t(-0x80)) == int8_t(-0x80));
    ASSERT_TRUE(safe_cast<int8_t>(int32_t(0x80)) == std::nullopt);
    ASSERT_TRUE(safe_cast<int8_t>(int32_t(-0x81)) == std::nullopt);
    ASSERT_TRUE(safe_cast<int8_t>(int32_t(0x7fffffff)) == std::nullopt);
    ASSERT_TRUE(safe_cast<int8_t>(int32_t(-0x80000000)) == std::nullopt);
}

TEST(Cast, u32_to_u8) {
    ASSERT_TRUE(safe_cast<uint8_t>(uint32_t(0)) == uint8_t(0));
    ASSERT_TRUE(safe_cast<uint8_t>(uint32_t(0xff)) == uint8_t(0xff));
    ASSERT_TRUE(safe_cast<uint8_t>(uint32_t(0x100)) == std::nullopt);
    ASSERT_TRUE(safe_cast<uint8_t>(uint32_t(0xffffffff)) == std::nullopt);
}

TEST(Cast, u8_to_i32) {
    ASSERT_TRUE(safe_cast<int32_t>(uint8_t(0)) == int32_t(0));
    ASSERT_TRUE(safe_cast<int32_t>(uint8_t(0xff)) == int32_t(0xff));
}

TEST(Cast, i8_to_u32) {
    ASSERT_TRUE(safe_cast<uint32_t>(int8_t(0)) == uint32_t(0));
    ASSERT_TRUE(safe_cast<uint32_t>(int8_t(0x7f)) == uint32_t(0x7f));
    ASSERT_TRUE(safe_cast<uint32_t>(int8_t(-0x80)) == std::nullopt);
    ASSERT_TRUE(safe_cast<uint32_t>(int8_t(-1)) == std::nullopt);
}

TEST(Cast, u32_to_i8) {
    ASSERT_TRUE(safe_cast<int8_t>(uint32_t(0)) == int8_t(0));
    ASSERT_TRUE(safe_cast<int8_t>(uint32_t(0x7f)) == int8_t(0x7f));
    ASSERT_TRUE(safe_cast<int8_t>(uint32_t(0x80)) == std::nullopt);
    ASSERT_TRUE(safe_cast<int8_t>(uint32_t(0xffffffff)) == std::nullopt);
}

TEST(Cast, i32_to_u8) {
    ASSERT_TRUE(safe_cast<uint8_t>(int32_t(0)) == uint8_t(0));
    ASSERT_TRUE(safe_cast<uint8_t>(int32_t(0xff)) == uint8_t(0xff));
    ASSERT_TRUE(safe_cast<uint8_t>(int32_t(-0x80)) == std::nullopt);
    ASSERT_TRUE(safe_cast<uint8_t>(int32_t(0x7fffffff)) == std::nullopt);
    ASSERT_TRUE(safe_cast<uint8_t>(int32_t(-0x80000000)) == std::nullopt);
    ASSERT_TRUE(safe_cast<uint8_t>(int32_t(-1)) == std::nullopt);
}


#endif // UNITTEST
