#pragma once
#include <concepts>

namespace ox::trait {

template <typename T, template <typename...> class U> struct is_instance_of : std::false_type {};

template <template <typename...> class U, typename... Args>
struct is_instance_of<U<Args...>, U> : std::true_type {};

template <typename Self, template <typename...> class Target>
concept InstanceOf = is_instance_of<Self, Target>::value;

template <typename Self>
concept Trait = requires() {
    requires std::default_initializable<Self>;
    { Self::Provide } -> std::convertible_to<const bool>;
};

template <typename T>
concept Copy = std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>;
template <typename T>
concept TrivialCopy =
    std::is_trivially_copy_constructible_v<T> && std::is_trivially_copy_assignable_v<T>;

template <typename T>
concept Clone = requires(T f) {
    { f.clone() } -> std::convertible_to<T>;
};

template <typename T> T forced_clone(const T &v) {
    if constexpr (Copy<T>) {
        return v;
    } else if constexpr (Clone<T>) {
        return v.clone();
    }
}

} // namespace ox::trait