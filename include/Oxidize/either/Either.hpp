#pragma once
#include "../core/Fn.hpp"
#include "../panic/Unwind.hpp"
#include "../core/Types.hpp"
#include "../core/Move.hpp"
#include "../core/Match.hpp"
#include "../core/Forward.hpp"
#include "Oxidize/traits/Traits.h"
#include <functional>
#include <type_traits>

namespace ox::either {

struct EitherBase {
    static constexpr u8 LEFT = 0x0;
    static constexpr u8 RIGHT = 0x1;
    static constexpr u8 Corrupted = 0xFF;
};

template <typename L> struct LValue {
  public:
    using type = L;
    L m_left;
    explicit LValue(const L &left)
        requires(std::is_copy_constructible_v<L>)
        : m_left(left) {}
    explicit LValue(L &&left)
        requires(std::is_move_constructible_v<L>)
        : m_left(ox::move(left)) {}
};

template <typename R> struct RValue {
  public:
    using type = R;
    R m_right;
    explicit RValue(const R &right)
        requires(std::is_copy_constructible_v<R>)
        : m_right(right) {}
    explicit RValue(R &&right)
        requires(std::is_move_constructible_v<R>)
        : m_right(ox::move(right)) {}
};

template <typename L, typename R> struct Either : public EitherBase {
  private:
    using LStore = std::remove_cv_t<std::remove_reference_t<L>>;
    using RStore = std::remove_cv_t<std::remove_reference_t<R>>;

    consteval static usize max_size() { return sizeof(LStore) > sizeof(RStore) ? sizeof(LStore) : sizeof(RStore); }
    consteval static usize max_align() { return alignof(LStore) > alignof(RStore) ? alignof(LStore) : alignof(RStore); }

    static const constinit usize size = max_size();
    static const constinit usize align = max_align();

    alignas(align) u8 data[size];
    u8 m_state;

    LStore *as_left_ptr() { return std::launder(reinterpret_cast<LStore *>(&data)); }
    const LStore *as_left_ptr() const { return std::launder(reinterpret_cast<const LStore *>(&data)); }
    RStore *as_right_ptr() { return std::launder(reinterpret_cast<RStore *>(&data)); }
    const RStore *as_right_ptr() const { return std::launder(reinterpret_cast<const RStore *>(&data)); }

    void construct_left(L &&value) {
        try {
            std::construct_at(as_left_ptr(), ox::forward<L>(value));
            m_state = LEFT;
        } catch (panic::PanicUnwind &unwind) {
            throw unwind; // Re-throw panic to preserve stack trace;
        } catch (...) {
            m_state = Corrupted;
        }
    }
    void construct_right(R &&value) {
        try {
            std::construct_at(as_right_ptr(), ox::forward<R>(value));
            m_state = RIGHT;
        } catch (panic::PanicUnwind &unwind) {
            throw unwind; // Re-throw panic to preserve stack trace;
        } catch (...) {
            m_state = Corrupted;
        }
    }
    void destroy() {
        if (m_state == LEFT)
            std::destroy_at(as_left_ptr());
        else if (m_state == RIGHT)
            std::destroy_at(as_right_ptr());
    }

    L move_left() && noexcept {
        m_state = Corrupted;
        return ox::move(*as_left_ptr());
    }
    R move_right() && noexcept {
        m_state = Corrupted;
        return ox::move(*as_right_ptr());
    }

    void try_access() {
        if (m_state == Corrupted) {
            panic("Either is in a corrupted state, cannot access data.");
        }
    }

  public:
    Either(LValue<L> &&value) { construct_left(ox::forward<L>(value.m_left)); }
    Either(RValue<R> &&value) { construct_right(ox::forward<R>(value.m_right)); }
    // Move

    Either(Either<L, R> &&other) {
        if (other.m_state == LEFT)
            construct_left(ox::move(other).unwrap_left());
        else if (other.m_state == RIGHT)
            construct_right(ox::move(other).unwrap_right());
    }
    Either<L, R> &operator=(Either<L, R> &&other) {
        try_access();

        if (&other == this)
            return *this;

        destroy();

        if (other.m_state == LEFT)
            construct_left(ox::move(other).unwrap_left());
        else if (other.m_state == RIGHT)
            construct_right(ox::move(other).unwrap_right());

        return *this;
    }

    // Copy

    Either(const Either<L, R> &other)
        requires(trait::Copy<L> && trait::Copy<R>)
    {
        if (other.m_state == LEFT)
            construct_left(*dynamic_cast<L *>(&other.data));
        else if (other.m_state == RIGHT)
            construct_right(*dynamic_cast<R *>(&other.data));
    }
    Either<L, R> &operator=(const Either<L, R> &other)
        requires(trait::Copy<L> && trait::Copy<R>)
    {
        try_access();

        if (&other == this)
            return *this;

        destroy();

        if (other.m_state == LEFT)
            construct_left(*dynamic_cast<L *>(&other.data));
        else if (other.m_state == RIGHT)
            construct_right(*dynamic_cast<R *>(&other.data));

        return *this;
    }

    ~Either() {
        destroy();
        m_state = Corrupted;
    }

    // Acessors

    constexpr bool isCorrupted() const noexcept { return m_state == Corrupted; }

    const L *const unsafe_retrieve_raw_left() const { return as_left_ptr(); }
    const R *const unsafe_retrieve_raw_right() const { return as_right_ptr(); }

    Either<const L *const, const R *const> as_ref() const {
        try_access();

        if (m_state == LEFT) {
            return Left<const L *const>(as_left_ptr());
        } else if (m_state == RIGHT) {
            return Right<const R *const>(as_right_ptr());
        }
    }

    Either<L *const, R *const> as_mut() const {
        try_access();

        if (m_state == LEFT) {
            return Left<L *const>(as_left_ptr());
        } else if (m_state == RIGHT) {
            return Right<R *const>(as_right_ptr());
        }
    }

    template <typename T, typename F, typename G>
        requires(Callable<F, T, L> && Callable<G, T, R>)
    decltype(auto) either(F &&f, G &&g) && {
        try_access();

        if (m_state == LEFT)
            return std::invoke(std::forward<F>(f), ox::move(*this).move_left());
        return std::invoke(std::forward<G>(g), ox::move(*this).move_right());
    }

    L expect_left(RawStr msg) && {
        try_access();

        if (m_state == RIGHT) {
            if constexpr (std::is_pointer_v<R>) {
                panic("{}: {}", msg, static_cast<void *>(ox::move(*this).move_right()));
            } else {
                panic("{}: {}", msg, ox::move(*this).move_right());
            }
        }
        return ox::move(*this).move_left();
    }

    R expect_right(RawStr msg) && {
        try_access();

        if (m_state == LEFT){
            if constexpr (std::is_pointer_v<L>) {
                panic("{}: {}", msg, static_cast<void *>(ox::move(*this).move_left()));
            } else {
                panic("{}: {}", msg, ox::move(*this).move_left());
            }
        }
        return ox::move(*this).move_right();
    }

    Either<R, L> flip() && {
        try_access();

        if (m_state == LEFT)
            return Right(ox::move(*this).move_left());
        else if (m_state == RIGHT)
            return Left(ox::move(*this).move_right());
    }

    constexpr bool is_left() const noexcept { return m_state == LEFT; }
    constexpr bool is_right() const noexcept { return m_state == RIGHT; }

    template <typename F, typename S> Either<S, R> left_and_then(F &&f) && {
        try_access();

        if (m_state == RIGHT)
            return Right(ox::move(*this).move_right());
        return Left(std::invoke(std::forward<F>(f), ox::move(*this).move_left()));
    }

    L left_or(const L &other) && {
        try_access();

        return m_state == LEFT ? ox::move(*this).move_left() : other;
    }

    template <typename F, typename S> Either<S, R> right_and_then(F &&f) && {
        try_access();

        if (m_state == LEFT)
            return Left(ox::move(*this).move_left());
        return Right(std::invoke(std::forward<F>(f), ox::move(*this).move_right()));
    }

    R right_or(const R &other) && {
        try_access();

        return m_state == RIGHT ? ox::move(*this).move_right() : other;
    }

    template <typename F, typename G, typename M, typename S>
    Either<M, S> map_either(F &&f, G &&g) && {
        try_access();

        if (m_state == LEFT)
            return Left(std::invoke(std::forward<F>(f), ox::move(*this).move_left()));
        return Right(std::invoke(std::forward<G>(g), ox::move(*this).move_right()));
    }
    template <typename F, typename M> Either<M, R> map_left(F &&f) && {
        try_access();
        if (m_state == RIGHT)
            return Right(ox::move(*this).move_right());
        return Left(std::invoke(std::forward<F>(f), ox::move(*this).move_left()));
    }
    template <typename F, typename M> Either<L, M> map_right(F &&f) && {
        try_access();
        if (m_state == LEFT)
            return Left(ox::move(*this).move_left());
        return Right(std::invoke(std::forward<F>(f), ox::move(*this).move_right()));
    }

    L unwrap_left() && {
        return std::move(*this).expect_left("called `Either::unwrap_left()` on a `Right` value");
    }

    R unwrap_right() && {
        return std::move(*this).expect_right("called `Either::unwrap_right()` on a `Left` value");
    }
};

template <typename L> LValue<L> Left(L &&value) {
    if constexpr (trait::Copy<L>) {
        return LValue<L>(value);
    } else {
        return LValue<L>(ox::move(value));
    }
}
inline LValue<Void> Left() {
    return LValue<Void>(Void{});
}

template <typename R> RValue<R> Right(R &&value) {
    if constexpr (trait::Copy<R>) {
        return RValue<R>(value);
    } else {
        return RValue<R>(ox::move(value));
    }
}
inline RValue<Void> Right() {
    return RValue<Void>(Void{});
}

} // namespace ox::either

namespace ox {

template <typename L, typename R> struct Match<const either::Either<L, R> &> {
    const either::Either<L, R> &data;

    Match(const either::Either<L, R> &data) : data(data) {}

    MATCH_FOR(left, L, data.is_left(), data.unwrap_left())
    MATCH_FOR(right, R, data.is_right(), data.unwrap_right())
};
template <typename L, typename R> struct Match<either::Either<L, R>> {
    either::Either<L, R> data;

    Match(either::Either<L, R> data) : data(ox::move(data)) {}

    MATCH_FOR(left, L, data.is_left(), ox::move(data).unwrap_left())
    MATCH_FOR(right, R, data.is_right(), ox::move(data).unwrap_right())
};

} // namespace ox