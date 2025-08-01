#pragma once
#include "../either/Either.hpp"
#include "Fn.hpp"
#include "Oxidize/core/Move.hpp"
#include "Oxidize/traits/Traits.h"
#include "Tuple.hpp"
#include "Types.hpp"
#include <type_traits>
#include <utility>

namespace ox {

#define TRY(expr)                                                                                  \
    ({                                                                                             \
        auto __res = (expr);                                                                       \
        if (!__res)                                                                                \
            return Err(__res.err().unwrap());                                                      \
        __res.unwrap();                                                                            \
    })

#define RETURN_EARLY(expr)                                                                         \
    ({                                                                                             \
        auto __res = (expr);                                                                       \
        if (!__res)                                                                                \
            return;                                                                                \
        __res.unwrap();                                                                            \
    })

template <typename T, typename E> struct Result;
template <typename T> struct Option;
template <typename T> struct OkValue;
template <typename T> struct ErrValue;

inline constexpr Void None = Void{};

template <typename> struct is_option : std::false_type {};
template <typename U> struct is_option<Option<U>> : std::true_type {};
template <typename T> constexpr bool is_option_v = is_option<T>::value;

template <typename> struct is_ok_value : std::false_type {};
template <typename U> struct is_ok_value<OkValue<U>> : std::true_type {};
template <typename T> constexpr bool is_ok_value_v = is_ok_value<T>::value;

template <typename> struct is_err_value : std::false_type {};
template <typename U> struct is_err_value<ErrValue<U>> : std::true_type {};
template <typename T> constexpr bool is_err_value_v = is_ok_value<T>::value;

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

template <typename T> Option<T> Some(T &&v);
template <typename T> Option<T &> Some(T &v);
template <typename T> Option<const T &> Some(const T &v);

template <typename T> struct [[nodiscard]] Option {
    static_assert(!std::is_same_v<T, Void>);
    static_assert(!std::is_reference_v<T>, "Option<T> must not hold references");

    ox::either::Either<T, Void> m_data;

  public:
    template <typename U = T>
    explicit Option(U &&v)
        requires(!std::is_same_v<std::remove_cvref_t<U>, Void>)
        : m_data(ox::either::Left(std::forward<U>(v))) {}
    Option(Void) : m_data(ox::either::Right()) {}

    Option<const T &> as_ref() const {
        if (is_some())
            return Option<const T &>(m_data.unwrap_left());
        return None;
    }

    Option<T &> as_mut() & {
        if (is_some())
            return Option<T &>(m_data.unwrap_left());
        return None;
    }

    auto flatten() const -> T
        requires is_option_v<T>
    {
        if (is_none())
            return None;

        const T &v = m_data.unwrap_left();
        if (v.is_none())
            return None;

        return Option<T>(v.unwrap());
    }

    template <typename T_ = T>
    auto transpose() const
        -> Result<Option<typename result_traits<T_>::ok_type>, typename result_traits<T_>::err_type>
        requires is_result_v<T_>
    {
        using U = typename result_traits<T_>::ok_type;
        using E = typename result_traits<T_>::err_type;

        if (is_none())
            return Ok<Option<U>>(None);

        const T_ &res = m_data.unwrap_left(); // T is Result<U, E>
        if (res.is_err())
            return Err(res.unwrap_err());

        return Ok(Option<T>(res.unwrap()));
    }

    u8 getState() const { return m_data.m_state; }

    operator bool() const noexcept { return is_some(); }
    constexpr bool is_some() const noexcept { return m_data.is_left(); }
    constexpr bool is_none() const noexcept { return m_data.is_right(); }

    template <typename U> Option<U> and_(const Option<U> &optb) const {
        return m_data.is_left() ? optb : None;
    }
    template <typename U, typename F>
        requires Callable<F, U, const T &>
    Option<U> and_then(F &&f) const {
        if (is_none())
            return None;
        return std::invoke<F>(std::forward<F>(f), m_data.unwrap_left());
    }

    template <typename U> Option<U> or_(const Option<U> &optb) const {
        return m_data.is_left() ? *this : optb;
    }
    template <typename F, typename U>
        requires Callable<F, U>
    Option<U> or_else(F &&f) const {
        if (is_some())
            return *this;
        return std::invoke<F>(std::forward<F>(f));
    }

    Option<T> xor_(Option<T> &&optb) const {
        if (getState() == optb.getState())
            return None;
        return is_some() ? *this : std::forward<Option<T>>(optb);
    }

    template <typename P>
        requires Callable<P, bool, const T &>
    Option<T> filter(P &&predicate) const {
        if (is_none())
            return None;
        return predicate(m_data.unwrap_left()) ? *this : None;
    }

    template <typename U> T &insert(U &&value) {
        m_data = Left(std::forward<U>(value));
        return m_data.unwrap_left();
    }

    template <typename F>
        requires Callable<F, void, const T &>
    Option<T> inspect(F &&f) const {
        if (is_some())
            std::invoke(std::forward<F>(f), m_data.unwrap_left());
        return *this;
    }

    template <typename U, typename F>
        requires Callable<F, U, const T &>
    Option<U> map(F &&f) const {
        if (is_none())
            return None;
        return Option<T>(std::invoke(std::forward<F>(f), m_data.unwrap_left()));
    }

    template <typename U, typename F>
        requires Callable<F, U, const T &>
    U map_or(U &&default_, F &&f) const {
        if (is_none())
            return std::forward<U>(default_);
        return std::invoke(std::forward<F>(f), m_data.unwrap_left());
    }

    template <typename U, typename F, typename D>
        requires Callable<F, U, const T &> && Callable<D, U>
    U map_or_else(D &&default_, F &&f) const {
        if (is_none())
            return std::invoke(std::forward<D>(default_));
        return std::invoke(std::forward<F>(f), m_data.unwrap_left());
    }

    template <typename E> Result<T, E> ok_or(E &&err) & {
        return is_some() ? Ok(m_data.unwrap_left()) : Err(std::forward<E>(err));
    }
    template <typename E> Result<const T, E> ok_or(E &&err) const & {
        return is_some() ? Ok(m_data.unwrap_left()) : Err(std::forward<E>(err));
    }
    template <typename E> Result<T, E> ok_or(E &&err) && {
        return is_some() ? Ok(ox::move(m_data).unwrap_left()) : Err(std::forward<E>(err));
    }

    Option<T> take() && {
        if (is_some()) {
            return Option<T>(std::exchange(m_data, ox::either::Right()).unwrap_left());
        }
        return None;
    }

    T expect(const char *s) &
        requires(trait::Copy<T> || trait::Clone<T>)
    {
        if (is_none())
            panic("{}", s);
        if constexpr (trait::Copy<T>) {
            return m_data.unwrap_left();
        } else {
            return m_data.unwrap_left().clone();
        }
    }
    const T expect(RawStr s) const &
        requires(trait::Copy<T> || trait::Clone<T>)
    {
        if (is_none())
            panic("{}", s);
        if constexpr (trait::Copy<T>) {
            return m_data.unwrap_left();
        } else {
            return m_data.unwrap_left().clone();
        }
    }
    T expect(RawStr s) && {
        if (is_none())
            panic("{}", s);
        return ox::move(m_data).unwrap_left();
    }

    T unwrap() &
        requires(trait::Copy<T> || trait::Clone<T>)
    {
        return expect("called `Option::unwrap()` on a `None` value");
    }
    const T unwrap() const &
        requires(trait::Copy<T> || trait::Clone<T>)
    {
        return expect("called `Option::unwrap()` on a `None` value");
    }
    T unwrap() && { return std::move(*this).expect("called `Option::unwrap()` on a `None` value"); }

    T unwrap_or(T &&default_) &
        requires(trait::Copy<T> || trait::Clone<T>)
    {
        return is_some() ? m_data.unwrap_left() : std::forward<T>(default_);
    }
    const T unwrap_or(T &&default_) const &
        requires(trait::Copy<T> || trait::Clone<T>)
    {
        return is_some() ? m_data.unwrap_left() : std::forward<T>(default_);
    }
    T unwrap_or(T &&default_) && {
        return is_some() ? ox::move(m_data).unwrap_left() : std::forward<T>(default_);
    }

    template <typename F>
        requires Callable<F, T>
    T unwrap_or_else(F &&f) & {
        return is_some() ? m_data.unwrap_left() : std::invoke(std::forward<F>(f));
    }
    template <typename F>
        requires Callable<F, T>
    const T &unwrap_or_else(F &&f) const & {
        return is_some() ? m_data.unwrap_left() : std::invoke(std::forward<F>(f));
    }
    template <typename F>
        requires Callable<F, T>
    T unwrap_or_else(F &&f) && {
        return is_some() ? ox::move(m_data).unwrap_left() : std::invoke(std::forward<F>(f));
    }

    template <typename U> Option<Tp<T, U>> zip(const Option<U> &other) const & {
        if (this->is_some() && other.is_some()) {
            if constexpr (trait::Copy<T> && trait::Copy<U>) {
                return Some(Tp<T, U>(this->unwrap(), other.unwrap()));
            } else if constexpr (trait::Copy<T> && trait::Clone<U>) {
                return Some(Tp<T, U>(this->unwrap(), other.unwrap().clone()));
            } else if constexpr (trait::Clone<T> && trait::Copy<U>) {
                return Some(Tp<T, U>(this->unwrap().clone(), other.unwrap()));
            } else {
                return Some(Tp<T, U>(this->unwrap().clone(), other.unwrap().clone()));
            }
        }
        return None;
    }

    template <typename U>
        Option<Tp<T, U>> zip(const Option<U> &other) &&
        requires(trait::Copy<U> || trait::Clone<U>) {
            if (is_none() || other.is_none())
                return None;

            U right = [&]() -> U {
                if constexpr (trait::Copy<U>)
                    return other.unwrap();
                else
                    return other.unwrap().clone();
            }();

            return Some(Tp<T, U>(std::move(*this).unwrap(), std::move(right)));
        }

        template <typename U>
        Option<Tp<T, U>> zip(Option<U> &&other) && {
        if (is_none() || other.is_none())
            return None;
        return Some(Tp<T, U>(std::move(*this).unwrap(), std::move(other).unwrap()));
    }

    template <typename U = void, typename SomeFn, typename NoneFn>
        requires Callable<SomeFn, U, const T &> && Callable<NoneFn, U>
    U match(SomeFn s, NoneFn n) const {
        if (is_some())
            return std::invoke(std::forward<SomeFn>(s), m_data.unwrap_left());
        return std::invoke(std::forward<NoneFn>(n));
    }

    ox::either::MatchExpr<T, Void> match() const & {
        return ox::either::MatchExpr<T, Void>{this->m_data};
    }
    ox::either::MatchExpr<T, Void> match() && {
        return ox::either::MatchExpr<T, Void>{move(m_data)};
    }
};

template <typename T> Option<T> Some(T &&v) {
    return Option<T>(std ::forward<T>(v));
}
template <typename T> Option<T &> Some(T &v) {
    return Option<T &>(v);
}
template <typename T> Option<const T &> Some(const T &v) {
    return Option<const T &>(v);
}

// Result<T, E>

template <typename T> struct OkValue : public ox::either::LValue<T> {
    using ox::either::LValue<T>::LValue;
};
template <> struct OkValue<Void> : public ox::either::LValue<Void> {
    OkValue(Void) : LValue<Void>(None) {}
};
CONSTRUCT_WRAPPER_DESIGNATION(T, Ok, OkValue)

template <typename E> struct ErrValue : public ox::either::RValue<E> {
    using ox::either::RValue<E>::RValue;
};
template <> struct ErrValue<Void> : public ox::either::RValue<Void> {
    ErrValue(Void) : RValue<Void>(None) {}
};
CONSTRUCT_WRAPPER_DESIGNATION(E, Err, ErrValue)

template <typename T, typename E> struct [[nodiscard]] Result {
    ox::either::Either<T, E> m_data;

  public:
    Result(OkValue<T> &&v) : m_data(ox::move(v)) {}
    Result(ErrValue<E> &&v) : m_data(ox::move(v)) {}

    Result<const T &, const E &> as_ref() const {
        if (is_ok())
            return Ok(m_data.unwrap_left());
        return Err(m_data.unwrap_right());
    }

    Result<T &, E &> as_mut() & {
        if (is_ok())
            return Ok(m_data.unwrap_left());
        return Err(m_data.unwrap_right());
    }

    T flatten() const
        requires is_option_v<T>
    {
        return match([](const T &ok) { return ok.is_none() ? None : Some(Ok(ok.unwrap())); },
            [](const E &err) { return Some(Err(err)); });
    }

    operator bool() const noexcept { return is_ok(); }
    constexpr bool is_ok() const noexcept { return m_data.is_left(); }
    constexpr bool is_err() const noexcept { return m_data.is_right(); }

    template <typename U> Result<U, E> and_(const Result<U, E> &res) {
        return is_ok() ? res : *this;
    }
    template <typename U, typename F>
        requires Callable<F, Result<U, E>, const T &>
    Result<U, E> and_then(F &&f) && {
        if (is_err())
            return Err(std::move(m_data).unwrap_right());
        return std::invoke(std::forward<F>(f), m_data.unwrap_left());
    }

    template <typename U> Result<U, E> or_(const Result<U, E> &res) {
        return is_ok() ? *this : res;
    }
    template <typename U> Result<U, E> or_(Result<U, E> &&res) && {
        return is_ok() ? ox::move(*this) : ox::move(res);
    }
    template <typename U, typename F>
        requires Callable<F, U, const T &>
    Result<T, U> or_else(F &&f) {
        if (is_ok())
            return *this;
        return std::invoke<F>(std::forward<F>(f), m_data.unwrap_left());
    }

    Option<T> ok() &
        requires(trait::Copy<T> || trait::Clone<T>)
    {
        if constexpr (trait::Copy<T>) {
            return is_ok() ? Some(m_data.unwrap_left()) : None;
        } else {
            return is_ok() ? Some(m_data.unwrap_left().clone()) : None;
        }
    }
    Option<const T> ok() const & { return is_ok() ? Some(m_data.unwrap_left()) : None; }
    Option<T> ok() && { return is_ok() ? Some(ox::move(m_data).unwrap_left()) : None; }

    Option<E> err() &
        requires(trait::Copy<E> || trait::Clone<T>)
    {
        if constexpr (trait::Copy<E>) {
            return is_err() ? Some(E(m_data.unwrap_right())) : None;
        } else {
            return is_err() ? Some(m_data.unwrap_right().clone()) : None;
        }
    }
    Option<const E> err() const &
        requires(trait::Copy<E> || trait::Clone<T>)
    {
        if constexpr (trait::Copy<E>) {
            return is_err() ? Some(E(m_data.unwrap_right())) : None;
        } else {
            return is_err() ? Some(m_data.unwrap_right().clone()) : None;
        }
    }
    Option<E> err() && { return is_err() ? Some(ox::move(m_data).unwrap_right()) : None; }

    T expect(RawStr msg) &
        requires(trait::Copy<T> || trait::Clone<T>)
    {
        if (is_err())
            panic("{}: {}", msg, m_data.unwrap_right());
        if constexpr (trait::Copy<T>) {
            return m_data.unwrap_left();
        } else {
            return m_data.unwrap_left().clone();
        }
    }
    const T expect(RawStr msg) const &
        requires(trait::Copy<T> || trait::Clone<T>)
    {
        if (is_err())
            panic("{}: {}", msg, m_data.unwrap_right());
        if constexpr (trait::Copy<T>) {
            return m_data.unwrap_left();
        } else {
            return m_data.unwrap_left().clone();
        }
    }
    T expect(RawStr msg) && {
        if (is_err())
            panic("{}: {}", msg, m_data.unwrap_right());
        return ox::move(m_data).unwrap_left();
    }

    E expect_err(RawStr msg) &
        requires(trait::Copy<E> || trait::Clone<E>)
    {
        if (is_ok())
            panic("{}: {}", msg, m_data.unwrap_left());
        if constexpr (trait::Copy<E>) {
            return m_data.unwrap_right();
        } else {
            return m_data.unwrap_right().clone();
        }
    }
    const E expect_err(RawStr msg) const &
        requires(trait::Copy<E> || trait::Clone<E>)
    {
        if (is_ok())
            panic("{}: {}", msg, m_data.unwrap_left());
        if constexpr (trait::Copy<E>) {
            return m_data.unwrap_right();
        } else {
            return m_data.unwrap_right().clone();
        }
    }
    E expect_err(RawStr msg) && {
        if (is_ok())
            panic("{}: {}", msg, m_data.unwrap_left());
        return ox::move(m_data).unwrap_right();
    }

    template <typename F>
        requires Callable<F, void, const T &>
    Option<T> inspect(F &&f) const {
        if (is_ok())
            std::invoke(std::forward<F>(f), m_data.unwrap_left());
        return *this;
    }
    template <typename F>
        requires Callable<F, void, const E &>
    Option<T> inspect_err(F &&f) const {
        if (is_err())
            std::invoke(std::forward<F>(f), m_data.unwrap_right());
        return *this;
    }

    template <class U, class F>
        requires Callable<F, U, const T &>
    Result<U, E> map(F &&f) {
        if (is_ok()) {
            return Ok(f(std::forward<T>(m_data.unwrap_left())));
        } else {
            return Err(std::forward<E>(m_data.unwrap_right()));
        }
    }

    template <typename U, typename F>
        requires Callable<F, U, const E &>
    Result<T, U> map_err(F &&f) const {
        if (is_ok())
            return *this;
        return Err(std::invoke(std::forward<F>(f), m_data.unwrap_right()));
    }

    template <typename F, typename U>
        requires Callable<F, U, const T &>
    U map_or(U &&default_, F &&f) const {
        if (is_err())
            return std::forward<U>(default_);
        return std::invoke(std::forward<F>(f), m_data.unwrap_left());
    }

    template <typename U, typename F, typename D>
        requires Callable<F, U, const T &> && Callable<D, U, const E &>
    auto map_or_else(D &&default_, F &&f) const {
        if (is_err())
            return std::invoke(std::forward<D>(default_), m_data.unwrap_right());
        return std::invoke(std::forward<F>(f), m_data.unwrap_left());
    }

    T unwrap() &
        requires(trait::Copy<T> || trait::Clone<T>)
    {
        return expect("called `Result::unwrap()` on a `Err` value");
    }
    const T unwrap() const &
        requires(trait::Copy<T> || trait::Clone<T>)
    {
        return expect("called `Result::unwrap()` on a `Err` value");
    }
    T unwrap() && { return expect("called `Result::unwrap()` on a `Err` value"); }

    E unwrap_err() &
        requires(trait::Copy<E> || trait::Clone<E>)
    {
        return expect_err("called `Result::unwrap_err()` on a `Ok` value");
    }
    const E unwrap_err() const &
        requires(trait::Copy<E> || trait::Clone<E>)
    {
        return expect_err("called `Result::unwrap_err()` on a `Ok` value");
    }
    E unwrap_err() && { return expect_err("called `Result::unwrap_err()` on a `Ok` value"); }

    T unwrap_or(T &&default_) &
        requires(trait::Copy<T> || trait::Clone<T>)
    {
        return is_ok() ? m_data.unwrap_left() : std::forward<T>(default_);
    }
    const T unwrap_or(T &&default_) const &
        requires(trait::Copy<T> || trait::Clone<T>)
    {
        return is_ok() ? m_data.unwrap_left() : std::forward<T>(default_);
    }
    T unwrap_or(T &&default_) && {
        return is_ok() ? ox::move(m_data).unwrap_left() : std::forward<T>(default_);
    }

    template <typename F>
        requires Callable<F, T, E>
    T unwrap_or_else(F &&f) & {
        return is_ok() ? m_data.unwrap_left()
                       : std::invoke(std::forward<F>(f), m_data.unwrap_right());
    }
    template <typename F>
        requires Callable<F, T, E>
    const T &unwrap_or_else(F &&f) const & {
        return is_ok() ? m_data.unwrap_left()
                       : std::invoke(std::forward<F>(f), m_data.unwrap_right());
    }
    template <typename F>
        requires Callable<F, T, E>
    T unwrap_or_else(F &&f) && {
        return is_ok() ? ox::move(m_data).unwrap_left()
                       : std::invoke(std::forward<F>(f), ox::move(m_data).unwrap_right());
    }

    template <typename U = void, typename OkFn, typename ErrFn>
        requires Callable<OkFn, U, const T &> && Callable<ErrFn, U, const E &>
    auto match(OkFn o, ErrFn e) const {
        if (is_ok())
            return std::invoke(std::forward<OkFn>(o), m_data.unwrap_left());
        return std::invoke(std::forward<ErrFn>(e), m_data.unwrap_right());
    }

    ox::either::MatchExpr<T, E> match() const { return ox::either::MatchExpr<T, E>{this->m_data}; }
};

} // namespace ox

template <typename T> struct fmt::formatter<ox::Option<T>> {
    constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const ox::Option<T> &v, format_context &ctx) const {
        return v.match([&](const T &ok) { return fmt::format_to(ctx.out(), "Ok({})", ok); },
            [&]() { return fmt::format_to(ctx.out(), "None"); });
    }
};

template <typename T, typename E> struct fmt::formatter<ox::Result<T, E>> {
    constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const ox::Result<T, E> &v, format_context &ctx) const {
        return v.match([&](const T &ok) { return fmt::format_to(ctx.out(), "Ok({})", ok); },
            [&](const E &err) { return fmt::format_to(ctx.out(), "Err({})", err); });
    }
};