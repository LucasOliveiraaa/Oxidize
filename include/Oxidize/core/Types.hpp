#pragma once
#include <fmt/format.h>
#include <limits>
#include <string>
#include <type_traits>

namespace ox {

using RawStr = const char*;
using RawString = std::string;

using i64 = long;
using i32 = int;
using i16 = short;
using i8 = signed char;

using usize = unsigned long;
using u64 = usize;
using u32 = unsigned int;
using u16 = unsigned short;
using u8 = unsigned char;

template <typename T> struct Default {
    template <typename U = T, std::enable_if_t<std::is_default_constructible_v<U>, int> = 0>
    static U default_() noexcept(std::is_nothrow_default_constructible_v<U>) {
        return U();
    }

    template <typename U = T, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
    static constexpr U min() noexcept {
        return std::numeric_limits<U>::min();
    }

    template <typename U = T, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
    static constexpr U max() noexcept {
        return std::numeric_limits<U>::max();
    }

    template <typename U = T, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
    static constexpr U epsilon() noexcept {
        return std::numeric_limits<U>::epsilon();
    }

    template <typename U = T, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
    static constexpr U infinity() noexcept {
        return std::numeric_limits<U>::infinity();
    }
};

struct Void {
    enum _tag {};
};

#define let const auto
#define mut auto

}

template <> struct fmt::formatter<ox::Void> {
    constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

    auto format(const ox::Void& v, format_context& ctx) const {
        return fmt::format_to(ctx.out(), "()");
    }
};