#include <core/convert.hpp>

#include <gtest/gtest.h>

#include <cstdint>

using namespace core;

TEST(Convert, i8_to_i32) {
    ASSERT_EQ(cast_int<int32_t>(int8_t(0)), Some<int32_t>(0));
    ASSERT_EQ(cast_int<int32_t>(int8_t(0x7f)), Some<int32_t>(0x7f));
    ASSERT_EQ(cast_int<int32_t>(int8_t(-0x80)), Some<int32_t>(-0x80));
}

TEST(Convert, u8_to_u32) {
    ASSERT_EQ(cast_int<uint32_t>(uint8_t(0)), Some<uint32_t>(0));
    ASSERT_EQ(cast_int<uint32_t>(uint8_t(0xff)), Some<uint32_t>(0xff));
}

TEST(Convert, i32_to_i8) {
    ASSERT_EQ(cast_int<int8_t>(int32_t(0)), Some<int8_t>(0));
    ASSERT_EQ(cast_int<int8_t>(int32_t(0x7f)), Some<int8_t>(0x7f));
    ASSERT_EQ(cast_int<int8_t>(int32_t(-0x80)), Some<int8_t>(-0x80));
    ASSERT_EQ(cast_int<int8_t>(int32_t(0x80)), None());
    ASSERT_EQ(cast_int<int8_t>(int32_t(-0x81)), None());
    ASSERT_EQ(cast_int<int8_t>(int32_t(0x7fffffff)), None());
    ASSERT_EQ(cast_int<int8_t>(int32_t(-0x80000000)), None());
}

TEST(Convert, u32_to_u8) {
    ASSERT_EQ(cast_int<uint8_t>(uint32_t(0)), Some<uint8_t>(0));
    ASSERT_EQ(cast_int<uint8_t>(uint32_t(0xff)), Some<uint8_t>(0xff));
    ASSERT_EQ(cast_int<uint8_t>(uint32_t(0x100)), None());
    ASSERT_EQ(cast_int<uint8_t>(uint32_t(0xffffffff)), None());
}

TEST(Convert, u8_to_i32) {
    ASSERT_EQ(cast_int<int32_t>(uint8_t(0)), Some<int32_t>(0));
    ASSERT_EQ(cast_int<int32_t>(uint8_t(0xff)), Some<int32_t>(0xff));
}

TEST(Convert, i8_to_u32) {
    ASSERT_EQ(cast_int<uint32_t>(int8_t(0)), Some<uint32_t>(0));
    ASSERT_EQ(cast_int<uint32_t>(int8_t(0x7f)), Some<uint32_t>(0x7f));
    ASSERT_EQ(cast_int<uint32_t>(int8_t(-0x80)), None());
    ASSERT_EQ(cast_int<uint32_t>(int8_t(-1)), None());
}

TEST(Convert, u32_to_i8) {
    ASSERT_EQ(cast_int<int8_t>(uint32_t(0)), Some<int8_t>(0));
    ASSERT_EQ(cast_int<int8_t>(uint32_t(0x7f)), Some<int8_t>(0x7f));
    ASSERT_EQ(cast_int<int8_t>(uint32_t(0x80)), None());
    ASSERT_EQ(cast_int<int8_t>(uint32_t(0xffffffff)), None());
}

TEST(Convert, i32_to_u8) {
    ASSERT_EQ(cast_int<uint8_t>(int32_t(0)), Some<uint8_t>(0));
    ASSERT_EQ(cast_int<uint8_t>(int32_t(0xff)), Some<uint8_t>(0xff));
    ASSERT_EQ(cast_int<uint8_t>(int32_t(-0x80)), None());
    ASSERT_EQ(cast_int<uint8_t>(int32_t(0x7fffffff)), None());
    ASSERT_EQ(cast_int<uint8_t>(int32_t(-0x80000000)), None());
    ASSERT_EQ(cast_int<uint8_t>(int32_t(-1)), None());
}

class A {
public:
    virtual ~A() = default;
};
class B : public A {
public:
    using A::A;
};
class C : public A {
public:
    using A::A;
};

TEST(Convert, downcast) {
    const B b;
    const C c;
    const A &ab = b;
    const A &ac = c;
    ASSERT_TRUE(downcast<const B>(ab).is_some());
    ASSERT_TRUE(downcast<const C>(ac).is_some());
    ASSERT_TRUE(downcast<const B>(ac).is_none());
    ASSERT_TRUE(downcast<const C>(ab).is_none());
}
