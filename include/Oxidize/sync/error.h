#pragma once
#include "Oxidize/core/Move.hpp"
#include "Oxidize/core/OptRes.hpp"
#include <utility>
#include "Oxidize/core/Match.hpp"

namespace ox::sync {

template <typename T> struct TryLockError {
    Option<T> value;

    TryLockError() : value(None) {}
    TryLockError(T &&data) : value(Some(std::forward<T>(data))) {}
};

template <typename T> struct PoisonError {
    T data;

  public:
    PoisonError(T &&data) : data(std::forward<T>(data)) {};

    constexpr T &get_mut() & noexcept { return data; }
    constexpr const T &get_ref() const & noexcept { return data; }
    constexpr T into_inner() && noexcept { return ox::move(data); }
};

template <typename T> using LockResult = Result<T, PoisonError<T>>;
template <typename T> using TryLockResult = Result<T, TryLockError<T>>;

} // namespace ox::sync

namespace ox {

template <typename T> struct Match<sync::TryLockError<T>> {
    sync::TryLockError<T> data;

    Match(sync::TryLockError<T> &&data) : data(std::move(data)) {}

    MATCH_FOR(poison, T, data.value.is_some(), ox::move(data.value).unwrap())
    MATCH_FOR_VOID(wouldBlock, data.value.is_none())
};
template <typename T> Match(sync::TryLockError<T>) -> Match<sync::TryLockError<T>>;

} // namespace ox

template <typename T> struct fmt::formatter<ox::sync::PoisonError<T>> {
    constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const ox::sync::PoisonError<T> &v, format_context &ctx) const {
        return fmt::format_to(ctx.out(), "poisoned lock: another task failed inside");
    }
};

template <typename T> struct fmt::formatter<ox::sync::TryLockError<T>> {
    constexpr auto parse(format_parse_context &context) { return context.begin(); }

    template <typename FormatterContext>
    auto format(const ox::sync::TryLockError<T> &m, FormatterContext &ctx) {
        if(m.value.is_some()) {
        return fmt::format_to(ctx.out(), "Poisoned");
        }else {
        return fmt::format_to(ctx.out(), "WouldBlock");
        }
    }
};