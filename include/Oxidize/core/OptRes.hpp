#pragma once
#include "../either/Either.hpp"
#include "Fn.hpp"
#include "Oxidize/core/Move.hpp"
#include "Oxidize/traits/Traits.h"
#include "Tuple.hpp"
#include "Types.hpp"
#include <type_traits>
#include <utility>
#include "Match.hpp"

namespace ox {

#define TRY(expr)                                                                                  \
    ({                                                                                             \
        auto __res = (expr);                                                                       \
        if (!__res)                                                                                \
            return Err(ox::move(__res).unwrap_err());                                              \
        ox::move(__res).unwrap();                                                                  \
    })

#define RETURN_EARLY(expr)                                                                         \
    ({                                                                                             \
        auto __res = (expr);                                                                       \
        if (!__res)                                                                                \
            return;                                                                                \
        ox::move(__res).unwrap();                                                                  \
    })

template <typename T, typename E> struct Result;
template <typename T> struct Option;
template <typename T> struct OkValue;
template <typename T> struct ErrValue;

inline constexpr Void None = Void{};

template <typename T> either::LValue<T> Some(T &&v) {
    if constexpr (trait::Copy<T>) {
        return either::LValue<T>(v);
    } else {
        return either::LValue<T>(ox::move(v));
    }
}

template <typename> struct is_option : std::false_type {};
template <typename U> struct is_option<Option<U>> : std::true_type {};
template <typename T> constexpr bool is_option_v = is_option<T>::value;

template <typename> struct is_ok_value : std::false_type {};
template <typename U> struct is_ok_value<OkValue<U>> : std::true_type {};
template <typename T> constexpr bool is_ok_value_v = is_ok_value<T>::value;

template <typename> struct is_err_value : std::false_type {};
template <typename U> struct is_err_value<ErrValue<U>> : std::true_type {};
template <typename T> constexpr bool is_err_value_v = is_err_value<T>::value;

template <typename> struct is_result : std::false_type {};
template <typename T, typename E> struct is_result<Result<T, E>> : std::true_type {
    using ok_type = T;
    using err_type = E;
};
template <typename T> constexpr bool is_result_v = is_result<T>::value;

template <typename> struct result_traits;
template <typename T, typename E> struct result_traits<Result<T, E>> {
    using ok_type = T;
    using err_type = E;
};

template <typename T>
concept ResultOfError = is_result_v<T> && requires { typename result_traits<T>::err_type; };

template <typename T> struct [[nodiscard]] Option {
    static_assert(!std::is_same_v<T, Void>);
    static_assert(!std::is_reference_v<T>, "Option<T> must not hold references");

    ox::either::Either<T, Void> m_data;

  public:
    Option(const either::LValue<T> &v) : m_data(v) {}
    Option(either::LValue<T> &&v) : m_data(std::move(v)) {}

    Option(Void) : m_data(ox::either::RValue<Void>(Void{})) {}

    Option<const T *const> as_ref() const {
        if (is_some())
            return Option<const T &>(m_data.as_ref().unwrap_left());
        return None;
    }

    Option<T *const> as_mut() & {
        if (is_some())
            return Option<T const *>(m_data.as_mut().unwrap_left());
        return None;
    }

    auto flatten() && -> T
        requires is_option_v<T>
    {
        if (is_none())
            return None;

        T v = m_data.unwrap_left();
        if (v.is_none())
            return None;

        return Option<T>(ox::move(v).unwrap());
    }

    template <typename T_ = T>
    auto transpose() && -> Result<Option<typename result_traits<T_>::ok_type>,
        typename result_traits<T_>::err_type>
        requires is_result_v<T_>
    {
        using U = typename result_traits<T_>::ok_type;

        if (is_none())
            return Ok<Option<U>>(None);

        T_ res = ox::move(m_data).unwrap_left(); // T is Result<U, E>
        if (res.is_err())
            return Err(ox::move(res).unwrap_err());

        return Ok(Option<T>(ox::move(res).unwrap()));
    }

    u8 getState() const { return m_data.m_state; }

    operator bool() const noexcept { return is_some(); }
    constexpr bool is_some() const noexcept { return m_data.is_left(); }
    constexpr bool is_none() const noexcept { return m_data.is_right(); }

    template <typename U> Option<U> and_(const Option<U> &optb) const {
        return m_data.is_left() ? optb : None;
    }
    template <typename U, typename F>
        requires Callable<F, U, T>
    Option<U> and_then(F &&f) && {
        if (is_none())
            return None;
        return std::invoke<F>(std::forward<F>(f), ox::move(m_data).unwrap_left());
    }

    template <typename U> Option<U> or_(Option<U> &&optb) && {
        return m_data.is_left() ? ox::move(*this) : ox::move(optb);
    }
    template <typename F, typename U>
        requires Callable<F, U>
    Option<U> or_else(F &&f) && {
        if (is_some())
            return ox::move(*this);
        return std::invoke<F>(std::forward<F>(f));
    }

    Option<T> xor_(Option<T> &&optb) && {
        if (getState() == optb.getState())
            return None;
        return is_some() ? ox::move(*this) : ox::move(optb);
    }

    template <typename P>
        requires Callable<P, bool, const T &>
    Option<T> filter(P &&predicate) && {
        if (is_none())
            return None;
        return predicate(ox::move(m_data).unwrap_left()) ? ox::move(*this) : None;
    }

    template <typename U> T *const insert(U &&value) {
        m_data = Left(std::forward<U>(value));
        return m_data.as_ref().unwrap_left();
    }

    template <typename F>
        requires Callable<F, void, const T &>
    Option<T> inspect(F &&f) && {
        if (is_some()) {
            T val = ox::move(m_data).unwrap_left();
            std::invoke(std::forward<F>(f), val);
            return Option<T>(ox::move(val));
        }
        return ox::move(*this);
    }

    template <typename U, typename F>
        requires Callable<F, U, T>
    Option<U> map(F &&f) && {
        if (is_none())
            return None;
        return Option<T>(std::invoke(std::forward<F>(f), ox::move(m_data).unwrap_left()));
    }

    template <typename U, typename F>
        requires Callable<F, U, T>
    U map_or(U &&default_, F &&f) && {
        if (is_none())
            return std::forward<U>(default_);
        return std::invoke(std::forward<F>(f), ox::move(m_data).unwrap_left());
    }

    template <typename U, typename F, typename D>
        requires Callable<F, U, T> && Callable<D, U>
    U map_or_else(D &&default_, F &&f) && {
        if (is_none())
            return std::invoke(std::forward<D>(default_));
        return std::invoke(std::forward<F>(f), ox::move(m_data).unwrap_left());
    }

    template <typename E> Result<T, E> ok_or(E &&err) && {
        return is_some() ? Ok(ox::move(m_data).unwrap_left()) : Err(std::forward<E>(err));
    }

    Option<T> take() && {
        if (is_some()) {
            T val = ox::move(m_data).unwrap_left();
            m_data = ox::either::Right<T>(); // Now in None state
            return Option<T>(std::move(val));
        }
        return None;
    }

    const T &expect(RawStr s) const & {
        if (is_none())
            panic("{}: ()", s);
        return *m_data.unsafe_retrieve_raw_left();
    }
    T expect(RawStr s) && { return ox::move(m_data).expect_left(s); }

    const T &unwrap() const & { return expect("called `Option::unwrap()` on a `None` value"); }
    T unwrap() && { return std::move(*this).expect("called `Option::unwrap()` on a `None` value"); }

    T unwrap_or(T &&default_) && {
        return is_some() ? ox::move(m_data).unwrap_left() : std::forward<T>(default_);
    }

    template <typename F>
        requires Callable<F, T>
    T unwrap_or_else(F &&f) && {
        return is_some() ? ox::move(m_data).unwrap_left() : std::invoke(std::forward<F>(f));
    }

    template <typename U> Option<Tp<T, U>> zip(Option<U> &&other) && {
        if (is_none() || other.is_none())
            return None;
        return Some(Tp<T, U>(std::move(*this).unwrap(), std::move(other).unwrap()));
    }

    Option<T> clone()
        requires(trait::Clone<T>)
    {
        return is_some() ? Some(m_data.unwrap_left().clone()) : None;
    }
};

template <typename T> struct Match<Option<T>> {
    Option<T> data;

    Match(Option<T> &&data) : data(ox::move(data)) {}

    MATCH_FOR(some, T, data.is_some(), ox::move(data).unwrap())
    MATCH_FOR_VOID(none, data.is_some())
};
template <typename T> Match(Option<T>) -> Match<Option<T>>;

// Result<T, E>

template <typename T> either::LValue<T> Ok(T &&v) {
    if constexpr (trait::Copy<T>) {
        return either::LValue<T>(v);
    } else {
        return either::LValue<T>(ox::move(v));
    }
}
inline either::LValue<Void> Ok() {
    return either::LValue<Void>(Void{});
}

template <typename E> either::RValue<E> Err(E &&v) {
    if constexpr (trait::Copy<E>) {
        return either::RValue<E>(v);
    } else {
        return either::RValue<E>(ox::move(v));
    }
}
inline either::RValue<Void> Err() {
    return either::RValue<Void>(Void{});
}

template <typename T, typename E> struct [[nodiscard]] Result {
    static_assert(!std::is_reference_v<T> && !std::is_reference_v<E>,
        "Result<T, E> must not hold references, opt for pointers instead");
    ox::either::Either<T, E> m_data;

  public:
    Result(const either::LValue<T> &v) : m_data(v) {}
    Result(either::LValue<T> &&v) : m_data(ox::move(v)) {}
    Result(const either::RValue<E> &v) : m_data(v) {}
    Result(either::RValue<E> &&v) : m_data(ox::move(v)) {}

    Result<const T *const, const E *const> as_ref() const {
        if (is_ok())
            return Ok(m_data.as_ref().unwrap_left());
        return Err(m_data.as_ref().unwrap_right());
    }

    Result<T *const, E *const> as_mut() & {
        if (is_ok())
            return Ok(m_data.as_mut().unwrap_left());
        return Err(m_data.as_mut().unwrap_right());
    }

    T flatten() &&
        requires is_result_v<T>
    {
        if (is_ok()) {
            return ox::move(m_data).unwrap_left();
        } else {
            return Err(ox::move(m_data).unwrap_right());
        }
    }

    operator bool() const noexcept { return is_ok(); }
    constexpr bool is_ok() const noexcept { return m_data.is_left(); }
    constexpr bool is_err() const noexcept { return m_data.is_right(); }

    template <typename U> Result<U, E> and_(const Result<U, E> &res) && {
        return is_ok() ? res : ox::move(*this);
    }
    template <typename U, typename F>
        requires Callable<F, Result<U, E>, T>
    Result<U, E> and_then(F &&f) && {
        if (is_err())
            return Err(std::move(m_data).unwrap_right());
        return std::invoke(std::forward<F>(f), ox::move(m_data).unwrap_left());
    }

    template <typename U> Result<U, E> or_(Result<U, E> &&res) && {
        return is_ok() ? ox::move(*this) : ox::move(res);
    }
    template <typename U, typename F>
        requires Callable<F, U, T>
    Result<T, U> or_else(F &&f) && {
        if (is_ok())
            return ox::move(*this);
        return std::invoke<F>(std::forward<F>(f), ox::move(m_data).unwrap_right());
    }

    Option<T> ok() && { return is_ok() ? Some(ox::move(m_data).unwrap_left()) : None; }
    Option<E> err() && { return is_err() ? Some(ox::move(m_data).unwrap_right()) : None; }

    const T &expect(RawStr msg) const & {
        if (is_err()) {
            if constexpr (std::is_pointer_v<E>) {
                panic("{}: {}", msg, static_cast<void *>(*m_data.unsafe_retrieve_raw_right()));
            }else {
                panic("{}: {}", msg, *m_data.unsafe_retrieve_raw_right());
            }
        }
        return *m_data.unsafe_retrieve_raw_left();
    }
    T expect(RawStr msg) && { return ox::move(m_data).expect_left(msg); }

    const E &expect_err(RawStr msg) const & {
        if (is_ok()) {
            if constexpr (std::is_pointer_v<T>) {
                panic("{}: {}", msg, static_cast<void *>(*m_data.unsafe_retrieve_raw_left()));
            }else {
                panic("{}: {}", msg, *m_data.unsafe_retrieve_raw_left());
            }
        }
        return *m_data.unsafe_retrieve_raw_right();
    }
    E expect_err(RawStr msg) && { return ox::move(m_data).expect_right(msg); }

    template <typename F>
        requires Callable<F, void, T>
    Result<T, E> inspect(F &&f) && {
        if (is_ok()) {
            T val = ox::move(m_data).unwrap_left();
            std::invoke(std::forward<F>(f), val);
            return Result<T, E>(either::LValue<T>(std::move(val)));
        }
        return ox::move(*this);
    }
    template <typename F>
        requires Callable<F, void, E>
    Result<T, E> inspect_err(F &&f) && {
        if (is_err()) {
            E val = ox::move(m_data).unwrap_right();
            std::invoke(std::forward<F>(f), val);
            return Result<T, E>(either::RValue<E>(ox::move(val)));
        }
        return ox::move(*this);
    }

    template <class U, class F>
        requires Callable<F, U, T>
    Result<U, E> map(F &&f) && {
        if (is_ok()) {
            return Ok(f(std::forward<T>(ox::move(m_data).unwrap_left())));
        } else {
            return Err(std::forward<E>(ox::move(m_data).unwrap_right()));
        }
    }

    template <typename U, typename F>
        requires Callable<F, U, E>
    Result<T, U> map_err(F &&f) && {
        if (is_ok())
            return ox::move(*this);
        return Err(std::invoke(std::forward<F>(f), ox::move(m_data).unwrap_right()));
    }

    template <typename F, typename U>
        requires Callable<F, U, T>
    U map_or(U &&default_, F &&f) && {
        if (is_err())
            return std::forward<U>(default_);
        return std::invoke(std::forward<F>(f), ox::move(m_data).unwrap_left());
    }

    template <typename U, typename F, typename D>
        requires Callable<F, U, T> && Callable<D, U, E>
    auto map_or_else(D &&default_, F &&f) && {
        if (is_err())
            return std::invoke(std::forward<D>(default_), ox::move(m_data).unwrap_right());
        return std::invoke(std::forward<F>(f), ox::move(m_data).unwrap_left());
    }

    const T &unwrap() const & { return expect("called `Result::unwrap()` on a `Err` value"); }
    T unwrap() && { return std::move(*this).expect("called `Result::unwrap()` on a `Err` value"); }
    const E &unwrap_err() const & {
        return expect_err("called `Result::unwrap_err()` on a `Ok` value");
    }
    E unwrap_err() && {
        return std::move(*this).expect_err("called `Result::unwrap_err()` on a `Ok` value");
    }

    T unwrap_or(T &&default_) && {
        return is_ok() ? ox::move(m_data).unwrap_left() : std::forward<T>(default_);
    }

    template <typename F>
        requires Callable<F, T, E>
    T unwrap_or_else(F &&f) && {
        return is_ok() ? ox::move(m_data).unwrap_left()
                       : std::invoke(std::forward<F>(f), ox::move(m_data).unwrap_right());
    }

    Result<T, E> clone() const
        requires(trait::Copy<T> || trait::Clone<T>) && (trait::Copy<E> || trait::Clone<E>)
    {
        if (is_ok()) {
            return Ok<T>(trait::forced_clone(*m_data.unsafe_retrieve_raw_left()));
        } else {
            return Err<E>(trait::forced_clone(*m_data.unsafe_retrieve_raw_right()));
        }
    }
};

template <typename T, typename E> struct Match<Result<T, E>> {
    Result<T, E> data;

    Match(Result<T, E> &&data) : data(ox::move(data)) {}

    MATCH_FOR(ok, T, data.is_ok(), ox::move(data).unwrap())
    MATCH_FOR(err, E, data.is_err(), ox::move(data).unwrap_err())
};
template <typename T, typename E> Match(Result<T, E>) -> Match<Result<T, E>>;

} // namespace ox

template <typename T> struct fmt::formatter<ox::Option<T>> {
    constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const ox::Option<T> &v, format_context &ctx) const {
        if (v.is_some()) {
            return fmt::format_to(ctx.out(), "Some({})", v.unwrap());
        } else {
            return fmt::format_to(ctx.out(), "None");
        }
    }
};

template <typename T, typename E> struct fmt::formatter<ox::Result<T, E>> {
    constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const ox::Result<T, E> &v, format_context &ctx) const {
        if (v.is_ok()) {
            return fmt::format_to(ctx.out(), "Ok({})", v.unwrap());
        } else {
            return fmt::format_to(ctx.out(), "Err({})", v.unwrap_err());
        }
    }
};