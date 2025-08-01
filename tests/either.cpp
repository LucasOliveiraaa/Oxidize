#define CATCH_CONFIG_MAIN
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "Oxidize/prelude.hpp"

using namespace ox;
using namespace ox::either;

TEST_CASE("Either type behavior", "[either]") {
    Either<String, i32> left = Left(move(String::from("error")));
    Either<String, i32> right = Right(123);

    REQUIRE(left.is_left());
    REQUIRE(!left.is_right());

    REQUIRE(right.is_right());
    REQUIRE(!right.is_left());

    REQUIRE(move(left).unwrap_left() == "error");
    REQUIRE(move(right).unwrap_right() == 123);
}