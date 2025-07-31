#pragma once
#include "../core/OptRes.hpp"
#include "../core/Types.hpp"
#include <type_traits>

namespace ox {

template <typename Idx = usize> struct Range {
    static_assert(std::is_integral_v<Idx>, "Idx must be an integer value");

  public:
    Idx start;
    Idx end;

    Range() : start(Default<Idx>::max()), end(Default<Idx>::max()) {}
    Range(Idx start, Idx end) : start(start), end(end) {}

    constexpr bool contains(const Idx& a) const { return a >= start && a < end; }
    constexpr bool is_empty() const { return start >= end; }

    struct RangeIter {
        Idx cur;
        Idx stop;

        bool next(Idx& out) {
            if (cur >= stop)
                return false;
            out = cur++;
            return true;
        }

        Option<Idx> next() { return cur < stop ? Some(cur++) : None; }

        bool is_empty() const { return cur >= stop; }
        usize remaining() const { return static_cast<usize>(stop - cur); }

        Idx operator*() const { return cur; }
        RangeIter& operator++() {
            ++cur;
            return *this;
        }
        bool operator!=(const RangeIter& other) const { return cur != other.cur; }

        constexpr RangeIter begin() const { return *this; }
        constexpr RangeIter end() const { return RangeIter{stop, stop}; }
    };

    RangeIter iter() const { return RangeIter{start, end}; }
};

template <typename Idx = usize> Range<Idx> range(Idx s, Idx e) {
    return Range(s, e);
}
template <typename Idx = usize> Range<Idx> range() {
    return Range<Idx>();
}

} // namespace ox