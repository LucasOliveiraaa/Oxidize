#pragma once
#include "../core/Fn.hpp"
#include "../panic/Unwind.hpp"
#include "../core/Types.hpp"
#include "../core/Move.hpp"
#include "Oxidize/traits/Traits.h"
#include <functional>
#include <type_traits>
#include <utility>

namespace ox::either {

#define CONSTRUCT_WRAPPER_DESIGNATION(t, name, stru)                                               \
    template <typename t> stru<t> name(t &&v) {                                                    \
        return stru<t>(std::forward<t>(v));                                                        \
    }                                                                                              \
    template <typename t> stru<t &> name(t &v) {                                                   \
        return stru<t &>(v);                                                                       \
    }                                                                                              \
    template <typename t> stru<const t &> name(const t &v) {                                       \
        return stru<const t &>(v);                                                                 \
    }                                                                                              \
    inline stru<Void> name() {                                                                     \
        return stru<Void>(Void{});                                                                 \
    }

#define CONSTRUCT_WRAPPER_DECLARATION(t, name, stru)                                               \
    template <typename t> stru<t> name(t &&v);                                                     \
    template <typename t> stru<t &> name(t &v);                                                    \
    template <typename t> stru<const t &> name(const t &v);                                        \
    inline stru<Void> name();

template <typename L, typename R> struct MatchExpr;

template <typename L> struct LValue {
  public:
    using type = L;
    L m_left;
    explicit LValue(L &left)
        requires(std::is_copy_constructible_v<L>)
        : m_left(left) {}
    explicit LValue(const L &left)
        requires(std::is_copy_constructible_v<L>)
        : m_left(left) {}
    explicit LValue(L &&left)
        requires(std::is_move_constructible_v<L>)
        : m_left(ox::move(left)) {}
};

CONSTRUCT_WRAPPER_DESIGNATION(L, Left, LValue)

template <typename R> struct RValue {
  public:
    using type = R;
    R m_right;
    explicit RValue(R &right)
        requires(std::is_copy_constructible_v<R>)
        : m_right(right) {}
    explicit RValue(const R &right)
        requires(std::is_copy_constructible_v<R>)
        : m_right(right) {}
    explicit RValue(R &&right)
        requires(std::is_move_constructible_v<R>)
        : m_right(ox::move(right)) {}
};

CONSTRUCT_WRAPPER_DESIGNATION(R, Right, RValue)

struct EitherBase {
    static constexpr u8 LEFT = 0x0;
    static constexpr u8 RIGHT = 0x1;
    static constexpr u8 Corrupted = 0xFF;
};

template <typename L, typename R> struct Either : public EitherBase {
    static_assert(!std::is_reference_v<L>, "Either<L, R> must not hold references");
    static_assert(!std::is_reference_v<R>, "Either<L, R> must not hold references");

    union {
        L m_left;
        R m_right;
    };
    u8 m_state;

  public:
    Either(LValue<L> &&v) {
        try {
            std::construct_at<L>(&m_left, ox::move(v.m_left));
            m_state = LEFT;
        } catch (...) {
            m_state = Corrupted;
        }
    }
    Either(RValue<R> &&v) {
        try {
            std::construct_at<R>(&m_right, ox::move(v.m_right));
            m_state = RIGHT;
        } catch (...) {
            m_state = Corrupted;
        }
    }

    // Move

    Either(Either<L, R> &&other)
        requires(std::is_move_constructible_v<L> && std::is_move_constructible_v<R>)
    {
        try {
            if (other.m_state == LEFT)
                std::construct_at<L>(&m_left, ox::move(other.m_left));
            else
                std::construct_at<R>(&m_right, ox::move(other.m_right));
            m_state = other.m_state;
        } catch (...) {
            m_state = Corrupted;
        }
    }
    Either<L, R> &operator=(Either<L, R> &&other)
        requires(std::is_move_constructible_v<L> && std::is_move_constructible_v<R>)
    {
        if (&other == this)
            return *this;

        try {
            if (m_state == LEFT)
                std::destroy_at<L>(&m_left);
            else
                std::destroy_at<R>(&m_right);

            if (other.m_state == LEFT)
                std::construct_at<L>(&m_left, ox::move(other.m_left));
            else
                std::construct_at<R>(&m_right, ox::move(other.m_right));

            m_state = other.m_state;
            return *this;
        } catch (...) {
            m_state = Corrupted;
            return *this;
        }
    }

    // Copy

    Either(const Either<L, R> &other)
        requires(std::is_copy_constructible_v<L> && std::is_copy_constructible_v<R>)
    {
        try {
            if (other.m_state == LEFT)
                std::construct_at<L>(&m_left, other.m_left);
            else
                std::construct_at<R>(&m_right, other.m_right);
            m_state = other.m_state;
        } catch (...) {
            m_state = Corrupted;
        }
    }
    Either<L, R> &operator=(const Either<L, R> &other)
        requires(std::is_copy_constructible_v<L> && std::is_copy_constructible_v<R>)
    {
        if (&other == this)
            return *this;

        try {
            if (m_state == LEFT)
                std::destroy_at<L>(&m_left);
            else
                std::destroy_at<R>(&m_right);
            if (other.m_state == LEFT)
                std::construct_at<L>(&m_left, other.m_left);
            else
                std::construct_at<R>(&m_right, other.m_right);

            m_state = other.m_state;
            return *this;
        } catch (...) {
            m_state = Corrupted;
            return *this;
        }
    }

    ~Either() {
        if (m_state == LEFT)
            std::destroy_at(&m_left);
        else
            std::destroy_at(&m_right);
    }

    // Acessors

    constexpr bool isCorrupted() const noexcept { return m_state == Corrupted; }

    template <typename F, typename G> decltype(auto) either(F &&f, G &&g) {
        if (m_state == LEFT)
            return std::invoke(std::forward<F>(f), m_left);
        return std::invoke(std::forward<G>(g), m_right);
    }

    L expect_left(RawStr msg) & noexcept
        requires(trait::Copy<L> || trait::Clone<L>)
    {
        if (m_state == RIGHT)
            panic("{}: {}", msg, ox::move(m_right));
        if constexpr (trait::Copy<L>) {
            return m_left;
        } else {
            return m_left.clone();
        }
    }
    const L expect_left(RawStr msg) const & noexcept
        requires(trait::Copy<L> || trait::Clone<L>)
    {
        if (m_state == RIGHT)
            panic("{}: {}", msg, ox::move(m_right));
        if constexpr (trait::Copy<L>) {
            return m_left;
        } else {
            return m_left.clone();
        }
    }
    L expect_left(RawStr msg) && noexcept {
        if (m_state == RIGHT)
            panic("{}: {}", msg, ox::move(m_right));
        return ox::move(m_left);
    }

    R expect_right(RawStr msg) & noexcept
        requires(trait::Copy<R> || trait::Clone<R>)
    {
        if (m_state == LEFT)
            panic("{}: {}", msg, ox::move(m_left));
        if constexpr (trait::Copy<R>) {
            return m_right;
        } else {
            return m_right.clone();
        }
    }
    const R expect_right(RawStr msg) const & noexcept
        requires(trait::Copy<R> || trait::Clone<R>)
    {
        if (m_state == LEFT)
            panic("{}: {}", msg, ox::move(m_left));
        if constexpr (trait::Copy<R>) {
            return m_right;
        } else {
            return m_right.clone();
        }
    }
    R expect_right(RawStr msg) && noexcept {
        if (m_state == LEFT)
            panic("{}: {}", msg, ox::move(m_left));
        return ox::move(m_right);
    }

    Either<R, L> flip() & { return m_state == LEFT ? Right(m_left) : Left(m_right); }
    const Either<R, L> flip() const & { return m_state == LEFT ? Right(m_left) : Left(m_right); }
    Either<R, L> flip() && {
        return m_state == LEFT ? Right(ox::move(m_left)) : Left(ox::move(m_right));
    }

    constexpr bool is_left() const noexcept { return m_state == LEFT; }
    constexpr bool is_right() const noexcept { return m_state == RIGHT; }

    template <typename F, typename S> Either<S, R> left_and_then(F &&f) {
        if (m_state == RIGHT)
            return Right(m_right);
        return Left(std::invoke(std::forward<F>(f), m_left));
    }

    L left_or(const L &other) & { return m_state == LEFT ? m_left : other; }
    const L left_or(const L &other) const & { return m_state == LEFT ? m_left : other; }
    L left_or(const L &other) && { return m_state == LEFT ? ox::move(m_left) : other; }

    template <typename F, typename S> Either<S, R> right_and_then(F &&f) {
        if (m_state == LEFT)
            return Left(m_left);
        return Right(std::invoke(std::forward<F>(f), m_right));
    }

    R right_or(const R &other) & { return m_state == RIGHT ? m_right : other; }
    const R right_or(const R &other) const & { return m_state == RIGHT ? m_right : other; }
    R right_or(const R &other) && { return m_state == RIGHT ? ox::move(m_right) : other; }

    template <typename F, typename G, typename M, typename S>
    Either<M, S> map_either(F &&f, G &&g) {
        if (m_state == LEFT)
            return Left(std::invoke(std::forward<F>(f), m_left));
        return Right(std::invoke(std::forward<G>(g), m_right));
    }
    template <typename F, typename M> Either<M, R> map_left(F &&f) {
        if (m_state == RIGHT)
            return Right(m_right);
        return Left(std::invoke(std::forward<F>(f), m_left));
    }
    template <typename F, typename M> Either<L, M> map_right(F &&f) {
        if (m_state == LEFT)
            return Left(m_left);
        return Right(std::invoke(std::forward<F>(f), m_right));
    }

    L unwrap_left() & noexcept
        requires(trait::Copy<L> || trait::Clone<L>)
    {
        return expect_left("called `Either::unwrap_left()` on a `Right` value");
    }
    const L unwrap_left() const & noexcept
        requires(trait::Copy<L> || trait::Clone<L>)
    {
        return expect_left("called `Either::unwrap_left()` on a `Right` value");
    }
    L unwrap_left() && noexcept {
        return std::move(*this).expect_left("called `Either::unwrap_left()` on a `Right` value");
    }

    R unwrap_right() & noexcept
        requires(trait::Copy<R> || trait::Clone<R>)
    {
        return expect_right("called `Either::unwrap_right()` on a `Left` value");
    }
    const R unwrap_right() const & noexcept
        requires(trait::Copy<R> || trait::Clone<R>)
    {
        return expect_right("called `Either::unwrap_right()` on a `Left` value");
    }
    R unwrap_right() && noexcept {
        return std::move(*this).expect_right("called `Either::unwrap_right()` on a `Left` value");
    }

    template <typename LeftFn, typename RightFn>
    decltype(auto) match(LeftFn &&leftFn, RightFn &&rightFn) const {
        if (m_state == LEFT)
            return std::invoke(std::forward<LeftFn>(leftFn), m_left);
        return std::invoke(std::forward<RightFn>(rightFn), m_right);
    }

    MatchExpr<L, R> match() const { return MatchExpr{*this}; }
};

template <typename L, typename R> struct MatchExpr {
    const Either<L, R> &data;

    template <typename F>
        requires Callable<F, void, const L &>
    const MatchExpr &left(F &&f) const {
        if (data.is_left())
            std::invoke(std::forward<F>(f), data.unwrap_left());
        return *this;
    }
    template <typename F>
        requires Callable<F, void, const L &>
    const MatchExpr &ok(F &&f) const {
        if (data.is_left())
            std::invoke(std::forward<F>(f), data.unwrap_left());
        return *this;
    }
    template <typename F>
        requires Callable<F, void, const L &>
    const MatchExpr &some(F &&f) const {
        if (data.is_left())
            std::invoke(std::forward<F>(f), data.unwrap_left());
        return *this;
    }

    template <typename F>
        requires Callable<F, void, const R &>
    const MatchExpr &right(F &&f) const {
        if (data.is_right())
            std::invoke(std::forward<F>(f), data.unwrap_right());
        return *this;
    }
    template <typename F>
        requires Callable<F, void, const R &>
    const MatchExpr &err(F &&f) const {
        if (data.is_right())
            std::invoke(std::forward<F>(f), data.unwrap_right());
        return *this;
    }
    template <typename F>
        requires Callable<F, void>
    const MatchExpr &none(F &&f) const {
        if (data.is_right())
            std::invoke(std::forward<F>(f));
        return *this;
    }
};

template <typename T> struct MatchExpr<T, Void> {
    const Either<T, Void> &data;

    template <typename F>
        requires Callable<F, void, const T &>
    const MatchExpr &some(F &&f) const {
        if (data.is_left())
            std::invoke(std::forward<F>(f), data.unwrap_left());
        return *this;
    }

    template <typename F>
        requires Callable<F, void>
    const MatchExpr &none(F &&f) const {
        if (data.is_right())
            std::invoke(std::forward<F>(f));
        return *this;
    }
};

} // namespace ox::either