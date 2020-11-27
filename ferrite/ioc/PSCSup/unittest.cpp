#define CATCH_MAIN
#include <catch/catch.hpp>

TEST_CASE( "Dummy test", "[dummy]" ) {
    REQUIRE(1 + 1 == 2);
}
