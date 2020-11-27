#include <gtest/gtest.h>

TEST(Dummy, Add) {
    ASSERT_EQ(1 + 1, 2);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
