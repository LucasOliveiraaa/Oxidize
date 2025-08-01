#pragma once
#include "../string/String.hpp"
#include <fmt/format.h>
#include <iostream>
#include <utility>

namespace ox {

template <typename... Args> String format(fmt::format_string<Args...> message, Args&&... args) {
    return fmt::format(message, std::forward<Args>(args)...);
}

template <typename... Args> void print(fmt::format_string<Args...> message, Args&&... args) {
    std::cout << fmt::format(message, std::forward<Args>(args)...);
}

template <typename... Args> void println(fmt::format_string<Args...> message, Args&&... args) {
    std::cout << fmt::format(message, std::forward<Args>(args)...) << std::endl;
}

inline void println() {
    println("");
}

} // namespace ox