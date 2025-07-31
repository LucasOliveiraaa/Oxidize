#pragma once
#include "../vec/Vec.hpp"
#include "../core/Types.hpp"
#include "../str/Str.hpp"

namespace ox {

struct String {
    Vec<u8> vec;

  public:
    String() : vec(Vec<u8>::new_()) {}
    String(const char* cstr) : String(str(cstr)) {}
    String(const str& data) : vec(Vec<u8>::from_iter(data.iter())) {}
    String(const RawString& data) : vec(Vec<u8>::from_iter(str(data).iter())) {}

    String(const String&) = delete;
    String& operator=(const String&) = delete;

    String(String&& other) noexcept : vec(move(other.vec)) {}
    String& operator=(String&& other) noexcept {
        vec = move(other.vec);
        return *this;
    }

    bool operator==(const String& other) const { return vec == other.vec; }
    bool operator!=(const String& other) const { return vec != other.vec; }

    bool operator==(RawStr other) const { return vec.as_slice() == str(other).as_slice(); }
    bool operator!=(RawStr other) const { return vec.as_slice() != str(other).as_slice(); }

    bool operator==(const RawString& other) const {
        return vec.as_slice() == str(other).as_slice();
    }
    bool operator!=(const RawString& other) const {
        return vec.as_slice() != str(other).as_slice();
    }

    bool operator==(const str& other) const { return vec.as_slice() == other.as_slice(); }
    bool operator!=(const str& other) const { return vec.as_slice() != other.as_slice(); }

    usize capacity() const { return vec.capacity(); }
    usize len() const { return vec.len(); }

    void push(char c) {
      if(!str::run_utf8_validation(&c, 1))
        panic("Invalid UTF-8 character");
      vec.push(static_cast<u8>(c));
    }

    bool starts_with(const Slice<u8>& slice) const { return vec.as_slice().starts_with(slice); }
    bool starts_with(const str& s) const { return starts_with(s.as_slice()); }
    bool starts_with(RawStr cstr) const { return starts_with(str(cstr).as_slice()); }
    bool starts_with(const RawString& s) const { return starts_with(str(s).as_slice()); }

    bool ends_with(const Slice<u8>& slice) const { return vec.as_slice().ends_with(slice); }
    bool ends_with(const str& s) const { return ends_with(s.as_slice()); }
    bool ends_with(RawStr cstr) const { return ends_with(str(cstr).as_slice()); }
    bool ends_with(const RawString& s) const { return ends_with(str(s).as_slice()); }

    Vec<str> split(str pattern) const { return str(vec.as_slice()).split(pattern); }
    Vec<str> split(String pattern) const {
        return str(vec.as_slice()).split(pattern.vec.as_slice());
    }

    Vec<str> split_inclusive(str pattern) const {
        return str(vec.as_slice()).split_inclusive(pattern);
    }
    Vec<str> split_inclusive(String pattern) const {
        return str(vec.as_slice()).split_inclusive(pattern.vec.as_slice());
    }

    String clone() const { return String(vec.as_slice()); }

    RawString as_cstring() const {
        return RawString(reinterpret_cast<const char*>(vec.m_ptr.get()), len());
    }
};

} // namespace ox

template <> struct std::formatter<ox::String> : std::formatter<std::string_view> {
    template <typename FormatContext> auto format(const ox::String& s, FormatContext& ctx) const {
        return std::formatter<std::string_view>::format(
            std::string_view(s.as_cstring().data(), s.len()), ctx);
    }
};