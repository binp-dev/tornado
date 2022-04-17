#include <core/collections/vec_deque.hpp>

#include <cstdint>
#include <memory>
#include <variant>

#include <gtest/gtest.h>

using namespace core;

TEST(VecDeque, grow) {
    VecDeque<int32_t> rb;
    ASSERT_EQ(rb.size(), 0u);
    ASSERT_EQ(rb.capacity(), 0u);

    rb.push_back(0);
    ASSERT_EQ(rb.size(), 1u);
    ASSERT_EQ(rb.capacity(), 1u);

    rb.push_back(1);
    ASSERT_EQ(rb.size(), 2u);
    ASSERT_EQ(rb.capacity(), 3u);

    rb.push_back(2);
    ASSERT_EQ(rb.size(), 3u);
    ASSERT_EQ(rb.capacity(), 3u);

    rb.push_back(3);
    ASSERT_EQ(rb.size(), 4u);
    ASSERT_EQ(rb.capacity(), 7u);

    for (int32_t i = 0; i < 4; ++i) {
        ASSERT_EQ(rb.pop_front(), i);
        ASSERT_EQ(rb.size(), size_t(3 - i));
        ASSERT_EQ(rb.capacity(), 7u);
    }

    ASSERT_EQ(rb.pop_front(), std::nullopt);
    ASSERT_EQ(rb.size(), 0u);
    ASSERT_EQ(rb.capacity(), 7u);
}

TEST(VecDeque, forward_cycle) {
    VecDeque<int32_t> rb;
    rb.reserve(4u);
    ASSERT_EQ(rb.size(), 0u);
    ASSERT_EQ(rb.capacity(), 4u);

    rb.push_back(0);
    rb.push_back(1);
    rb.push_back(2);
    ASSERT_EQ(rb.size(), 3u);

    ASSERT_EQ(rb.pop_front(), 0);
    ASSERT_EQ(rb.pop_front(), 1);
    ASSERT_EQ(rb.size(), 1u);

    rb.push_back(3);
    rb.push_back(4);
    ASSERT_EQ(rb.size(), 3u);

    ASSERT_EQ(rb.pop_front(), 2);
    ASSERT_EQ(rb.pop_front(), 3);
    ASSERT_EQ(rb.size(), 1u);

    rb.push_back(5);
    rb.push_back(6);
    ASSERT_EQ(rb.size(), 3u);

    ASSERT_EQ(rb.pop_front(), 4);
    ASSERT_EQ(rb.pop_front(), 5);
    ASSERT_EQ(rb.pop_front(), 6);
    ASSERT_EQ(rb.size(), 0u);

    ASSERT_EQ(rb.pop_front(), std::nullopt);
    ASSERT_EQ(rb.size(), 0u);
}

TEST(VecDeque, backward_cycle) {
    VecDeque<int32_t> rb;
    rb.reserve(4u);
    ASSERT_EQ(rb.size(), 0u);
    ASSERT_EQ(rb.capacity(), 4u);

    rb.push_front(0);
    rb.push_front(1);
    rb.push_front(2);
    ASSERT_EQ(rb.size(), 3u);

    ASSERT_EQ(rb.pop_back(), 0);
    ASSERT_EQ(rb.pop_back(), 1);
    ASSERT_EQ(rb.size(), 1u);

    rb.push_front(3);
    rb.push_front(4);
    ASSERT_EQ(rb.size(), 3u);

    ASSERT_EQ(rb.pop_back(), 2);
    ASSERT_EQ(rb.pop_back(), 3);
    ASSERT_EQ(rb.size(), 1u);

    rb.push_front(5);
    rb.push_front(6);
    ASSERT_EQ(rb.size(), 3u);

    ASSERT_EQ(rb.pop_back(), 4);
    ASSERT_EQ(rb.pop_back(), 5);
    ASSERT_EQ(rb.pop_back(), 6);
    ASSERT_EQ(rb.size(), 0u);

    ASSERT_EQ(rb.pop_back(), std::nullopt);
    ASSERT_EQ(rb.size(), 0u);
}

