#pragma once
#include "Oxidize/core/Forward.hpp"
#include "Oxidize/core/Move.hpp"
#include "Oxidize/panic/Unwind.hpp"
#include "Oxidize/sync/error.h"
#include <atomic>
#include <mutex>

namespace ox::sync {

template <typename T> struct MutexGuard {
    using LockType = std::unique_lock<std::mutex>;
    LockType lock;
    T *data;
    std::atomic<bool> *poisoned;

  public:
    MutexGuard(LockType lock, T *data, std::atomic<bool> *poisoned)
        : lock(ox::move(lock)), data(data), poisoned(poisoned) {}
    ~MutexGuard() {
        if (thread::panicking()) {
            poisoned->store(true, std::memory_order_release);
        }
    }

    MutexGuard(MutexGuard &&other) : lock(ox::move(other.lock)), data(other.data), poisoned(other.poisoned) {}
    MutexGuard &operator=(MutexGuard &&other) {
        if(this != &other) {
            this->lock = ox::move(other.lock);
            this->data = other.data;
            this->poisoned = other.poisoned;
        }
        return *this;
    }

    MutexGuard(const MutexGuard &) = delete;
    MutexGuard &operator=(const MutexGuard &) = delete;

    T *operator->() { return data; }
    T &operator*() { return *data; }
    const T *operator->() const { return data; }
    const T &operator*() const { return *data; }
};


/// [from Rust's Doc]
/// A mutual exclusion primitive useful for protecting shared data
///
/// This mutex will block threads waiting for the lock to become available. The
/// mutex can be created via a [`new_`] constructor. Each mutex has a type parameter
/// which represents the data that it is protecting. The data can only be accessed
/// through the RAII guards returned from [`lock`] and [`try_lock`], which
/// guarantees that the data is only ever accessed when the mutex is locked.
///
/// # Poisoning
///
/// The mutexes in this module implement a strategy called "poisoning" where a
/// mutex is considered poisoned whenever a thread panics while holding the
/// mutex. Once a mutex is poisoned, all other threads are unable to access the
/// data by default as it is likely tainted (some invariant is not being
/// upheld).
///
/// For a mutex, this means that the [`lock`] and [`try_lock`] methods return a
/// [`Result`] which indicates whether a mutex has been poisoned or not. Most
/// usage of a mutex will simply [`unwrap()`] these results, propagating panics
/// among threads to ensure that a possibly invalid invariant is not witnessed.
///
/// A poisoned mutex, however, does not prevent all access to the underlying
/// data. The [`PoisonError`] type has an [`into_inner`] method which will return
/// the guard that would have otherwise been returned on a successful lock. This
/// allows access to the data, despite the lock being poisoned.
template <typename T> struct Mutex {
    mutable T data;
    mutable std::mutex mutex;
    mutable std::atomic<bool> poisoned;

  public:
    using Ok = MutexGuard<T> &&;

    using Err = PoisonError<MutexGuard<T>> &&;
    using TryErr = TryLockError<MutexGuard<T>> &&;

    Mutex(T &&data) : data(std::forward<T>(data)), poisoned(false) {}

    Mutex(Mutex &&other) : data(ox::move(other.data)) {}
    Mutex &operator=(Mutex &&other) {
        if (this != &other) {
            this->data = ox::move(other.data);
        }
        return *this;
    }

    Mutex(const Mutex &) = delete;
    Mutex &operator=(const Mutex &) = delete;

    Mutex<T> new_(T &&data) {
        return Mutex<T>(forward<T>(data));
    }

    LockResult<MutexGuard<T>> lock() const {
        if (is_poisoned()) {
            return ::ox::Err(PoisonError<MutexGuard<T>>(
                MutexGuard<T>(std::unique_lock<std::mutex>(mutex), &data, &poisoned)));
        }
        return ::ox::Ok(MutexGuard<T>(std::unique_lock<std::mutex>(mutex), &data, &poisoned));
    }

    bool is_poisoned() const noexcept { return poisoned.load(std::memory_order_acquire); }

    TryLockResult<MutexGuard<T>> try_lock() const {
        if (mutex.try_lock()) {
            if (is_poisoned()) {
                return ::ox::Err(TryLockError<MutexGuard<T>>(MutexGuard<T>(
                    std::unique_lock<std::mutex>(mutex, std::adopt_lock), &data, &poisoned)));
            }
            return ::ox::Ok(MutexGuard<T>(std::unique_lock<std::mutex>(mutex, std::adopt_lock), &data, &poisoned));
        }
        return ::ox::Err(TryLockError<MutexGuard<T>>());
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

template <typename T> struct fmt::formatter<ox::sync::Mutex<T>> {
    constexpr auto parse(format_parse_context &context) { return context.begin(); }

    template <typename FormatterContext>
    auto format(const ox::sync::Mutex<T> &m, FormatterContext &ctx) {
        return fmt::format_to(ctx.out(), "Mutex {{ data: <locked> }}");
    }
};

template <typename T> struct fmt::formatter<ox::sync::MutexGuard<T>> {
    constexpr auto parse(format_parse_context &context) { return context.begin(); }

    template <typename FormatterContext>
    auto format(const ox::sync::MutexGuard<T> &m, FormatterContext &ctx) {
        return fmt::format_to(ctx.out(), "{}", *m);
    }
};