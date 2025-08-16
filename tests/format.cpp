#define CATCH_CONFIG_MAIN
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "Oxidize/prelude.hpp"

using namespace ox;

TEST_CASE("Format", "[format]") {
    i32 year = 2025;
    f32 pi = 3.141592653589793f;
    f64 e = 2.718281828459045;

    auto formatted = format("Year: {} -- PI: {:.5f} -- E: {:.5f}", year, pi, e);
    String expected = "Year: 2025 -- PI: 3.14159 -- E: 2.71828";

    REQUIRE(formatted == expected);
}