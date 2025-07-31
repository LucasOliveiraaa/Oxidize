#pragma once
#include <concepts>
#include <type_traits>

namespace ox {

template <typename F, typename Ret, typename... Args>
concept Callable =
    std::invocable<F, Args...> && std::convertible_to<std::invoke_result_t<F, Args...>, Ret>;

#define CALL_FN(t, name, ...) std::invoke(std::forward<t>(name), __VA_ARGS__)

} // namespace ox