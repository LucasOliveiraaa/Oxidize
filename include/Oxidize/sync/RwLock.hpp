#pragma once
#include "Oxidize/core/Move.hpp"
#include "Oxidize/core/OptRes.hpp"
#include "Oxidize/panic/Unwind.hpp"
#include "Oxidize/sync/error.h"
#include <atomic>
#include <mutex>
#include <shared_mutex>

namespace ox::sync {

template <typename T> struct RwLockReadGuard {
    using LockType = std::shared_lock<std::shared_mutex>;
    LockType lock;
    T *data;
    std::atomic<bool> *poisoned;

  public:
    RwLockReadGuard(LockType lock, T *data, std::atomic<bool> *poisoned)
        : lock(ox::move(lock)), data(data), poisoned(poisoned) {}
    ~RwLockReadGuard() {
        if (thread::panicking()) {
            poisoned->store(true, std::memory_order_release);
        }
    }


    RwLockReadGuard(RwLockReadGuard &&other) : lock(ox::move(other.lock)), data(other.data), poisoned(other.poisoned) {}
    RwLockReadGuard &operator=(RwLockReadGuard &&other) {
        if(this != &other) {
            this->lock = ox::move(other.lock);
            this->data = other.data;
            this->poisoned = other.poisoned;
        }
        return *this;
    }

    RwLockReadGuard(const RwLockReadGuard &) = delete;
    RwLockReadGuard &operator=(const RwLockReadGuard &) = delete;

    const T *operator->() const { return data; }
    const T &operator*() const { return *data; }
};
template <typename T> struct RwLockWriteGuard {
    using LockType = std::unique_lock<std::shared_mutex>;
    LockType lock;
    T *data;
    std::atomic<bool> *poisoned;

  public:
    RwLockWriteGuard(LockType lock, T *data, std::atomic<bool> *poisoned)
        : lock(ox::move(lock)), data(data), poisoned(poisoned) {}
    ~RwLockWriteGuard() {
        if (thread::panicking()) {
            poisoned->store(true, std::memory_order_release);
        }
    }


    RwLockWriteGuard(RwLockWriteGuard &&other) : lock(ox::move(other.lock)), data(other.data), poisoned(other.poisoned) {}
    RwLockWriteGuard &operator=(RwLockWriteGuard &&other) {
        if(this != &other) {
            this->lock = ox::move(other.lock);
            this->data = other.data;
            this->poisoned = other.poisoned;
        }
        return *this;
    }

    RwLockWriteGuard(const RwLockWriteGuard &) = delete;
    RwLockWriteGuard &operator=(const RwLockWriteGuard &) = delete;

    T *operator->() { return data; }
    T &operator*() { return *data; }
    const T *operator->() const { return data; }
    const T &operator*() const { return *data; }
};

template <typename T> struct RwLock {
    mutable T data;
    mutable std::shared_mutex mutex;
    mutable std::atomic<bool> poisoned;

  public:
    RwLock(T &&data) : data(std::forward<T>(data)), poisoned(false) {}

    RwLock(RwLock &&other) : data(ox::move(other.data)) {}
    RwLock &operator=(RwLock &&other) {
        if (this != &other) {
            this->data = ox::move(other.data);
        }
        return *this;
    }

    RwLock(const RwLock &) = delete;
    RwLock &operator=(const RwLock &) = delete;

    RwLock<T> new_(T &&data) {
        return RwLock<T>(forward<T>(data));
    }

    LockResult<RwLockReadGuard<T>> read() const noexcept {
        if (is_poisoned()) {
            return Err(PoisonError<RwLockReadGuard<T>>(
                RwLockReadGuard<T>(std::shared_lock<std::shared_mutex>(mutex), &data, &poisoned)));
        }
        return Ok(RwLockReadGuard<T>(std::shared_lock<std::shared_mutex>(mutex), &data, &poisoned));
    }
    LockResult<RwLockWriteGuard<T>> write() const noexcept {
        if (is_poisoned()) {
            return Err(PoisonError<RwLockWriteGuard<T>>(
                RwLockWriteGuard<T>(std::unique_lock<std::shared_mutex>(mutex), &data, &poisoned)));
        }
        return Ok(RwLockWriteGuard<T>(std::unique_lock<std::shared_mutex>(mutex), &data, &poisoned));
    }

    bool is_poisoned() const noexcept { return poisoned.load(std::memory_order_acquire); }

    TryLockResult<RwLockReadGuard<T>> try_read() const {
        if (mutex.try_lock_shared()) {
            if (is_poisoned()) {
                return Err(TryLockError<RwLockReadGuard<T>>(RwLockReadGuard<T>(
                    std::shared_lock<std::shared_mutex>(mutex, std::adopt_lock), &data, &poisoned)));
            }
            return Ok(
                RwLockReadGuard<T>(std::shared_lock<std::shared_mutex>(mutex,  std::adopt_lock), &data, &poisoned));
        }
        return Err(TryLockError<RwLockReadGuard<T>>());
    }
    TryLockResult<RwLockWriteGuard<T>> try_write() const {
        if (mutex.try_lock()) {
            if (is_poisoned()) {
                return Err(TryLockError<RwLockWriteGuard<T>>(RwLockWriteGuard<T>(
                    std::unique_lock<std::shared_mutex>(mutex, std::adopt_lock), &data, &poisoned)));
            }
            return Ok(
                RwLockWriteGuard<T>(std::unique_lock<std::shared_mutex>(mutex,  std::adopt_lock), &data, &poisoned));
        }
        return Err(TryLockError<RwLockWriteGuard<T>>());
    }


    void clear_poison() noexcept {
        poisoned.store(false, std::memory_order_release);
    }

    LockResult<T *const> get_mut() & {
        if(is_poisoned()) {
            return ::ox::Err(PoisonError<T *const>(&data));
        }
        return ::ox::Ok<T *const>(&data);
    }

    LockResult<T> into_inner() && {
        if(is_poisoned()) {
            return ::ox::Err(PoisonError<T>(ox::move(data)));
        }
        return ::ox::Ok(ox::move(data));
    }
};

} // namespace ox::sync

template <typename T> struct fmt::formatter<ox::sync::RwLock<T>> {
    constexpr auto parse(format_parse_context &context) { return context.begin(); }

    template <typename FormatterContext>
    auto format(const ox::sync::RwLock<T> &m, FormatterContext &ctx) {
        return fmt::format_to(ctx.out(), "Mutex {{ data: <locked> }}");
    }
};

template <typename T> struct fmt::formatter<ox::sync::RwLockReadGuard<T>> {
    constexpr auto parse(format_parse_context &context) { return context.begin(); }

    template <typename FormatterContext>
    auto format(const ox::sync::RwLockReadGuard<T> &m, FormatterContext &ctx) {
        return fmt::format_to(ctx.out(), "{}", *m);
    }
};
template <typename T> struct fmt::formatter<ox::sync::RwLockWriteGuard<T>> {
    constexpr auto parse(format_parse_context &context) { return context.begin(); }

    template <typename FormatterContext>
    auto format(const ox::sync::RwLockWriteGuard<T> &m, FormatterContext &ctx) {
        return fmt::format_to(ctx.out(), "{}", *m);
    }
};