#pragma once
#include "../core/OptRes.hpp"
#include "../core/Types.hpp"

namespace ox::iter {

template <typename Self>
concept Iterator = requires (Self s, Self::ItemPtr m_ptr, usize &m_len, usize &m_pos) {
    typename Self::Item;
    typename Self::ItemRef;
    typename Self::ItemPtr;
    { s.next(m_ptr, m_len, m_pos) } -> std::same_as<Option<typename Self::Item>>;
}; 

template <Iterator Self> struct RawIterator {
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = Self::Item;

    using Item = typename Self::Item;
    using ItemRef = typename Self::ItemRef;
    using ItemPtr = typename Self::ItemPtr;

    ItemPtr m_ptr;
    usize m_len;
    usize m_pos;

    Self m_impl;

    RawIterator(ItemPtr ptr, usize len, usize pos = 0) : m_ptr(ptr), m_len(len), m_pos(pos), m_impl() {}

    RawIterator(const RawIterator&) = default;
    RawIterator& operator=(const RawIterator&) = default;

    RawIterator(RawIterator&&) = default;
    RawIterator& operator=(RawIterator&&) = default;

    void set_impl(Self impl) {
        m_impl = impl;
    }

    Option<Item> next() {
        return m_impl.next(m_ptr, m_len, m_pos);
    }

    constexpr usize len() const noexcept { return m_len; }

    bool is_empty() const { return m_pos >= m_len; }
    usize remaining() const { return m_len - m_pos; }

    template <typename B> B collect() const { return B(*this); }

    ItemPtr operator->() const {
        if (m_pos >= m_len)
            panic("Dereferencing past the end of Iter");
        return &m_ptr[m_pos];
    }
    ItemRef operator*() const {
        if (m_pos >= m_len)
            panic("Dereferencing past the end of Iter");
        return m_ptr[m_pos];
    }
    RawIterator operator++(int) {
        if (m_pos >= m_len)
            panic("Dereferencing past the end of Iter");
        RawIterator tmp = *this;
        m_pos++;
        return tmp;
    }
    RawIterator& operator++() {
        if (m_pos >= m_len)
            panic("Dereferencing past the end of Iter");
        m_pos++;
        return *this;
    }

    RawIterator operator--(int) {
        if (m_pos == 0)
            panic("Dereferencing past the start of Iter");
        RawIterator tmp = *this;
        --(*this);
        return tmp;
    }
    RawIterator& operator--() {
        if (m_pos == 0)
            panic("Dereferencing past the start of Iter");
        --m_pos;
        return *this;
    }
    bool operator==(const RawIterator& other) const { return m_pos == other.m_pos; }
    bool operator!=(const RawIterator& other) const { return m_pos != other.m_pos; }

    constexpr RawIterator begin() const { return RawIterator<Self>(m_ptr, m_len, 0); }
    constexpr RawIterator end() const { return RawIterator<Self>(m_ptr, m_len, m_len); }
};

template <typename T>
struct RawIter {
    using Item = T;
    using ItemRef = const Item &;
    using ItemPtr = const Item *;

    Option<Item> next(ItemPtr m_ptr, usize &m_len, usize &m_pos) {
        if(m_pos == m_len) return None;

        return Some(m_ptr[m_pos++]);
    }
};
template <typename T> using Iter = RawIterator<RawIter<T>>;

template <typename T>
struct RawIterMut {
    using Item = T;
    using ItemRef = Item &;
    using ItemPtr = Item *;

    Option<Item> next(ItemPtr m_ptr, usize &m_len, usize &m_pos) {
        if(m_pos == m_len) return None;

        return Some(m_ptr[m_pos++]);
    }
};
template <typename T> using IterMut = RawIterator<RawIterMut<T>>;

} // namespace ox::iter

template <ox::iter::Iterator Self> struct fmt::formatter<ox::iter::RawIterator<Self>> {
    constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

    auto format(const ox::iter::RawIterator<Self>& iter, format_context& ctx) const {
        std::string res = "";
        for (mut i = 0; i < iter.len(); i++) {
            if (i == 0) {
                res = fmt::format("{}", iter.m_ptr[i]);
            } else {
                res = fmt::format("{}, {}", res, iter.m_ptr[i]);
            }
        }
        return fmt::format_to(ctx.out(), "Iterator([{}])", res);
    }
};