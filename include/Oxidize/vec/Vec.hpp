#pragma once

#include "../alloc/Allocator.hpp"
#include "../core/Fn.hpp"
#include "../iter/Iterator.hpp"
#include "../core/OptRes.hpp"
#include "../util/Range.hpp"
#include "../core/Move.hpp"
#include "../core/Types.hpp"
#include "../slice/Slice.hpp"
#include "Oxidize/ptr/NonNull.hpp"
#include "Oxidize/traits/Traits.h"
#include <type_traits>

namespace ox {

template <typename T, alloc::Allocator A = ox::alloc::Global> struct Vec {
    ptr::NonNull<T> m_ptr;
    usize m_len;
    usize m_cap;

    alloc::Alloc<A> m_alloc;

  public:
    Vec(ptr::NonNull<T> ptr, usize cap, usize len, alloc::Alloc<A> alloc)
        : m_ptr(ptr), m_cap(cap), m_len(len), m_alloc(std::move(alloc)) {}
    Vec(Vec &&other)
        : m_ptr(std::exchange(other.m_ptr, ox::ptr::NonNull<T>::dangling())),
          m_len(std::exchange(other.m_len, 0)), m_cap(std::exchange(other.m_cap, 0)),
          m_alloc(std::exchange(other.m_alloc, alloc::Alloc<A>())) {}

    Vec &operator=(Vec &&other) noexcept {
        if (this != &other) {
            if (m_ptr) {
                if (m_cap > 0) {
                    let layout = alloc::Layout::for_value<T>().repeat(m_cap);
                    m_alloc.deallocate(m_ptr.template cast<void>(), layout);
                }
            }

            m_ptr = std::exchange(other.m_ptr, ox::ptr::NonNull<T>::dangling());
            m_len = std::exchange(other.m_len, 0);
            m_cap = std::exchange(other.m_cap, 0);
            m_alloc = std::exchange(other.m_alloc, alloc::Alloc<A>());
        }
        return *this;
    }

    Vec(const Vec<T> &) = delete;
    Vec<T> &operator=(const Vec<T> &) = delete;

    ~Vec() {
        for (usize i = 0; i < m_len; i++) {
            m_ptr[i].~T();
        }
        if (m_cap > 0) {
            let full = alloc::Layout::for_value<T>().repeat(m_cap);
            m_alloc.deallocate(m_ptr.get(), full);
        }
    }

    constexpr alloc::Alloc<A> &allocator() const noexcept { return m_alloc; }

    bool operator==(const Vec<T, A> &other) const { return as_slice() == other.as_slice(); }
    bool operator!=(const Vec<T, A> &other) const { return as_slice() != other.as_slice(); }

    static Vec<T> from(const Slice<T> slice) {
        mut vec = Vec::with_capacity(slice.len());
        vec.extend_from_slice(slice);
        return vec;
    }

    static Vec<T> new_() {
        return move(Vec<T>(ptr::NonNull<T>::dangling(), 0, 0, alloc::Alloc<A>()));
    }
    static Vec<T> with_capacity(usize cap) {
        mut vec = Vec<T>::new_();
        vec.reserve(cap);
        return move(vec);
    }
    static Vec<T> from_iter(iter::Iter<T> iter)
        requires(trait::Copy<T> || trait::Clone<T>)
    {
        mut vec = Vec<T>::new_();
        vec.reserve(iter.len());
        for (let el : iter) {
            if constexpr (trait::Copy<T>) {
                vec.push(el);
            } else {
                vec.push(el.clone());
            }
        }
        return move(vec);
    }

    static Vec<T> new_in(A &alloc) {
        return move(Vec<T>(ptr::NonNull<T>::dangling(), 0, 0, alloc::Alloc<A>(alloc)));
    }
    static Vec<T> with_capacity_in(usize cap, A &alloc) {
        mut vec = Vec<T, A>::new_in(alloc);
        vec.reserve(cap);
        return move(vec);
    }
    static Vec<T> from_iter_in(iter::Iter<T> iter, A &alloc)
        requires(trait::Copy<T> || trait::Clone<T>)
    {
        mut vec = Vec<T, A>::new_in(alloc);
        vec.reserve(iter.len());
        for (let el : iter) {
            if constexpr (trait::Copy<T>) {
                vec.push(el);
            } else {
                vec.push(el.clone());
            }
        }
        return move(vec);
    }

    void reserve(usize additional) {
        if (additional == 0)
            return;

        let new_cap = m_cap + additional;
        let new_layout = alloc::Layout::for_value<T>().repeat(new_cap);
        let old_layout = alloc::Layout::for_value<T>().repeat(m_cap);

        let result = m_alloc.grow(m_ptr, old_layout, new_layout).expect("Vec::reserve");

        m_ptr = result.template cast<T>();
        m_cap = new_cap;
    }

    void shrink_to(usize min_capacity) {
        if (min_capacity >= m_cap)
            return;

        if (min_capacity < m_len) {
            truncate(min_capacity);
        }

        let old_layout = alloc::Layout::for_value<T>().repeat(m_cap);
        let new_layout = alloc::Layout::for_value<T>().repeat(min_capacity);

        let ptr = m_alloc.shrink(m_ptr, old_layout, new_layout).expect("Vec::shrink_to");

        m_ptr = ptr.template cast<T>();
        m_cap = min_capacity;
    }
    void shrink_to_fit() { shrink_to(m_len); }

    void clear() { truncate(0); }

    void resize(uint new_len, T value) {
        if (new_len == m_len)
            return;
        if (new_len < m_len) {
            truncate(new_len);
        } else {
            reserve(new_len - m_len);
            for (usize i = m_len; i < new_len; i++) {
                new (m_ptr + i) T(value);
            }
            m_len = new_len;
        }
    }

    T remove(usize i) {
        if (i >= m_len)
            panic("index out of bounds: the len is {} but the index is {}", m_len, i);

        mut el = move(m_ptr[i]);
        m_ptr[i].~T();
        for (usize j = i + 1; j < m_len; j++) {
            mut temp = move(m_ptr[j]);
            m_ptr[j].~T();
            new (m_ptr + j - 1) T(temp);
        }
        m_len--;
        return move(el);
    }
    T swap_remove(usize i) {
        if (i >= m_len)
            panic("index out of bounds: the len is {} but the index is {}", m_len, i);

        mut el = move(m_ptr[i]);
        m_ptr[i].~T();

        if (i <= m_len - 1) {
            mut temp = move(m_ptr[m_len - 1]);
            m_ptr[m_len - 1].~T();
            new (m_ptr + i) T(move(temp));
        }
        m_len--;

        return move(el);
    }

    template <typename F>
        requires Callable<F, bool, const T &>
    void retain(F &&f) {
        usize i = 0;
        while (i < m_len) {
            if (!f(m_ptr[i])) {
                remove(i);
            } else {
                i++;
            }
        }
    }
    template <typename F>
        requires Callable<F, bool, T &>
    void retain_mut(F &&f) {
        usize i = 0;
        while (i < m_len) {
            if (!f(m_ptr[i])) {
                remove(i);
            } else {
                i++;
            }
        }
    }

    template <typename F>
        requires Callable<F, bool, const T &>
    Vec<T> filter(F &&predicate) {
        Vec<T> res;
        for (let el : *this) {
            if (CALL_FN(F, predicate, el))
                res.push(move(el));
        }
        return move(res);
    }
    template <typename U, typename F>
        requires Callable<F, Option<U>, const T &>
    Vec<U> filter_map(F &&predicate) {
        Vec<U> res;
        for (let el : *this) {
            let new_el = CALL_FN(F, predicate, el);
            if (new_el.is_some)
                res.push(move(new_el.unwrap()));
        }
        return move(res);
    }

    template <typename B, typename F>
        requires Callable<F, B, B, const T &>
    B fold(B init, F &&f) {
        mut acc = init;
        for (let el : *this) {
            acc = CALL_FN(F, f, acc, el);
        }
        return move(acc);
    }
    template <typename F>
        requires Callable<F, T, T, const T &>
    T reduce(F &&f) {
        if (m_len == 0)
            return Default<T>::default_();
        mut acc = m_ptr[0];
        for (usize i = 1; i < m_len; i++) {
            acc = CALL_FN(F, f, acc, m_ptr[i]);
        }
        return move(acc);
    }

    void truncate(usize len) {
        if (len >= m_len)
            return;
        for (usize i = len; i < m_len; i++) {
            m_ptr[i].~T();
        }
        m_len = len;
    }

    void push(const T &value)
        requires trait::Copy<T>
    {
        if (m_len == m_cap) {
            reserve(m_cap == 0 ? 8 : m_cap);
        }
        new (m_ptr + m_len) T(value);
        m_len++;
    }
    void push(T &&value) {
        if (m_len == m_cap) {
            reserve(m_cap == 0 ? 8 : m_cap);
        }
        new (m_ptr + m_len) T(move(value));
        m_len++;
    }

    Option<T> pop() & {
        if (m_len == 0)
            return None;
        m_len--;
        mut val = move(m_ptr[m_len]);
        m_ptr[m_len].~T();
        return Some(move(val));
    }
    template <typename F>
        requires Callable<F, bool, T &>
    Option<T> pop_if(F &&f) {
        if (m_len == 0)
            return None;

        let res = std::invoke(std::forward<F>(f), m_ptr[m_len - 1]);
        if (res)
            return pop();

        return None;
    }

    void append(Vec<T> &other) {
        reserve(other.len());
        let other_len = other.len();
        for (usize i = 0; i < other.len(); i++) {
            mut temp = move(other[i]);
            other[i].~T();
            new (m_ptr + m_len + i) T(temp);
        }
        other.m_len = 0;
        m_len += other_len;
    }

    const T &operator[](usize i) const {
        if (i >= m_len)
            panic("index out of bounds: the len is {} but the index is {}", m_len, i);
        return m_ptr[i];
    }
    T &operator[](usize i) {
        if (i >= m_len)
            panic("index out of bounds: the len is {} but the index is {}", m_len, i);
        return m_ptr[i];
    }

    Vec<T> clone() const {
        mut vec = Vec::with_capacity(m_cap);
        vec.extend_from_slice(as_slice());
        return vec;
    }

    void extend_from_slice(const Slice<T> slice)
        requires(trait::Copy<T> || trait::Clone<T>)
    {
        if (m_cap < m_len + slice.len()) {
            reserve((m_len + slice.len()) - m_cap);
        }

        for (let &el : slice) {
            if constexpr (trait::Copy<T>) {
                push(el);
            } else {
                push(el.clone());
            }
        }
    }

    const Slice<T> as_slice() const { return Slice<T>(m_ptr, m_len); }
    Slice<T> as_slice_mut() { return Slice<T>(m_ptr, m_len); }

    const Slice<T> operator[](Range<usize> range) const {
        if (range.start == std::numeric_limits<usize>::max()) {
            return as_slice();
        }

        if (range.start >= m_len)
            panic("index out of bounds: the len is {} but the index is {}", m_len, range.start);
        if (range.end > m_len)
            panic("index out of bounds: the len is {} but the index is {}", m_len, range.end);

        return Slice<T>(m_ptr.get() + range.start, range.end - range.start);
    }
    Slice<T> operator[](Range<usize> range) {
        if (range.start == std::numeric_limits<usize>::max()) {
            return as_slice_mut();
        }

        if (range.start >= m_len)
            panic("index out of bounds: the len is {} but the index is {}", m_len, range.start);
        if (range.end > m_len)
            panic("index out of bounds: the len is {} but the index is {}", m_len, range.end);

        return Slice<T>(m_ptr.get() + range.start, range.end - range.start);
    }

    usize len() const { return m_len; }
    usize capacity() const { return m_cap; }

    iter::Iter<T> iter() const { return iter::Iter<T>(m_ptr.get(), m_len); }
    iter::IterMut<T> iter_mut() { return iter::IterMut<T>(m_ptr.get(), m_len); }
};

template <typename T> void vec_helper(Vec<T> &vec, const T &last) {
    vec.push(last);
}
template <typename T, typename... Ts> void vec_helper(Vec<T> &vec, T first, Ts... rest) {
    static_assert((std::is_same_v<T, Ts> && ...), "Mismatched types");
    vec.push(first);
    vec_helper(vec, rest...);
}

template <typename T, typename... Ts> Vec<T> vec(const T &first, const Ts &...rest) {
    static_assert((std::is_same_v<T, Ts> && ...), "Mismatched types");
    mut vec = Vec<T>::new_();
    vec_helper(vec, first, rest...);
    return move(vec);
}
template <typename T> Vec<T> vec() {
    mut vec = Vec<T>::new_();
    return vec;
}

} // namespace ox

template <typename T> struct std::formatter<ox::Vec<T>> {
    constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const ox::Vec<T> &v, format_context &ctx) const {
        std::string res = "";
        for (mut i = 0; i < v.len(); i++) {
            if (i == 0) {
                res = std::format("{}", v[i]);
            } else {
                res = std::format("{}, {}", res, v[i]);
            }
        }
        return std::format_to(ctx.out(), "[{}]", res);
    }
};