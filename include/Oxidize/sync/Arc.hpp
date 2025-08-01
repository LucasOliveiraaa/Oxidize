#pragma once
#include "Oxidize/alloc/Allocator.hpp"
#include "Oxidize/alloc/Layout.hpp"
#include "Oxidize/boxed/Box.hpp"
#include "Oxidize/core/Types.hpp"
#include "Oxidize/ptr/NonNull.hpp"
#include "Oxidize/util/Print.hpp"
#include <atomic>

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#endif

namespace ox::sync {

template <typename T> struct ArcInner {
    std::atomic<usize> strong;
    std::atomic<usize> weak;

    T data;

    ArcInner(T &&data) : data(move(data)), strong(1), weak(1) {}
    ArcInner(ArcInner &&other) noexcept : strong(1), weak(1), data(std::move(other.data)) {}
};

template <typename T, alloc::Allocator A> struct Weak {
    ptr::NonNull<ArcInner<T>> ptr;
    alloc::Alloc<A> alloc;

  public:
    Weak(ptr::NonNull<ArcInner<T>> ptr, alloc::Alloc<A> alloc) : ptr(ptr), alloc(alloc) {}
    ~Weak() {
        let old = ptr->weak.fetch_sub(1, std::memory_order_release);
        println("~Weak() = Old({})", old);
        if (old == 1) {
            std::atomic_thread_fence(std::memory_order_acquire);
            alloc.deallocate(ptr.template cast<void>(), alloc::Layout::for_value<ArcInner<T>>());
        }
    }

    Weak<T, A> clone() const {
        ptr->weak.fetch_add(1, std::memory_order_relaxed);

        return Weak(ptr, alloc);
    }

    T *operator->() const { return &ptr->data; }
};

template <typename T, alloc::Allocator A = alloc::Global> struct Arc {
    ptr::NonNull<ArcInner<T>> ptr;
    alloc::Alloc<A> alloc;

  public:
    Arc(ptr::NonNull<ArcInner<T>> ptr, alloc::Alloc<A> alloc) : ptr(ptr), alloc(alloc) {}
    ~Arc() {
        if (ptr::is_dangling(ptr.as_ptr()))
            return;

        if (ptr->strong.fetch_sub(1, std::memory_order_release) != 1) {
            return;
        }

        std::atomic_thread_fence(std::memory_order_acquire);

        let weak_ = Weak<T, A>(ptr, alloc);
        std::destroy_at(&ptr->data);
    }

    Arc(Arc &&other) noexcept : ptr(other.ptr), alloc(std::move(other.alloc)) {
        other.ptr = ptr::NonNull<ArcInner<T>>::dangling();
    }

    Arc &operator=(Arc &&other) noexcept {
        if (this != &other) {
            this->~Arc(); // destroy current
            new (this) Arc(std::move(other));
        }
        return *this;
    }

    Arc(const Arc &) = delete;
    Arc &operator=(const Arc &) = delete;

    static Arc<T, A> from_inner(ptr::NonNull<ArcInner<T>> ptr) { return from_inner_in(ptr, A()); }
    static Arc<T, A> from_inner_in(ptr::NonNull<ArcInner<T>> ptr, const A &alloc) {
        return Arc(ptr, alloc::Alloc<A>(alloc));
    }

    static Arc<T, A> new_in(T data, const A &alloc) {
        mut box = boxed::Box<ArcInner<T>, A>::new_in(ArcInner<T>(move(data)), alloc);
        return from_inner_in(box.leak(), alloc);
    }
    static Arc<T, A> new_(T data) { return new_in(data, A()); }

    Weak<T, A> downgrade() const {
        mut cur = ptr->weak.load(std::memory_order_relaxed);

        while (true) {
            if (cur == Default<usize>::max()) [[unlikely]] {
#if defined(__x86_64__) || defined(_M_X64)
                _mm_pause();
#elif defined(__aarch64__)
                __asm__ volatile("yield");
#endif
                cur = ptr->weak.load(std::memory_order_relaxed);
                continue;
            }

            if (ptr->weak.compare_exchange_weak(
                    cur, cur + 1, std::memory_order_acquire, std::memory_order_relaxed)) {
                return Weak(ptr, alloc);
            }
        }
    }

    Arc<T, A> clone() const {
        ptr->strong.fetch_add(1, std::memory_order_relaxed);

        return Arc(ptr, alloc);
    }

    usize strong_count() const { return this->ptr->strong.load(std::memory_order_relaxed); }

    usize weak_count() const {
        let cnt = this->ptr->weak.load(std::memory_order_relaxed);
        return cnt == Default<usize>::max() ? 0 : cnt - 1;
    }

    Result<T, Arc<T, A>> try_unwrap() && {
        if (!ptr->strong.compare_exchange_strong(
                1, 0, std::memory_order_relaxed, std::memory_order_relaxed)) {
            return Err(move(*this));
        }

        std::atomic_thread_fence(std::memory_order_acquire);

        let weak_ = Weak(ptr, alloc);
        return move(ptr->data);
    }

    T unwrap_or_clone() && requires(trait::Copy<T>) || (trait::Clone<T>) {
        return move(*this).try_unwrap().unwrap_or_else([](Arc<T, A> arc) {
            if constexpr (trait::Copy<T>) {
                return arc.ptr->data;
            } else {
                return arc.ptr.data.clone();
            }
        });
    }

    T *get() const { return &ptr->data; }
    T *operator->() const { return &ptr->data; }
};

} // namespace ox::sync