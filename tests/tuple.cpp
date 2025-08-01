#define CATCH_CONFIG_MAIN
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "Oxidize/prelude.hpp"

using namespace ox;

TEST_CASE("Tuple basics", "[tuple]") {
    // Create tuples using parentheses and brace initialization
    mut t1 = Tp(1, "a", true);
    let t2 = Tp{1, "a", true};

    // Structured bindings to unpack tuple elements
    auto &&[a, b, c] = move(t1);

    // Check unpacked values
    REQUIRE(a == 1);
    REQUIRE(String::from(b) == "a"); // Convert const char* to std::string for comparison
    REQUIRE(c == true);

    // Equality comparison with brace-initialized tuple
    REQUIRE(t1 == Tp{1, "a", true});
    REQUIRE(t1 == t2);

    // Access elements via get<>
    REQUIRE(t1.get<0>() == 1);
    REQUIRE(String::from(t1.get<1>()) == "a");
    REQUIRE(t1.get<2>() == true);

    // Negative test: tuples with different contents should not be equal
    let t3 = Tp(2, "a", true);
    REQUIRE_FALSE(t1 == t3);

    let nested = Tp(move(t1), 3.14);
    mut && [ inner, pi ] = nested;
    REQUIRE(inner == t1);
    REQUIRE(pi == Catch::Approx(3.14));
}