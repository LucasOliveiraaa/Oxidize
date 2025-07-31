#pragma once
#include "../core/OptRes.hpp"
#include "../core/Types.hpp"
#include "Layout.hpp"
#include "../ptr/NonNull.hpp"
#include "Oxidize/traits/Traits.h"
#include <concepts>
#include <cstring>
#include <new>

namespace ox::alloc {

enum class AllocError {
    BadAlloc,
    ZeroLen
};

template <typename Self>
concept Allocator = requires(Self self, Layout layout, ptr::NonNull<void> ptr) {
    requires trait::Trait<Self>;
    { self.allocate(layout) } -> std::same_as<Result<ptr::NonNull<void>, AllocError>>;
    { self.deallocate(ptr, layout) };
};

template <Allocator Self> struct Alloc {
    Self m_impl;

    Alloc() : m_impl() {}
    Alloc(Self impl) : m_impl(impl) {}

    Result<ptr::NonNull<void>, AllocError> allocate(Layout layout) {
        return m_impl.allocate(layout);
    }

    void deallocate(ptr::NonNull<void> ptr, Layout layout) { m_impl.deallocate(ptr, layout); }

    template <typename T>
        requires(Self::Provide)
    Result<ptr::NonNull<T>, AllocError> grow(
        ptr::NonNull<T> old_ptr, Layout old_layout, Layout new_layout) {
        mut new_mem = TRY(m_impl.allocate(new_layout)).template cast<T>();
        if (old_layout.size() == 0)
            return Ok(move(new_mem));

        T* new_ptr = new_mem.get();
        T* old_raw = old_ptr.get();

        if constexpr (trait::TrivialCopy<T>) {
            std::memcpy(new_ptr, old_raw, old_layout.size());
        } else {
            for (usize i = 0; i < old_layout.size() / sizeof(T); ++i) {
                auto temp = move(old_raw[i]);
                old_raw[i].~T();
                new (new_ptr + i) T(temp);
            }
        }

        m_impl.deallocate(old_ptr.template cast<void>(), old_layout);

        return Ok(move(new_mem));
    }

    template <typename T>
        requires(Self::Provide)
    Result<ox::ptr::NonNull<T>, AllocError> shrink(
        ox::ptr::NonNull<T> old_ptr, Layout old_layout, Layout new_layout) {
        mut new_mem = TRY(m_impl.allocate(new_layout)).template cast<T>();

        T* new_ptr = new_mem.get();
        T* old_raw = old_ptr.get();

        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(new_ptr, old_raw, new_layout.size());
        } else {
            let new_count = new_layout.size() / sizeof(T);
            let old_count = old_layout.size() / sizeof(T);

            for (usize i = 0; i < new_count; ++i) {
                mut temp = move(old_raw[i]);
                old_raw[i].~T();
                new (new_ptr + i) T(temp);
            }
            for (usize i = new_count; i < old_count; ++i) {
                old_raw[i].~T();
            }
        }
        m_impl.deallocate(old_ptr.template cast<void>(), old_layout);

        return Ok(move(new_mem));
    }
};

struct Global {
    static constexpr bool Provide = true;

    Global() {}

    Result<ptr::NonNull<void>, AllocError> allocate(Layout layout) {
        try {
            void* data = ::operator new(layout.size(), std::align_val_t(layout.align()));
            return Ok(ox::ptr::NonNull<void>(data));
        } catch (std::bad_alloc&) {
            return Err(AllocError::BadAlloc);
        }
    }

    void deallocate(ptr::NonNull<void> ptr, Layout layout) {
        ::operator delete(ptr.get(), std::align_val_t(layout.align()));
    }
};
static_assert(Allocator<Global>);

}; // namespace ox::alloc

template <> struct std::formatter<ox::alloc::AllocError> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    auto format(const ox::alloc::AllocError& v, format_context& ctx) const {
        switch (v) {
        case ox::alloc::AllocError::BadAlloc: {
            return std::format_to(ctx.out(), "Bad Allocator");
            break;
        }
        case ox::alloc::AllocError::ZeroLen: {
            return std::format_to(ctx.out(), "Zero Length");
            break;
        }
        }
        return std::format_to(ctx.out(), "Unknown Allocation Error");
    }
};