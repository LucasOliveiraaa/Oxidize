#include "Oxidize/panic/Unwind.hpp"
#include <thread>
#define CATCH_CONFIG_MAIN
#include "Oxidize/vec/Vec.hpp"
#include "Oxidize/util/Print.hpp"
#include "Oxidize/panic/Panic.hpp"
#include "Oxidize/sync/Arc.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace ox;
using namespace ox::either;
using namespace ox::sync;

TEST_CASE("Panic!", "[panic]") {
    let res = panic::catch_unwind([]() {
        REQUIRE(!thread::panicking());
        panic("Oops");
    });
    REQUIRE(!thread::panicking());

    REQUIRE(res.is_err());
}

TEST_CASE("Arc", "[arc]") {
    mut arc = Arc<i32>::new_(10);

    println("Arc strong: {}", arc.strong_count());

    std::thread t([arc_clone = arc.clone()]() mutable {
        println("Arc2 strong: {}", arc_clone.strong_count());
    });

    t.join();
}

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
    println("{}", v);
    println("{}", v.iter());

    int sum = 0;
    for (let &x : v.iter()) {
        println("{}", x);
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

TEST_CASE("Tuple basics", "[tuple]") {
    // Create tuples using parentheses and brace initialization
    mut t1 = Tp(1, "a", true);
    let t2 = Tp{1, "a", true};

    // Structured bindings to unpack tuple elements
    auto &&[a, b, c] = move(t1);

    // Check unpacked values
    REQUIRE(a == 1);
    REQUIRE(String(b) == "a"); // Convert const char* to std::string for comparison
    REQUIRE(c == true);

    // Equality comparison with brace-initialized tuple
    REQUIRE(t1 == Tp{1, "a", true});
    REQUIRE(t1 == t2);

    // Access elements via get<>
    REQUIRE(t1.get<0>() == 1);
    REQUIRE(String(t1.get<1>()) == "a");
    REQUIRE(t1.get<2>() == true);

    // Negative test: tuples with different contents should not be equal
    let t3 = Tp(2, "a", true);
    REQUIRE_FALSE(t1 == t3);

    let nested = Tp(move(t1), 3.14);
    mut && [ inner, pi ] = nested;
    REQUIRE(inner == t1);
    REQUIRE(pi == Catch::Approx(3.14));
}

TEST_CASE("Result basic operations", "[result]") {
    Result<i32, String> ok1 = Ok(100);
    Result<i32, String> ok2 = Ok(200);
    Result<i32, String> err = Err(move(String("Failure")));

    REQUIRE(ok1.is_ok());
    REQUIRE(!ok1.is_err());

    REQUIRE(err.is_err());
    REQUIRE(!err.is_ok());

    REQUIRE(move(ok1).unwrap() == 100);
    REQUIRE(move(err).unwrap_or(42) == 42);

    REQUIRE(move(ok1).or_(move(ok2)).unwrap() == 100); // ok1 is used
    REQUIRE(move(err).or_(move(ok2)).unwrap() == 200); // fallback
}

TEST_CASE("Result map and and_then", "[result]") {
    let double_val = [](const i32 &x) { return x * 2; };
    let to_string = [](const i32 &x) { return Ok(format("Value: {}", x)); };

    Result<i32, String> ok = Ok(10);
    Result<i32, String> err = Err(move(String("nope")));

    REQUIRE(move(ok).map<i32>(double_val).unwrap() == 20);
    REQUIRE(move(err).map<i32>(double_val).is_err());

    REQUIRE(move(ok).and_then<String>(to_string).unwrap() == "Value: 10");
    REQUIRE(move(err).and_then<String>(to_string).is_err());
}

TEST_CASE("Option basic usage", "[option]") {
    let some = Some(7);
    Option<i32> none = None;

    REQUIRE(some.is_some());
    REQUIRE(!some.is_none());

    REQUIRE(none.is_none());
    REQUIRE(!none.is_some());

    REQUIRE(some.unwrap() == 7);
    REQUIRE(none.unwrap_or(99) == 99);
}

TEST_CASE("Option map and and_then", "[option]") {
    mut some = Some(3);
    let other = Some(move(String("A")));
    Option<i32> none = Option<i32>(None);

    let square = [](const i32 &x) { return x * x; };
    let to_option = [](const i32 &x) { return Some(x + 1); };

    some.match([](let a) { REQUIRE(a == 3); }, []() {});

    other.match().some([](const String &a) { REQUIRE(a == String("A")); });

    REQUIRE(some.map<i32>(square).unwrap() == 9);
    REQUIRE(none.map<i32>(square).is_none());

    REQUIRE(move(some).zip(move(other)).unwrap() == Tp(3, String("A"))); // Here
    REQUIRE(move(none).zip(move(some)).is_none());

    REQUIRE(some.and_then<i32>(to_option).unwrap() == 4);
    REQUIRE(none.and_then<i32>(to_option).is_none());
}

TEST_CASE("Either type behavior", "[either]") {
    Either<String, i32> left = Left(move(String("error")));
    Either<String, i32> right = Right(123);

    REQUIRE(left.is_left());
    REQUIRE(!left.is_right());

    REQUIRE(right.is_right());
    REQUIRE(!right.is_left());

    REQUIRE(move(left).unwrap_left() == "error");
    REQUIRE(move(right).unwrap_right() == 123);
}