#pragma once
#include "Oxidize/core/Types.hpp"
#include "Oxidize/iter/Iterator.hpp"
#include "Oxidize/slice/Slice.hpp"
#include "Oxidize/vec/Vec.hpp"
#include <cstring>

namespace ox {

struct str {
    Slice<u8> m_data;

    static bool run_utf8_validation(RawStr data, usize len) {
        i32 cnt = 0;
        usize i = 0;
        while (i < len) {
            u8 x = static_cast<u8>(data[i]);

            if (cnt == 0) {
                if ((x >> 5) == 0b110) {
                    cnt = 1; // 2-byte sequence
                } else if ((x >> 4) == 0b1110) {
                    cnt = 2; // 3-byte sequence
                } else if ((x >> 3) == 0b11110) {
                    cnt = 3; // 4-byte sequence
                } else if ((x >> 7) != 0) {
                    // Leading byte is invalid (not ASCII and not UTF-8 start byte)
                    return false;
                }
                // else ASCII byte, cnt stays 0
            } else {
                // Continuation byte: must start with '10xxxxxx'
                if ((x >> 6) != 0b10) {
                    return false;
                }
                cnt--;
            }
            i++;
        }
        // At end, no expected continuation bytes left
        return cnt == 0;
    }

  public:
    str(RawStr data) {
        usize strlen = std::strlen(data);
        if (!run_utf8_validation(data, strlen)) {
            panic("Invalid UTF-8 string");
        }

        m_data = Slice<u8>(reinterpret_cast<u8*>(const_cast<char*>(data)), strlen);
    }
    str(RawString data) {
        if (!run_utf8_validation(data.data(), data.size())) {
            panic("Invalid UTF-8 string");
        }

        m_data = Slice<u8>(reinterpret_cast<u8*>(data.data()), data.size());
    }
    str(const Slice<u8> slice) {
        if (!run_utf8_validation(reinterpret_cast<RawStr>(slice.begin()), slice.len())) {
            panic("Invalid UTF-8 string");
        }

        m_data = slice;
    }

    bool operator==(const str& other) { return m_data == other.m_data; }
    bool operator!=(const str& other) { return m_data != other.m_data; }

    Vec<str> split(Slice<u8> pattern) const {
        mut result = Vec<str>::new_();

        if (pattern.len() == 0) {
            panic("Cannot split on empty pattern");
        }

        const u8* haystack = m_data.begin();
        usize hay_len = m_data.len();
        usize pat_len = pattern.len();

        usize start = 0;
        usize i = 0;

        while (i + pat_len <= hay_len) {
            bool matched = true;
            for (usize j = 0; j < pat_len; ++j) {
                if (haystack[i + j] != pattern[j]) {
                    matched = false;
                    break;
                }
            }

            if (matched) {
                usize segment_len = i - start;
                result.push(str(Slice<u8>(haystack + start, segment_len)));
                i += pat_len;
                start = i;
            } else {
                i++;
            }
        }

        if (start <= hay_len) {
            result.push(str(Slice<u8>(haystack + start, hay_len - start)));
        }

        return result;
    }
    Vec<str> split(str pattern) const { return split(pattern.as_slice()); }

    Vec<str> split_inclusive(Slice<u8> pattern) const {
        mut result = Vec<str>::new_();

        if (pattern.len() == 0) {
            panic("Cannot split on empty pattern");
        }

        const u8* haystack = m_data.begin();
        usize hay_len = m_data.len();
        usize pat_len = pattern.len();

        usize start = 0;
        usize i = 0;

        while (i + pat_len <= hay_len) {
            bool matched = true;
            for (usize j = 0; j < pat_len; ++j) {
                if (haystack[i + j] != pattern[j]) {
                    matched = false;
                    break;
                }
            }

            if (matched) {
                usize segment_len = (i + pat_len) - start;
                result.push(str(Slice<u8>(haystack + start, segment_len)));
                i += pat_len;
                start = i;
            } else {
                i++;
            }
        }

        if (start < hay_len) {
            result.push(str(Slice<u8>(haystack + start, hay_len - start)));
        }

        return result;
    }
    Vec<str> split_inclusive(str pattern) const { return split_inclusive(pattern.as_slice()); }

    usize len() const { return m_data.len(); }

    const Slice<u8> as_slice() const { return m_data; }

    iter::Iter<u8> iter() const { return m_data.iter(); }
    iter::IterMut<u8> iter_mut() { return m_data.iter_mut(); }
};

} // namespace ox