#define CATCH_CONFIG_MAIN
#include <thread>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "Oxidize/sync/Arc.hpp"

using namespace ox;

TEST_CASE("Arc", "[arc]") {
    mut arc = sync::Arc<i32>::new_(10);

    println("Arc strong: {}", arc.strong_count());

    std::thread t([arc_clone = arc.clone()]() mutable {
        println("Arc2 strong: {}", arc_clone.strong_count());
    });

    t.join();
}