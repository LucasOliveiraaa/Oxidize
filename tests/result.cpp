#define CATCH_CONFIG_MAIN
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "Oxidize/prelude.hpp"

using namespace ox;

TEST_CASE("Result basic operations", "[result]") {
    Result<i32, String> ok1 = Ok(100);
    Result<i32, String> ok2 = Ok(200);
    Result<i32, String> err = Err(String::from("Failure"));

    REQUIRE(ok1.is_ok());
    REQUIRE(!ok1.is_err());

    REQUIRE(err.is_err());
    REQUIRE(!err.is_ok());

    REQUIRE(ok1.unwrap() == 100);
    REQUIRE(move(err).unwrap_or(42) == 42);

    REQUIRE(move(ok1).or_(move(ok2)).unwrap() == 100); // ok1 is used
    REQUIRE(move(err).or_(move(ok2)).unwrap() == 200); // fallback
}

TEST_CASE("Result map and and_then", "[result]") {
    let double_val = [](const i32 &x) { return x * 2; };
    let to_string = [](const i32 &x) { return Ok(format("Value: {}", x)); };

    Result<i32, String> ok = Ok(10);
    Result<i32, String> err = Err(String::from("nope"));

    REQUIRE(ok.clone().map<i32>(double_val).unwrap() == 20);
    REQUIRE(err.clone().map<i32>(double_val).is_err());

    REQUIRE(move(ok).and_then<String>(to_string).unwrap() == "Value: 10");
    REQUIRE(move(err).and_then<String>(to_string).is_err());
}