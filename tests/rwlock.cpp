#define CATCH_CONFIG_MAIN
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "Oxidize/panic/Panic.hpp"
#include "Oxidize/sync/RwLock.hpp"
#include "Oxidize/core/Match.hpp"

using namespace ox;

TEST_CASE("RwLock basic read/write", "[rwlock]") {
    let rwlock = sync::RwLock(42);

    // Write lock
    match(rwlock.write())
        .ok([](sync::RwLockWriteGuard<i32> guard) {
            REQUIRE(*guard == 42);
            *guard = 100;
        })
        .err([](auto) { FAIL("Write lock should not be poisoned"); });

    // Read lock sees updated value
    match(rwlock.read())
        .ok([](sync::RwLockReadGuard<i32> guard) {
            REQUIRE(*guard == 100);
        })
        .err([](auto) { FAIL("Read lock should not be poisoned"); });
}

TEST_CASE("RwLock try_read / try_write", "[rwlock]") {
    let rwlock = sync::RwLock(5);

    match(rwlock.try_write())
        .ok([&](sync::RwLockWriteGuard<i32> guard) {
            REQUIRE(*guard == 5);

            // try_write should fail while write lock held
            match(rwlock.try_write())
                .ok([](auto) { FAIL("Second try_write should not succeed"); })
                .err([](sync::TryLockError<sync::RwLockWriteGuard<i32>> err) {
                    match(err).wouldBlock([]() { SUCCEED("Correctly blocked on second try_write"); });
                });

            // try_read should also fail (exclusive write held)
            match(rwlock.try_read())
                .ok([](auto) { FAIL("try_read should not succeed while write lock held"); })
                .err([](sync::TryLockError<sync::RwLockReadGuard<i32>> err) {
                    match(err).wouldBlock([]() { SUCCEED("Correctly blocked on try_read"); });
                });
        })
        .err([](auto) { FAIL("First try_write should succeed"); });

    // Multiple concurrent readers allowed
    match(rwlock.try_read())
        .ok([&](sync::RwLockReadGuard<i32> guard1) {
            match(rwlock.try_read())
                .ok([&](sync::RwLockReadGuard<i32> guard2) {
                    REQUIRE(*guard1 == *guard2);
                })
                .err([](auto) { FAIL("Second try_read should succeed concurrently"); });
        })
        .err([](auto) { FAIL("First try_read should succeed"); });
}

TEST_CASE("RwLock poisoning on panic", "[rwlock]") {
    mut rwlock = sync::RwLock(99);

    // Cause a panic while holding write lock
    let _ = panic::catch_unwind([&]() {
        match(rwlock.write())
            .ok([](sync::RwLockWriteGuard<i32> guard) {
                REQUIRE(*guard == 99);
                *guard = 123;
                panic("simulated panic");
            })
            .err([](auto) { FAIL("Should not be poisoned before panic"); });
    });

    REQUIRE(rwlock.is_poisoned());

    // Attempt to acquire write after poison
    match(rwlock.write())
        .ok([](auto) { FAIL("Write lock should not succeed, mutex is poisoned"); })
        .err([&rwlock](sync::PoisonError<sync::RwLockWriteGuard<i32>> err) {
            mut guard = move(err).into_inner();
            REQUIRE(*guard == 123);
            *guard = 456;
        });

    // Clear poison and validate recovery
    rwlock.poisoned.store(false, std::memory_order_release);

    match(rwlock.write())
        .ok([](sync::RwLockWriteGuard<i32> guard) {
            REQUIRE(*guard == 456);
        })
        .err([](auto) { FAIL("Should not be poisoned anymore"); });
}

TEST_CASE("RwLock read after write updates", "[rwlock]") {
    let rwlock = sync::RwLock(10);

    match(rwlock.write())
        .ok([](sync::RwLockWriteGuard<i32> guard) { *guard = 77; })
        .err([](auto) { FAIL("Write lock should succeed"); });

    match(rwlock.read())
        .ok([](sync::RwLockReadGuard<i32> guard) { REQUIRE(*guard == 77); })
        .err([](auto) { FAIL("Read lock should succeed"); });
}