TEST(VecDeque, drop) {
    auto rc = std::make_shared<std::monostate>();

    {
        VecDeque<std::shared_ptr<std::monostate>> rb;
        ASSERT_EQ(rc.use_count(), 1);

        rb.push_back(rc);
        ASSERT_EQ(rc.use_count(), 2);

        rb.push_back(rc);
        ASSERT_EQ(rc.use_count(), 3);

        rb.push_back(rc);
        ASSERT_EQ(rc.use_count(), 4);

        ASSERT_TRUE(rb.pop_front().has_value());
        ASSERT_EQ(rc.use_count(), 3);

        rb.clear();
        ASSERT_EQ(rc.use_count(), 1);

        rb.push_front(rc);
        ASSERT_EQ(rc.use_count(), 2);
    }

    ASSERT_EQ(rc.use_count(), 1);
}

TEST(VecDeque, append) {
    VecDeque<std::pair<int32_t, std::shared_ptr<std::monostate>>> a, b;

    auto rc = std::make_shared<std::monostate>();
    ASSERT_EQ(rc.use_count(), 1);

    a.push_back(std::pair(0, rc));
    a.push_back(std::pair(1, rc));
    ASSERT_EQ(rc.use_count(), 3);

    b.push_back(std::pair(2, rc));
    b.push_back(std::pair(3, rc));
    ASSERT_EQ(rc.use_count(), 5);

    a.append(b);
    ASSERT_EQ(rc.use_count(), 5);
    ASSERT_TRUE(!b.pop_front().has_value());

    for (int32_t i = 0; i < 4; ++i) {
        auto x = a.pop_front();
        ASSERT_TRUE(x.has_value());
        ASSERT_EQ(x->first, i);
        ASSERT_EQ(rc.use_count(), 5 - i);
    }

    ASSERT_TRUE(!a.pop_front().has_value());
    ASSERT_EQ(rc.use_count(), 1);
}

TEST(VecDeque, skip) {
    VecDeque<std::pair<int32_t, std::shared_ptr<std::monostate>>> rb;

    auto rc = std::make_shared<std::monostate>();
    ASSERT_EQ(rc.use_count(), 1);

    rb.push_back(std::pair(0, rc));
    rb.push_back(std::pair(1, rc));
    rb.push_back(std::pair(2, rc));
    rb.push_back(std::pair(3, rc));
    ASSERT_EQ(rc.use_count(), 5);

    ASSERT_EQ(rb.skip_front(1u), 1u);
    ASSERT_EQ(rc.use_count(), 4);

    {
        auto x = rb.pop_front();
        ASSERT_TRUE(x.has_value());
        ASSERT_EQ(x->first, 1);
    }
    ASSERT_EQ(rc.use_count(), 3);

    ASSERT_EQ(rb.skip_back(1u), 1u);
    ASSERT_EQ(rc.use_count(), 2);

    {
        auto x = rb.pop_back();
        ASSERT_TRUE(x.has_value());
        ASSERT_EQ(x->first, 2);
    }
    ASSERT_EQ(rc.use_count(), 1);

    ASSERT_TRUE(!rb.pop_front().has_value());
    ASSERT_EQ(rc.use_count(), 1);
}

