#pragma once
#include "Oxidize/alloc/Allocator.hpp"
#include "Oxidize/alloc/Layout.hpp"
#include "Oxidize/ptr/NonNull.hpp"
#include "Oxidize/slice/Slice.hpp"
#include <algorithm>
#include <memory>
#include <utility>

namespace ox {

/// Represents a raw vector that can be used to manage memory for a vector-like structure.
/// It's a low-level structure that does not manage the length of the vector and does not guarantee safety.
///
/// ## Safety
/// You must ensure that the elements are properly created and destructed using the provided methods.
/// You must ensure in-bound access to the elements.
template <typename T, alloc::Allocator A = alloc::Global> struct RawVec {
  public:
    ptr::NonNull<T> m_ptr;
    usize m_cap;

    alloc::Alloc<A> m_alloc;

  public:
    RawVec() : m_ptr(ptr::NonNull<T>::dangling()), m_cap(0), m_alloc(alloc::Alloc<A>()) {}
    RawVec(ptr::NonNull<T> ptr, usize capacity, alloc::Alloc<A> alloc)
        : m_ptr(ptr), m_cap(capacity), m_alloc(alloc) {}

    RawVec(RawVec<T, A> &&other)
        : m_ptr(std::exchange(other.m_ptr, ptr::NonNull<T>::dangling())),
          m_cap(std::exchange(other.m_cap, 0)),
          m_alloc(std::exchange(other.m_alloc, alloc::Alloc<A>())) {}
          
    RawVec &operator=(RawVec &&other) noexcept {
        if (this != &other) {
            if (m_ptr) {
                if (m_cap > 0) {
                    let layout = alloc::Layout::for_value<T>().repeat(m_cap);
                    m_alloc.deallocate(m_ptr.template cast<void>(), layout);
                }
            }

            m_ptr = std::exchange(other.m_ptr, ox::ptr::NonNull<T>::dangling());
            m_cap = std::exchange(other.m_cap, 0);
            m_alloc = std::exchange(other.m_alloc, alloc::Alloc<A>());
        }
        return *this;
    }

    RawVec(const RawVec<T> &) = delete;
    RawVec<T> &operator=(const RawVec<T> &) = delete;

    ~RawVec() {
        if (m_cap > 0 && !ptr::is_dangling(m_ptr.get())) {
            let layout = alloc::Layout::for_value<T>().repeat(m_cap);
            m_alloc.deallocate(m_ptr.template cast<void>(), layout);
        }
    }

    usize capacity() const { return m_cap; }
    T *as_ptr() const { return m_ptr.as_ptr(); }

    constexpr alloc::Alloc<A> &allocator() const noexcept { return m_alloc; }

    /// Grow the RawVec to contain at least `additional` more elements.
    void reserve(usize additional) {
        if (additional == 0)
            return;

        let new_cap = std::max(m_cap * 2, m_cap + additional);
        let new_layout = alloc::Layout::for_value<T>().repeat(new_cap);
        let old_layout = alloc::Layout::for_value<T>().repeat(m_cap);

        let result = m_alloc.grow(m_ptr, old_layout, new_layout).expect("RawVec::reserve");

        m_ptr = result.template cast<T>();
        m_cap = new_cap;
    }

    /// Shrinks the capacity of the RawVec to the specified minimum capacity.
    /// 
    /// ## Safety
    /// You must ensure that the excess elements are properly destructed before shrinking the RawVec.
    void shrink_to(usize min_capacity) {
        if (min_capacity >= m_cap)
            return;

        let old_layout = alloc::Layout::for_value<T>().repeat(m_cap);
        let new_layout = alloc::Layout::for_value<T>().repeat(min_capacity);

        let ptr = m_alloc.shrink(m_ptr, old_layout, new_layout).expect("RawVec::shrink_to");

        m_ptr = ptr.template cast<T>();
        m_cap = min_capacity;
    }

    /// Returns a reference to the element at the specified index.
    /// 
    /// ## Safety
    /// You must ensure that the index is within bounds and constructed before accessing it.
    const T &operator[](usize i) const {
        std::cout << m_ptr.get()[i] << " " << i << " " << m_cap << std::endl;
        return m_ptr.get()[i];
    }
    /// Returns a reference to the element at the specified index.
    /// 
    /// ## Safety
    /// You must ensure that the index is within bounds and constructed before accessing it.
    T &operator[](usize i) {
        std::cout << m_ptr.get()[i] << " " << i << " " << m_cap << std::endl;
        return m_ptr.get()[i];
    }

    /// Inserts a new element at the specified index.
    void insert_at(usize i, const T &value) {
        new (m_ptr + i) T(value);
    }
    /// Inserts a new element at the specified index.
    void insert_at(usize i, T &&value) {
        new (m_ptr + i) T(::ox::move(value));
    }

    /// Destroy the element at the specified index.
    /// 
    /// ## Safety
    /// You must ensure that the index is within bounds and constructed before destructing it.
    void destroy_at(usize i) {
        std::destroy_at<T>(m_ptr + i);
    }

    /// Moves the element from the specified index to the nex index.
    /// 
    /// ## Safety
    /// You must ensure that both indexes are within bounds.
    /// The element at `from` must be valid.
    /// Any element at `to` will be overwritten, destruct it if necessary.
    void move_assign(usize from, usize to) {
        m_ptr[to] = ::ox::move(m_ptr[from]);
        std::destroy_at<T>(m_ptr + from);
    }

    /// Moves the element from the specified index to the nex index.
    /// 
    /// ## Safety
    /// You must ensure that both indexes are within bounds.
    /// The element at `from` must be valid.
    /// Any element at `to` will be overwritten, destruct it if necessary.
    void move_construct(usize from, usize to) {
        new (m_ptr + to) T(::ox::move(m_ptr[from]));
        std::destroy_at<T>(m_ptr + from);
    }

    Slice<T> slice(usize start, usize len) {
        return Slice<T>(m_ptr.get() + start, len);
    }
};

} // namespace ox