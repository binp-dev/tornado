#include <core/collections/vec.hpp>

#include <limits>
#include <array>

#include <core/format.hpp>

#include <gtest/gtest.h>

using namespace core;

TEST(Vec, from_std_vector) {
    std::vector<int32_t> sv{0, 1, 2, 3};
    ASSERT_EQ(sv.size(), 4u);

    std::vector<int32_t> vec(std::move(sv));
    ASSERT_EQ(vec.size(), 4u);
    for (size_t i = 0; i < vec.size(); ++i) {
        ASSERT_EQ(vec[i], int32_t(i));
    }
}

TEST(Vec, into_std_vector) {
    Vec<int32_t> vec{0, 1, 2, 3};
    ASSERT_EQ(vec.size(), 4u);

    std::vector<int32_t> sv(std::move(vec));
    ASSERT_EQ(sv.size(), 4u);
    for (size_t i = 0; i < sv.size(); ++i) {
        ASSERT_EQ(sv[i], int32_t(i));
    }
}

TEST(Vec, write_array_from) {
    Vec<int32_t> a;
    Vec<int32_t> b{0, 1, 2, 3, 4, 5, 6, 7};
    {
        auto s = b.slice();
        ASSERT_EQ(s.size(), 8u);

        ASSERT_EQ(a.write_array_from(s, 4), 4u);
        ASSERT_EQ(a.size(), 4u);
        ASSERT_EQ(s.size(), 4u);
        for (size_t i = 0; i < s.size(); ++i) {
            ASSERT_EQ(s[i], int32_t(i) + 4);
        }

        ASSERT_EQ(a.write_array_from(s, std::nullopt), 4u);
        ASSERT_EQ(a.size(), 8u);
        ASSERT_EQ(s.size(), 0u);

        ASSERT_EQ(a.write_array_from(s, std::nullopt), 0u);
    }

    for (size_t i = 0; i < a.size(); ++i) {
        ASSERT_EQ(a[i], int32_t(i));
    }
}

TEST(Vec, fmt) {
    static_assert(Printable<Vec<int32_t>>);
    Vec<int32_t> vec{0, 1, -1, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min()};
    ASSERT_EQ(core_format("{}", vec), "[0, 1, -1, 2147483647, -2147483648]");
}