TEST(VecDeque, read_write) {
    VecDeque<int32_t> rb;
    rb.reserve(5);

    std::array<int32_t, 4> array{0};
    ASSERT_EQ(array, (std::array<int32_t, 4>{0, 0, 0, 0}));
    ASSERT_EQ(rb.read_array(std::span(array).subspan(0, 1)), 0u);

    rb.push_back(1);
    rb.push_back(2);
    rb.push_back(3);
    rb.push_back(4);
    ASSERT_EQ(rb.size(), 4u);
    ASSERT_TRUE(rb.read_array_exact(std::span(array)));
    ASSERT_EQ(rb.size(), 0u);
    ASSERT_EQ(array, (std::array<int32_t, 4>{1, 2, 3, 4}));

    array = std::array<int32_t, 4>{5, 6, 7, 8};
    ASSERT_TRUE(rb.write_array_exact(std::span(array)));
    ASSERT_EQ(rb.size(), 4u);
    ASSERT_EQ(rb.pop_front(), 5u);
    ASSERT_EQ(rb.pop_front(), 6u);
    ASSERT_EQ(rb.pop_front(), 7u);
    ASSERT_EQ(rb.pop_front(), 8u);
    ASSERT_EQ(rb.pop_front(), std::nullopt);
    ASSERT_EQ(rb.size(), 0u);

    array = std::array<int32_t, 4>{9, 10, 11, 12};
    ASSERT_TRUE(rb.write_array_exact(std::span(array)));
    ASSERT_EQ(rb.size(), 4u);

    array = std::array<int32_t, 4>{0};
    ASSERT_TRUE(rb.read_array_exact(std::span(array)));
    ASSERT_EQ(array, (std::array<int32_t, 4>{9, 10, 11, 12}));
    ASSERT_EQ(rb.size(), 0u);
}

TEST(VecDeque, write_grow) {
    VecDeque<int32_t> rb;

    std::array<int32_t, 8> array{1, 2, 3, 4, 5, 6, 7, 8};
    ASSERT_EQ(rb.read_array(std::span(array).subspan(0, 1)), 0u);

    ASSERT_TRUE(rb.write_array_exact(std::span(array)));
    ASSERT_EQ(rb.size(), 8u);
    ASSERT_EQ(rb.capacity(), 15u);
    for (int32_t i = 0; i < 8; ++i) {
        ASSERT_EQ(rb.pop_front(), i + 1);
    }
    ASSERT_EQ(rb.pop_front(), std::nullopt);
    ASSERT_EQ(rb.size(), 0u);
    ASSERT_EQ(rb.capacity(), 15u);
}

TEST(VecDeque, write_array_from) {
    VecDeque<int32_t> a, b;

    std::array<int32_t, 4> array{1, 2, 3, 4};
    ASSERT_TRUE(a.write_array_exact(std::span(array)));
    ASSERT_EQ(a.size(), 4u);
    ASSERT_EQ(b.size(), 0u);

    ASSERT_EQ(b.write_array_from(a, std::nullopt), 4u);
    ASSERT_EQ(a.size(), 0u);
    ASSERT_EQ(b.size(), 4u);

    ASSERT_EQ(a.write_array_from(b, std::nullopt), 4u);
    ASSERT_EQ(a.size(), 4u);
    ASSERT_EQ(b.size(), 0u);

    ASSERT_EQ(b.write_array_from(a, std::nullopt), 4u);
    ASSERT_EQ(a.size(), 0u);
    ASSERT_EQ(b.size(), 4u);
    ASSERT_EQ(b.capacity(), 7u);

    array = std::array<int32_t, 4>{0};
    ASSERT_TRUE(b.read_array_exact(std::span(array)));
    ASSERT_EQ(array, (std::array<int32_t, 4>{1, 2, 3, 4}));
    ASSERT_EQ(b.size(), 0u);
}

TEST(VecDeque, read_array_into) {
    VecDeque<int32_t> a, b;

    std::array<int32_t, 8> array{1, 2, 3, 4, 5, 6, 7, 8};
    ASSERT_TRUE(a.write_array_exact(std::span(array)));
    ASSERT_EQ(a.size(), 8u);
    ASSERT_EQ(b.size(), 0u);

    ASSERT_EQ(a.read_array_into(b, std::nullopt), 8u);
    ASSERT_EQ(a.size(), 0u);
    ASSERT_EQ(b.size(), 8u);
    ASSERT_EQ(b.capacity(), 15u);

    ASSERT_EQ(b.read_array_into(a, 8), 8u);
    ASSERT_EQ(a.size(), 8u);
    ASSERT_EQ(b.size(), 0u);

    ASSERT_EQ(a.read_array_into(b, 4), 4u);
    ASSERT_EQ(a.size(), 4u);
    ASSERT_EQ(b.size(), 4u);
    ASSERT_EQ(a.read_array_into(b, 4), 4u);
    ASSERT_EQ(a.size(), 0u);
    ASSERT_EQ(b.size(), 8u);

    array = std::array<int32_t, 8>{0};
    ASSERT_TRUE(b.read_array_exact(std::span(array)));
    ASSERT_EQ(array, (std::array<int32_t, 8>{1, 2, 3, 4, 5, 6, 7, 8}));
    ASSERT_EQ(b.size(), 0u);
}

