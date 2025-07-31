#pragma once

#include "../core/Types.hpp"

namespace ox::alloc {

constexpr bool is_power_of_two(usize x) {
    return x != 0 && (x & (x - 1)) == 0;
}

struct Layout {
    usize m_size;
    usize m_align;

  public:
    template <typename T> static constexpr Layout for_value() {
        return make_with_align<alignof(T)>(sizeof(T)).pad_to_align();
    }

    template <usize Align> static constexpr Layout make_with_align(usize size) {
        static_assert(is_power_of_two(Align), "Alignment must be a power of two");
        return Layout(size, Align);
    }

    constexpr Layout(usize size, usize align) : m_size(size), m_align(align) {}

    constexpr usize size() const noexcept { return m_size; }
    constexpr usize align() const noexcept { return m_align; }

    constexpr Layout pad_to_align() const {
        return Layout((m_size + m_align - 1) & ~(m_align - 1), m_align);
    }

    constexpr Layout extend(const Layout& next) const {
        usize new_align = m_align > next.m_align ? m_align : next.m_align;

        usize padding = (next.m_align - (m_size % next.m_align)) % next.m_align;
        usize new_size = m_size + padding + next.m_size;

        return Layout(new_size, new_align);
    }

    constexpr Layout repeat(usize count) const {
        if (count == 0)
            return Layout(0, m_align);

        usize stride = (m_size + m_align - 1) & ~(m_align - 1); // align up
        usize total_size = stride * count;

        return Layout(total_size, m_align);
    }
};

} // namespace ox::alloc