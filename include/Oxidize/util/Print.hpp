#pragma once
#include "../string/String.hpp"
#include <format>
#include <print>

namespace ox {

template <typename... Args> void print(std::format_string<Args...> message, Args&&... args) {
    std::print(message, std::forward<Args>(args)...);
}

template <typename... Args> void println(std::format_string<Args...> message, Args&&... args) {
    std::println(message, std::forward<Args>(args)...);
}

inline void println() {
    std::println("");
}

template <typename... Args> String format(std::format_string<Args...> message, Args&&... args) {
    return std::format(message, std::forward<Args>(args)...);
}

} // namespace ox