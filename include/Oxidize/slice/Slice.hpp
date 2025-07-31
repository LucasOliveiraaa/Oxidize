#pragma once
#include "../iter/Iterator.hpp"
#include "../core/OptRes.hpp"
#include "../panic/Unwind.hpp"
#include "../core/Types.hpp"
#include "Oxidize/ptr/NonNull.hpp"
#include <utility>

namespace ox {

template <typename T> struct Slice {
    T* m_ptr;
    usize m_len;

  public:
    Slice() : m_ptr(ptr::NonNull<T>::dangling()), m_len(0) {}
    Slice(const T* ptr, usize len) : m_ptr(const_cast<T *>(ptr)), m_len(len) {}
    Slice(iter::Iter<T> iter) : m_ptr(iter.begin()), m_len(iter.len()) {}

    static Slice<T> from(iter::Iter<T> iter) { return Slice(iter); }

    bool operator==(const Slice<T>& other) const {
        if (m_len != other.m_len)
            return false;
        for (usize i = 0; i < m_len; ++i) {
            if (!(m_ptr[i] == other.m_ptr[i]))
                return false;
        }
        return true;
    }

    bool operator!=(const Slice<T>& other) const { return !(*this == other); }

    bool contains(const T& x) const {
        for (usize i = 0; i < m_len; i++) {
            if (m_ptr[i] == x)
                return true;
        }
        return false;
    }
    bool is_empty() const { return m_len == 0; }

    bool starts_with(const Slice<T>& needle) const {
        if (needle.m_len > m_len)
            return false;
        for (usize i = 0; i < needle.m_len; i++) {
            if (m_ptr[i] != needle[i])
                return false;
        }
        return true;
    }
    bool ends_with(const Slice<T>& needle) const {
        if (needle.m_len > m_len)
            return false;
        for (usize i = 0; i < needle.m_len; ++i) {
            if (m_ptr[m_len - needle.m_len + i] != needle[i])
                return false;
        }
        return true;
    }

    constexpr usize len() const { return m_len; }

    Option<const T> first() const { return m_len > 0 ? Some(*m_ptr) : None; }
    Option<T> first_mut() { return m_len > 0 ? Some(*m_ptr) : None; }
    Option<const T> last() const { return m_len > 0 ? Some(m_ptr[m_len - 1]) : None; }
    Option<T> last_mut() { return m_len > 0 ? Some(m_ptr[m_len - 1]) : None; }

    Option<const T> get(usize i) const { return i >= m_len ? None : Some(m_ptr[i]); }
    Option<T> get_mut(usize i) { return i >= m_len ? None : Some(m_ptr[i]); }

    const T& operator[](usize i) const {
        if (i >= m_len)
            panic("index out of bounds: the len is {} but the index is {}", m_len, i);
        return m_ptr[i];
    }
    T& operator[](usize i) {
        if (i >= m_len)
            panic("index out of bounds: the len is {} but the index is {}", m_len, i);
        return m_ptr[i];
    }

    void reverse() {
        for (usize i = 0; i < m_len / 2; i++) {
            std::swap(m_ptr[i], m_ptr[m_len - i - 1]);
        }
    }

    void swap(usize a, usize b) {
        if (a == b)
            return;
        if (a >= m_len)
            panic("index out of bounds: the len is {} but the index is {}", m_len, a);
        if (b >= m_len)
            panic("index out of bounds: the len is {} but the index is {}", m_len, b);
        std::swap(m_ptr[a], m_ptr[b]);
    }

    Tuple<const Slice<T>, const Slice<T>> slice_at(usize mid) const {
        if (mid >= m_len)
            panic("index out of bounds: the len is {} but the index is {}", m_len, mid);

        return Tuple(Slice(m_ptr, mid), Slice(&m_ptr[mid], m_len - mid));
    }
    Tuple<Slice<T>, Slice<T>> slice_at_mut(usize mid) {
        if (mid >= m_len)
            panic("index out of bounds: the len is {} but the index is {}", m_len, mid);

        return Tuple(Slice(m_ptr, mid), Slice(&m_ptr[mid], m_len - mid));
    }

    Option<Tuple<const Slice<T>, const Slice<T>>> slice_at_checked(usize mid) const {
        return mid >= m_len ? None
                            : Some(Tuple(Slice(m_ptr, mid), Slice(&m_ptr[mid], m_len - mid)));
    }
    Option<Tuple<Slice<T>, Slice<T>>> slice_at_mut_checked(usize mid) {
        return mid >= m_len ? None
                            : Some(Tuple(Slice(m_ptr, mid), Slice(&m_ptr[mid], m_len - mid)));
    }

    Option<Tuple<const T&, const Slice<T>>> split_first() const {
        if (is_empty())
            return None;
        return Some(Tuple(m_ptr[0], Slice(&m_ptr[1], m_len - 1)));
    }
    Option<Tuple<T&, Slice<T>>> split_first_mut() {
        if (is_empty())
            return None;
        return Some(Tuple(m_ptr[0], Slice(&m_ptr[1], m_len - 1)));
    }

    Option<Tuple<const T&, const Slice<T>>> split_last() const {
        if (is_empty())
            return None;
        return Some(Tuple(m_ptr[m_len - 1], Slice(&m_ptr[0], m_len - 1)));
    }
    Option<Tuple<T&, Slice<T>>> split_last_mut() {
        if (is_empty())
            return None;
        return Some(Tuple(m_ptr[m_len - 1], Slice(&m_ptr[0], m_len - 1)));
    }

    iter::Iter<T> iter() const { return iter::Iter<T>(m_ptr, m_len); }
    iter::IterMut<T> iter_mut() { return iter::IterMut<T>(m_ptr, m_len); }

    const T* begin() const { return m_ptr; }
    const T* end() const { return m_ptr + m_len; }

    T* begin() { return m_ptr; }
    T* end() { return m_ptr + m_len; }
};

} // namespace ox