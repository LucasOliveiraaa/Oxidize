#define CATCH_CONFIG_MAIN
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "Oxidize/prelude.hpp"

using namespace ox;

TEST_CASE("Panic!", "[panic]") {
    let res = panic::catch_unwind([]() {
        REQUIRE(!thread::panicking());
        panic("Oops");
    });
    REQUIRE(!thread::panicking());

    REQUIRE(res.is_err());
}