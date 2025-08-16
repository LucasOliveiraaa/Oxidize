#pragma once
#include <type_traits>

namespace ox {

template <typename T> T &&forward(std::remove_reference_t<T> &t) noexcept {
    return static_cast<T &&>(t);
}

}