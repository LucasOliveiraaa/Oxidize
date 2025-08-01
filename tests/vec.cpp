#define CATCH_CONFIG_MAIN
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "Oxidize/prelude.hpp"

using namespace ox;

TEST_CASE("Vec push and indexing", "[vec]") {
    mut v = Vec<int>::new_();
    REQUIRE(v.len() == 0);

    v.push(10);
    v.push(20);
    v.push(30);

    REQUIRE(v.len() == 3);
    REQUIRE(v[0] == 10);
    REQUIRE(v[1] == 20);
    REQUIRE(v[2] == 30);
}

TEST_CASE("Vec pop and pop_if", "[vec]") {
    mut v = vec(1, 2, 3);

    let popped = v.pop();
    REQUIRE(move(popped).is_some());
    REQUIRE(popped.unwrap() == 3);
    REQUIRE(v.len() == 2);

    // pop_if with predicate true
    let popped_if = v.pop_if([](int &x) { return x == 2; });
    REQUIRE(move(popped_if).is_some());
    REQUIRE(move(popped_if).unwrap() == 2);
    REQUIRE(v.len() == 1);

    // pop_if with predicate false
    let popped_if_none = v.pop_if([](int &x) { return x == 99; });
    REQUIRE(popped_if_none.is_none());
    REQUIRE(v.len() == 1);
}

TEST_CASE("Vec remove and swap_remove", "[vec]") {
    mut v = vec(1, 2, 3, 4, 5);

    int removed = v.remove(2); // remove value 3
    REQUIRE(removed == 3);
    REQUIRE(v.len() == 4);
    REQUIRE(v[0] == 1);
    REQUIRE(v[1] == 2);
    REQUIRE(v[2] == 4);
    REQUIRE(v[3] == 5);

    int swapped = v.swap_remove(1); // swap remove value 2
    REQUIRE(swapped == 2);
    REQUIRE(v.len() == 3);
    REQUIRE(v[0] == 1);
    REQUIRE(v[1] == 5);
    REQUIRE(v[2] == 4);
}

TEST_CASE("Vec resize and clear", "[vec]") {
    mut v = vec(5, 6, 7);

    v.resize(5, 42);
    REQUIRE(v.len() == 5);
    REQUIRE(v[3] == 42);
    REQUIRE(v[4] == 42);

    v.resize(2, 0);
    REQUIRE(v.len() == 2);
    REQUIRE(v[0] == 5);
    REQUIRE(v[1] == 6);

    v.clear();
    REQUIRE(v.len() == 0);
}

TEST_CASE("Vec retain and retain_mut", "[vec]") {
    mut v = vec(1, 2, 3, 4, 5, 6);

    v.retain([](const int &x) { return x % 2 == 0; });
    REQUIRE(v.len() == 3);
    REQUIRE(v[0] == 2);
    REQUIRE(v[1] == 4);
    REQUIRE(v[2] == 6);

    v.retain_mut([](int &x) {
        x *= 10;
        return true;
    });
    REQUIRE(v[0] == 20);
    REQUIRE(v[1] == 40);
    REQUIRE(v[2] == 60);
}

TEST_CASE("Vec append", "[vec]") {
    mut v1 = vec(1, 2);
    mut v2 = vec(3, 4, 5);

    v1.append(v2);

    REQUIRE(v1.len() == 5);
    REQUIRE(v1[0] == 1);
    REQUIRE(v1[1] == 2);
    REQUIRE(v1[2] == 3);
    REQUIRE(v1[3] == 4);
    REQUIRE(v1[4] == 5);

    REQUIRE(v2.len() == 0);
}

TEST_CASE("Vec iterators", "[vec]") {
    mut v = vec(10, 20, 30);

    int sum = 0;
    for (let &x : v.iter()) {
        sum += x;
    }
    REQUIRE(sum == 60);

    for (mut &x : v.iter_mut()) {
        x += 1;
    }
    REQUIRE(v[0] == 11);
    REQUIRE(v[1] == 21);
    REQUIRE(v[2] == 31);
}