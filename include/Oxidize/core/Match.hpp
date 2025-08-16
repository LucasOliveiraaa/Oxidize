#pragma once
#include <utility>

namespace ox {

template <typename T> class Match {
  public:
    template <typename U> Match(U &&value) : value_(std::forward<U>(value)) {}

  private:
    T value_;
};

#define match(v) ox::move(Match(ox::move(v)))

#define MATCH_FOR(NAME, TYPE, TEST, VALUE)                                                         \
    template <typename F>                                                                          \
        requires(ox::Callable<F, void, TYPE>)                                                      \
    auto NAME(F &&f) && -> decltype(auto) {                                                        \
        if (TEST)                                                                                  \
            std::invoke(ox::forward<F>(f), VALUE);                                                 \
        return ox::move(*this);                                                                    \
    }

#define MATCH_FOR_VOID(NAME, TEST)                                                         \
    template <typename F>                                                                          \
        requires(ox::Callable<F, void>)                                                      \
    auto NAME(F &&f) && -> decltype(auto) {                                                        \
        if (TEST)                                                                                  \
            std::invoke(ox::forward<F>(f));                                                 \
        return ox::move(*this);                                                                    \
    }

} // namespace ox