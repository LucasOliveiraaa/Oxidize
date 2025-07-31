#pragma once
#include "../core/Types.hpp"
#include "../panic/Unwind.hpp"

namespace ox::ptr {

template <typename T> bool is_dangling(T *ptr) {
    return ptr == reinterpret_cast<T *>(uintptr_t(alignof(T)));
}

template <typename T> struct NonNull {
    T *m_ptr;

  public:
    NonNull(T *p) : m_ptr(p) {
        if (p == nullptr)
            panic("NonNull constructed will nullptr");
    }

    NonNull(const NonNull<T> &) = default;
    NonNull<T> &operator=(const NonNull<T> &) = default;
    NonNull(NonNull<T> &&) = default;
    NonNull<T> &operator=(NonNull<T> &&) = default;

    static NonNull<T> dangling() {
        return NonNull<T>(reinterpret_cast<T *>(uintptr_t(alignof(T))));
    }

    template <typename U> NonNull<U> cast() const {
        return NonNull<U>(reinterpret_cast<U *>(m_ptr));
    }

    constexpr T *as_ptr() const noexcept { return m_ptr; }
    constexpr T *get() const noexcept { return m_ptr; }
    constexpr T *operator->() const noexcept { return m_ptr; }

    constexpr operator T *() const noexcept { return m_ptr; }

    T *operator+(usize offset) const noexcept { return m_ptr + offset; }
    template <typename U = T>
    std::enable_if_t<!std::is_void_v<U>, U &> operator[](usize index) const noexcept {
        return m_ptr[index];
    }
};

} // namespace ox::ptr

template <typename T> struct std::formatter<ox::ptr::NonNull<T>> {
    constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const ox::ptr::NonNull<T> &v, format_context &ctx) const {
        return std::format_to(ctx.out(), "NonNull {{ ptr: {:#x} }}", (ox::u64) v.m_ptr);
    }
};