TEST(VecDequeView, pop_front) {
    VecDeque<int32_t> rb;
    rb.reserve(4u);

    rb.push_back(0);
    rb.push_back(1);
    rb.push_back(2);

    {
        auto rbv = rb.view();
        ASSERT_EQ(rbv.size(), 3u);
        ASSERT_EQ(rbv.pop_front(), 0);
        ASSERT_EQ(rbv.pop_front(), 1);
        ASSERT_EQ(rbv.pop_front(), 2);
        ASSERT_EQ(rbv.pop_front(), std::nullopt);
        ASSERT_EQ(rbv.size(), 0u);
    }
    ASSERT_EQ(rb.pop_front(), 0);
    ASSERT_EQ(rb.pop_front(), 1);

    rb.push_back(3);
    rb.push_back(4);

    {
        auto rbv = rb.view();
        ASSERT_EQ(rbv.size(), 3u);
        ASSERT_EQ(rbv.pop_front(), 2);
        ASSERT_EQ(rbv.pop_front(), 3);
        ASSERT_EQ(rbv.pop_front(), 4);
        ASSERT_EQ(rbv.pop_front(), std::nullopt);
        ASSERT_EQ(rbv.size(), 0u);
    }
    ASSERT_EQ(rb.pop_front(), 2);
    ASSERT_EQ(rb.pop_front(), 3);

    rb.push_back(5);
    rb.push_back(6);

    {
        auto rbv = rb.view();
        ASSERT_EQ(rbv.size(), 3u);
        ASSERT_EQ(rbv.pop_front(), 4);
        ASSERT_EQ(rbv.pop_front(), 5);
        ASSERT_EQ(rbv.pop_front(), 6);
        ASSERT_EQ(rbv.pop_front(), std::nullopt);
        ASSERT_EQ(rbv.size(), 0u);
    }
    ASSERT_EQ(rb.pop_front(), 4);
    ASSERT_EQ(rb.pop_front(), 5);
    ASSERT_EQ(rb.pop_front(), 6);

    ASSERT_EQ(rb.pop_front(), std::nullopt);
}

TEST(VecDequeView, pop_back) {
    VecDeque<int32_t> rb;
    rb.reserve(4u);

    rb.push_front(0);
    rb.push_front(1);
    rb.push_front(2);

    {
        auto rbv = rb.view();
        ASSERT_EQ(rbv.size(), 3u);
        ASSERT_EQ(rbv.pop_back(), 0);
        ASSERT_EQ(rbv.pop_back(), 1);
        ASSERT_EQ(rbv.pop_back(), 2);
        ASSERT_EQ(rbv.pop_back(), std::nullopt);
        ASSERT_EQ(rbv.size(), 0u);
    }
    ASSERT_EQ(rb.pop_back(), 0);
    ASSERT_EQ(rb.pop_back(), 1);

    rb.push_front(3);
    rb.push_front(4);

    {
        auto rbv = rb.view();
        ASSERT_EQ(rbv.size(), 3u);
        ASSERT_EQ(rbv.pop_back(), 2);
        ASSERT_EQ(rbv.pop_back(), 3);
        ASSERT_EQ(rbv.pop_back(), 4);
        ASSERT_EQ(rbv.pop_back(), std::nullopt);
        ASSERT_EQ(rbv.size(), 0u);
    }
    ASSERT_EQ(rb.pop_back(), 2);
    ASSERT_EQ(rb.pop_back(), 3);

    rb.push_front(5);
    rb.push_front(6);

    {
        auto rbv = rb.view();
        ASSERT_EQ(rbv.size(), 3u);
        ASSERT_EQ(rbv.pop_back(), 4);
        ASSERT_EQ(rbv.pop_back(), 5);
        ASSERT_EQ(rbv.pop_back(), 6);
        ASSERT_EQ(rbv.pop_back(), std::nullopt);
        ASSERT_EQ(rbv.size(), 0u);
    }
    ASSERT_EQ(rb.pop_back(), 4);
    ASSERT_EQ(rb.pop_back(), 5);
    ASSERT_EQ(rb.pop_back(), 6);
}


