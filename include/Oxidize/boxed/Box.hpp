#pragma once
#include "Oxidize/alloc/Allocator.hpp"
#include "Oxidize/alloc/Layout.hpp"
#include "Oxidize/ptr/NonNull.hpp"
#include "Oxidize/panic/Unwind.hpp"

namespace ox::boxed {

template <typename T, alloc::Allocator A = alloc::Global> struct Box {
    ptr::NonNull<T> ptr;
    alloc::Alloc<A> alloc;

  public:
    Box(ptr::NonNull<T> ptr, alloc::Alloc<A> alloc) : ptr(ptr), alloc(alloc) {}
    ~Box() {
        if (ptr::is_dangling(ptr.get()))
            return;
        T *raw_ptr = ptr.get();
        raw_ptr->~T();
        alloc.deallocate(ptr.template cast<void>(), ox::alloc::Layout::for_value<T>());
    }

    static Box<T, A> new_(T x) { return new_in(x, A()); }
    static Box<T, A> new_in(T x, const A &a) {
        mut alloc = alloc::Alloc<A>(a);

        let layout = alloc::Layout::for_value<T>().pad_to_align();
        let res = alloc.allocate(layout);
        if (res.is_err()) {
            panic("Failed allocating Box: {}", move(res).unwrap_err());
        }

        let raw = res.unwrap();
        T *typed_ptr = new (raw.get()) T(move(x));

        return Box<T, A>(ptr::NonNull<T>(typed_ptr), alloc);
    }

    constexpr ptr::NonNull<T> leak() {
        let ptr = this->ptr;
        this->ptr = ptr::NonNull<T>::dangling();
        return ptr;
    }

    constexpr T *as_ptr() const { return ptr.get(); }
};

} // namespace ox::boxed