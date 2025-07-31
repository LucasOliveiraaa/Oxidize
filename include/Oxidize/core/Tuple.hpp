#pragma once
#include <tuple>

template <typename... Ts> struct Tuple {
    using usize = unsigned long;
    using BaseType = std::tuple<Ts...>;
    BaseType m_data;

  public:
    Tuple() = default;

    // 🛠️ Prevent copying if any Ts are move-only
    Tuple(const Tuple&) = delete;
    Tuple& operator=(const Tuple&) = delete;

    Tuple(Tuple&&) noexcept = default;
    Tuple& operator=(Tuple&&) noexcept = default;

    Tuple(const std::tuple<Ts...>& other) : m_data(other) {}
    Tuple(std::tuple<Ts...>&& other) : m_data(std::move(other)) {}

    template <typename... Us>
        requires(std::constructible_from<Ts, Us &&> && ...)
    Tuple(Us&&... vs) : m_data(std::forward<Us>(vs)...) {}

    template <usize n> decltype(auto) get() const { return std::get<n>(m_data); }
    template <usize n> decltype(auto) get() { return std::get<n>(m_data); }

    bool operator==(const Tuple<Ts...>& other) const { return m_data == other.m_data; }
    bool operator!=(const Tuple<Ts...>& other) const { return m_data != other.m_data; }

    template <typename... Us> bool operator==(const Tuple<Us...>& other) const {
        return m_data == other.m_data;
    }
};

template <typename... Us> Tuple(Us&&...) -> Tuple<std::decay_t<Us>...>;

#define tp(...) Tuple<__VA_ARGS__>
template <typename... Ts> using Tp = Tuple<Ts...>;

namespace std {
template <typename... Ts>
struct tuple_size<Tuple<Ts...>> : std::integral_constant<std::size_t, sizeof...(Ts)> {};

template <std::size_t N, typename... Ts> struct tuple_element<N, Tuple<Ts...>> {
    using type = typename std::tuple_element<N, std::tuple<Ts...>>::type;
};

// Overload std::get
template <std::size_t N, typename... Ts> decltype(auto) get(const Tuple<Ts...>& t) {
    return std::get<N>(t.m_data);
}

template <std::size_t N, typename... Ts> decltype(auto) get(Tuple<Ts...>& t) {
    return std::get<N>(t.m_data);
}
} // namespace std