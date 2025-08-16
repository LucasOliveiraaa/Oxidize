#define CATCH_CONFIG_MAIN
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "Oxidize/panic/Panic.hpp"
#include "Oxidize/sync/Mutex.hpp"
#include "Oxidize/core/Match.hpp"

using namespace ox;

TEST_CASE("Basic lock/unlock", "[mutex]") {
    let mutex = sync::Mutex(42);

    match(mutex.lock())
        .ok([](sync::Mutex<i32>::Ok guard) {
            REQUIRE(*guard == 42);
            *guard = 100;
        })
        .err([](sync::Mutex<i32>::Err) { FAIL("Lock should not be poisoned"); });
}

TEST_CASE("try_lock success and wouldBlock", "[mutex]") {
    let mutex = sync::Mutex(5);

    match(mutex.try_lock())
        .ok([&](sync::Mutex<i32>::Ok guard) {
            REQUIRE(*guard == 5);

            match(mutex.try_lock())
                .ok([](sync::Mutex<i32>::Ok) { FAIL("Second try_lock should not succeed"); })
                .err([](sync::Mutex<i32>::TryErr err) {
                    match(err).wouldBlock(
                        []() { SUCCEED("Correctly blocked on second try_lock"); });
                });
        })
        .err([](sync::Mutex<i32>::TryErr) { FAIL("First try_lock should succeed"); });
}

TEST_CASE("Poisoned mutex behavior", "[mutex]") {
    mut mutex = sync::Mutex(99);

    // Cause a panic while holding the lock
    let _ = panic::catch_unwind([&]() {
        match(mutex.lock())
            .ok([](sync::Mutex<i32>::Ok guard) {
                REQUIRE(*guard == 99);
                *guard = 123;
                panic("simulated panic");
            })
            .err([](sync::Mutex<i32>::Err) { FAIL("Should not be poisoned before panic"); });
    });

    REQUIRE(mutex.is_poisoned());

    // Try to lock after panic; should be poisoned
    match(mutex.lock())
        .ok([](sync::Mutex<i32>::Ok) { FAIL("Should not succeed, should return poison error"); })
        .err([&mutex](sync::Mutex<i32>::Err err) {
            mut guard = move(err).into_inner();

            REQUIRE(mutex.is_poisoned());
            REQUIRE(*guard == 123);
            *guard = 456;
        });

    // Clear poison, validate not poisoned anymore
    mutex.clear_poison();
    REQUIRE_FALSE(mutex.is_poisoned());

    auto recovered = mutex.lock();
    match(recovered)
        .ok([](sync::Mutex<i32>::Ok guard) { REQUIRE(*guard == 456); })
        .err([](sync::Mutex<i32>::Err) { FAIL("Should not be poisoned anymore"); });
}

TEST_CASE("get_mut returns mutable reference", "[mutex]") {
    mut mutex = sync::Mutex(17);
    match(mutex.get_mut())
        .ok([](int *v) {
            REQUIRE(*v == 17);
            *v = 88;
        })
        .err([](auto) { FAIL("Mutex should not be poisoned"); });
        
    REQUIRE(move(mutex).into_inner().unwrap() == 88);
}

TEST_CASE("into_inner consumes and returns inner", "[mutex]") {
    auto mutex = sync::Mutex(42);
    REQUIRE(move(mutex).into_inner().unwrap() == 42);
}
