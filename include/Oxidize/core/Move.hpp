#pragma once

#include <type_traits>
namespace ox {

template <typename T> constexpr std::remove_reference_t<T>&& move(T&& t) noexcept {
    return static_cast<std::remove_reference_t<T>&&>(t);
}

} // namespace ox