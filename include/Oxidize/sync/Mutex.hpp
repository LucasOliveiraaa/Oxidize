#pragma once
#include "Oxidize/core/Move.hpp"
#include "Oxidize/core/OptRes.hpp"
#include "Oxidize/panic/Unwind.hpp"
#include <atomic>
#include <mutex>

namespace ox::sync {

enum class TryLockError {
    WouldBlock    
};

template <typename T> struct PoisonError {
    T data;

  public:
    PoisonError(T &&data) : data(std::forward<T>(data)) {};

    constexpr T &get_mut() & noexcept { return data; }
    constexpr const T &get_ref() const & noexcept { return data; }
    constexpr T into_inner() && noexcept { return ox::move(data); }
};

template <typename T> using LockResult = Result<T, PoisonError<T>>;
template <typename T> using TryLockResult = Result<T, TryLockError>;

template <typename T> struct MutexGuard {
    using LockType = std::unique_lock<std::mutex>;
    LockType lock;
    T *data;
    std::atomic<bool> &poisoned;

  public:
    MutexGuard(LockType &&lock, T *data, std::atomic<bool> &poisoned)
        : lock(std::forward<LockType>(lock)), data(data), poisoned(poisoned) {}
    ~MutexGuard() {
        if (thread::panicking()) {
            poisoned.store(true, std::memory_order_release);
        }
        lock.unlock();
    }

    T *operator->() { return data; }
    T &operator*() { return *data; }
    const T *operator->() const { return data; }
    const T &operator*() const { return *data; }
};

template <typename T> struct Mutex {
    T data;
    mutable std::mutex mutex;
    std::atomic<bool> poisoned;

  public:
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

    LockResult<MutexGuard<T>> lock() noexcept {
        if (is_poisoned()) {
            return Err(PoisonError<T>(MutexGuard<T>(std::unique_lock<std::mutex>(mutex), &data, poisoned)));
        }
        return Ok(MutexGuard<T>(std::unique_lock<std::mutex>(mutex), &data, poisoned));
    }

    bool is_poisoned() const noexcept { return poisoned.load(std::memory_order_acquire); }

    TryLockResult<MutexGuard<T>> try_lock() const {
        if(mutex.try_lock()) {
            
        }
    }
};

} // namespace ox::sync