TEST(VecDequeView, skip) {
    VecDeque<int32_t> rb;

    rb.push_back(0);
    rb.push_back(1);
    rb.push_back(2);
    rb.push_back(3);

    VecDequeView<int32_t> rbv = rb.view();

    ASSERT_EQ(rbv.skip_front(1u), 1u);
    ASSERT_EQ(rbv.pop_front(), 1);

    ASSERT_EQ(rbv.skip_back(1u), 1u);
    ASSERT_EQ(rbv.pop_back(), 2);

    ASSERT_TRUE(!rbv.pop_front().has_value());
    ASSERT_EQ(rbv.skip_front(1u), 0u);
    ASSERT_TRUE(!rbv.pop_back().has_value());
    ASSERT_EQ(rbv.skip_back(1u), 0u);

    ASSERT_EQ(rb.size(), 4u);
}

TEST(VecDequeView, read) {
    VecDeque<int32_t> rb;
    rb.reserve(5);

    std::array<int32_t, 4> array{0};
    ASSERT_EQ(rb.view().read_array(std::span(array).subspan(0, 1)), 0u);

    rb.push_back(1);
    rb.push_back(2);
    rb.push_back(3);
    rb.push_back(4);
    ASSERT_EQ(rb.size(), 4u);

    VecDequeView<int32_t> rbv = rb.view();
    ASSERT_EQ(rbv.size(), 4u);
    ASSERT_TRUE(rbv.read_array_exact(std::span(array)));
    ASSERT_EQ(rbv.size(), 0u);
    ASSERT_EQ(array, (std::array<int32_t, 4>{1, 2, 3, 4}));

    array = std::array<int32_t, 4>{5, 6, 7, 8};
    ASSERT_EQ(rb.skip_front(4u), 4u);
    ASSERT_TRUE(rb.write_array_exact(std::span(array)));
    ASSERT_EQ(rb.size(), 4u);

    rbv = rb.view();
    ASSERT_EQ(rbv.size(), 4u);
    ASSERT_EQ(rbv.pop_front(), 5u);
    ASSERT_EQ(rbv.pop_front(), 6u);
    ASSERT_EQ(rbv.pop_front(), 7u);
    ASSERT_EQ(rbv.pop_front(), 8u);
    ASSERT_EQ(rbv.pop_front(), std::nullopt);
    ASSERT_EQ(rbv.size(), 0u);

    array = std::array<int32_t, 4>{9, 10, 11, 12};
    ASSERT_EQ(rb.skip_front(4u), 4u);
    ASSERT_TRUE(rb.write_array_exact(std::span(array)));
    ASSERT_EQ(rb.size(), 4u);

    rbv = rb.view();
    ASSERT_EQ(rbv.size(), 4u);
    array = std::array<int32_t, 4>{0};
    ASSERT_TRUE(rbv.read_array_exact(std::span(array)));
    ASSERT_EQ(array, (std::array<int32_t, 4>{9, 10, 11, 12}));
    ASSERT_EQ(rbv.size(), 0u);
    ASSERT_EQ(rb.size(), 4u);
}
