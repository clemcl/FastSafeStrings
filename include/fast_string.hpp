/******************************************************************************
 * PROJECT:       FastSafeStrings (FSS) & VBIO
 * FILE:         fast_string.hpp
 * DESCRIPTION:  Modern C++17/C++20 fixed-capacity, length-aware string class
 *               with zero heap allocation, full constexpr support, and
 *               std::string_view interop.
 *
 * AUTHOR:        Clement Victor Clarke (Warracknabeal, Australia)
 * COPYRIGHT:     Copyright (c) 1988-2026 Clement Victor Clarke (Originator of Jol)
 *                All Rights Reserved.
 *
 * LICENSE TERMS:
 *   1. INDIVIDUAL/NON-PROFIT: Use is free under the MIT License.
 *   2. COMMERCIAL: Use by entities with annual revenue > $1M AUD requires
 *      a paid Commercial License.
 *
 * MISSION:       To reduce global energy consumption through computational
 *                efficiency.
 *
 * CONTACT:       clemclarke@gmail.com for commercial terms and
 *                "Shared Savings" agreements.
 ******************************************************************************/
  
#ifndef FAST_STRING_HPP
#define FAST_STRING_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <ostream>
#include <functional>
#include <type_traits>

#if __cplusplus >= 202002L || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)
#include <compare>
#define FSS_CPP20_OR_LATER 1
#endif

namespace fss {

namespace detail {

/**
 * Compile-time / constexpr strlen helper.
 */
constexpr std::size_t constexpr_strlen(const char* s) noexcept {
    if (!s) return 0;
#if defined(__cpp_lib_is_constant_evaluated) && __cpp_lib_is_constant_evaluated >= 201811L
    if (std::is_constant_evaluated()) {
        std::size_t len = 0;
        while (s[len] != '\0') ++len;
        return len;
    }
#endif
    return std::char_traits<char>::length(s);
}

} // namespace detail

/**
 * fast_string<N>:
 * Stack-allocated, fixed-capacity string of N bytes (including null terminator).
 * Maximum content capacity is N - 1 bytes.
 */
template <std::size_t N>
class fast_string {
    static_assert(N > 0, "fast_string capacity N must be greater than 0");

public:
    // Types
    using value_type             = char;
    using size_type              = std::size_t;
    using difference_type       = std::ptrdiff_t;
    using reference              = char&;
    using const_reference        = const char&;
    using pointer                = char*;
    using const_pointer          = const char*;
    using iterator               = char*;
    using const_iterator         = const char*;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // Constants
    static constexpr size_type npos        = static_cast<size_type>(-1);
    static constexpr size_type buffer_size = N;

private:
    alignas(16) char data_[N];
    std::uint32_t len_;

    constexpr void assign_range(const char* s, size_type count) noexcept {
        if (!s || count == 0) {
            clear();
            return;
        }
        size_type cap = capacity();
        size_type to_copy = (count < cap) ? count : cap;
        for (size_type i = 0; i < to_copy; ++i) {
            data_[i] = s[i];
        }
        len_ = static_cast<std::uint32_t>(to_copy);
        data_[len_] = '\0';
    }

    constexpr void append_range(const char* s, size_type count) noexcept {
        if (!s || count == 0) return;
        size_type cap = capacity();
        if (len_ >= cap) return;
        size_type space = cap - len_;
        size_type to_copy = (count < space) ? count : space;
        for (size_type i = 0; i < to_copy; ++i) {
            data_[len_ + i] = s[i];
        }
        len_ += static_cast<std::uint32_t>(to_copy);
        data_[len_] = '\0';
    }

public:
    /* ---------------------------------------------------------------------- */
    /* Constructors                                                           */
    /* ---------------------------------------------------------------------- */

    /**
     * Default constructor: initializes to empty string.
     */
    constexpr fast_string() noexcept : data_{0}, len_(0) {}

    /**
     * Literal constructor: optimizes string literals without strlen().
     */
    template <std::size_t M>
    constexpr fast_string(const char (&lit)[M]) noexcept : data_{0}, len_(0) {
        assign_range(lit, (M > 0 && lit[M - 1] == '\0') ? M - 1 : M);
    }

    /**
     * std::string_view constructor.
     */
    constexpr fast_string(std::string_view sv) noexcept : data_{0}, len_(0) {
        assign_range(sv.data(), sv.size());
    }

    /**
     * Runtime C-string constructor.
     */
    constexpr explicit fast_string(const char* s) noexcept : data_{0}, len_(0) {
        if (s) {
            assign_range(s, detail::constexpr_strlen(s));
        }
    }

    /**
     * Pointer + count constructor.
     */
    constexpr fast_string(const char* s, size_type count) noexcept : data_{0}, len_(0) {
        assign_range(s, count);
    }

    /**
     * Fill constructor.
     */
    constexpr fast_string(size_type count, char ch) noexcept : data_{0}, len_(0) {
        size_type cap = capacity();
        size_type to_copy = (count < cap) ? count : cap;
        for (size_type i = 0; i < to_copy; ++i) {
            data_[i] = ch;
        }
        len_ = static_cast<std::uint32_t>(to_copy);
        data_[len_] = '\0';
    }

    /**
     * Copy constructor.
     */
    constexpr fast_string(const fast_string& other) noexcept : data_{0}, len_(0) {
        assign_range(other.data_, other.len_);
    }

    /**
     * Converting copy constructor from fast_string of another capacity.
     */
    template <std::size_t M>
    constexpr fast_string(const fast_string<M>& other) noexcept : data_{0}, len_(0) {
        assign_range(other.data(), other.size());
    }

    /**
     * Move constructor.
     */
    constexpr fast_string(fast_string&& other) noexcept = default;

    /* ---------------------------------------------------------------------- */
    /* Assignment Operators                                                   */
    /* ---------------------------------------------------------------------- */

    template <std::size_t M>
    constexpr fast_string& operator=(const char (&lit)[M]) noexcept {
        assign_range(lit, (M > 0 && lit[M - 1] == '\0') ? M - 1 : M);
        return *this;
    }

    constexpr fast_string& operator=(std::string_view sv) noexcept {
        assign_range(sv.data(), sv.size());
        return *this;
    }

    constexpr fast_string& operator=(const char* s) noexcept {
        if (s) {
            assign_range(s, detail::constexpr_strlen(s));
        } else {
            clear();
        }
        return *this;
    }

    constexpr fast_string& operator=(char ch) noexcept {
        if (capacity() >= 1) {
            data_[0] = ch;
            data_[1] = '\0';
            len_ = 1;
        } else {
            clear();
        }
        return *this;
    }

    constexpr fast_string& operator=(const fast_string& other) noexcept {
        if (this != &other) {
            assign_range(other.data_, other.len_);
        }
        return *this;
    }

    template <std::size_t M>
    constexpr fast_string& operator=(const fast_string<M>& other) noexcept {
        assign_range(other.data(), other.size());
        return *this;
    }

    constexpr fast_string& operator=(fast_string&& other) noexcept = default;

    /* ---------------------------------------------------------------------- */
    /* Append Operations                                                      */
    /* ---------------------------------------------------------------------- */

    template <std::size_t M>
    constexpr fast_string& operator+=(const char (&lit)[M]) noexcept {
        append_range(lit, (M > 0 && lit[M - 1] == '\0') ? M - 1 : M);
        return *this;
    }

    constexpr fast_string& operator+=(std::string_view sv) noexcept {
        append_range(sv.data(), sv.size());
        return *this;
    }

    constexpr fast_string& operator+=(const char* s) noexcept {
        if (s) {
            append_range(s, detail::constexpr_strlen(s));
        }
        return *this;
    }

    constexpr fast_string& operator+=(char ch) noexcept {
        push_back(ch);
        return *this;
    }

    template <std::size_t M>
    constexpr fast_string& operator+=(const fast_string<M>& other) noexcept {
        append_range(other.data(), other.size());
        return *this;
    }

    constexpr fast_string& append(std::string_view sv) noexcept {
        append_range(sv.data(), sv.size());
        return *this;
    }

    constexpr fast_string& append(const char* s, size_type count) noexcept {
        append_range(s, count);
        return *this;
    }

    constexpr fast_string& append(size_type count, char ch) noexcept {
        for (size_type i = 0; i < count; ++i) {
            push_back(ch);
        }
        return *this;
    }

    template <std::size_t M>
    constexpr fast_string& append(const fast_string<M>& other) noexcept {
        append_range(other.data(), other.size());
        return *this;
    }

    constexpr void push_back(char ch) noexcept {
        if (len_ < capacity()) {
            data_[len_++] = ch;
            data_[len_] = '\0';
        }
    }

    constexpr void pop_back() noexcept {
        if (len_ > 0) {
            --len_;
            data_[len_] = '\0';
        }
    }

    /* ---------------------------------------------------------------------- */
    /* Access & Conversions                                                   */
    /* ---------------------------------------------------------------------- */

    constexpr operator std::string_view() const noexcept {
        return std::string_view(data_, len_);
    }

    constexpr std::string_view view() const noexcept {
        return std::string_view(data_, len_);
    }

    constexpr std::string_view sv() const noexcept {
        return std::string_view(data_, len_);
    }

    constexpr const char* c_str() const noexcept {
        return data_;
    }

    constexpr const char* data() const noexcept {
        return data_;
    }

    constexpr char* data() noexcept {
        return data_;
    }

    constexpr const_reference front() const noexcept {
        return data_[0];
    }

    constexpr reference front() noexcept {
        return data_[0];
    }

    constexpr const_reference back() const noexcept {
        return data_[len_ > 0 ? len_ - 1 : 0];
    }

    constexpr reference back() noexcept {
        return data_[len_ > 0 ? len_ - 1 : 0];
    }

    constexpr const_reference operator[](size_type pos) const noexcept {
        return data_[pos];
    }

    constexpr reference operator[](size_type pos) noexcept {
        return data_[pos];
    }

    constexpr const_reference at(size_type pos) const {
        if (pos >= len_) {
            throw std::out_of_range("fast_string::at index out of range");
        }
        return data_[pos];
    }

    constexpr reference at(size_type pos) {
        if (pos >= len_) {
            throw std::out_of_range("fast_string::at index out of range");
        }
        return data_[pos];
    }

    /* ---------------------------------------------------------------------- */
    /* Iterators                                                              */
    /* ---------------------------------------------------------------------- */

    constexpr iterator begin() noexcept { return data_; }
    constexpr const_iterator begin() const noexcept { return data_; }
    constexpr const_iterator cbegin() const noexcept { return data_; }

    constexpr iterator end() noexcept { return data_ + len_; }
    constexpr const_iterator end() const noexcept { return data_ + len_; }
    constexpr const_iterator cend() const noexcept { return data_ + len_; }

    constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    constexpr const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }

    constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    constexpr const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

    /* ---------------------------------------------------------------------- */
    /* Capacity & Sizing                                                      */
    /* ---------------------------------------------------------------------- */

    constexpr size_type size() const noexcept { return len_; }
    constexpr size_type length() const noexcept { return len_; }
    constexpr size_type capacity() const noexcept { return N > 0 ? N - 1 : 0; }
    constexpr size_type max_size() const noexcept { return N > 0 ? N - 1 : 0; }
    constexpr bool empty() const noexcept { return len_ == 0; }

    constexpr void clear() noexcept {
        len_ = 0;
        data_[0] = '\0';
    }

    constexpr void resize(size_type count, char ch = '\0') noexcept {
        size_type cap = capacity();
        size_type target = (count < cap) ? count : cap;
        if (target > len_) {
            for (size_type i = len_; i < target; ++i) {
                data_[i] = ch;
            }
        }
        len_ = static_cast<std::uint32_t>(target);
        data_[len_] = '\0';
    }

    constexpr void set_length(size_type l) noexcept {
        if (l <= capacity()) {
            len_ = static_cast<std::uint32_t>(l);
            data_[len_] = '\0';
        }
    }

    /* ---------------------------------------------------------------------- */
    /* Query & Search                                                         */
    /* ---------------------------------------------------------------------- */

    constexpr bool starts_with(std::string_view prefix) const noexcept {
        if (prefix.size() > len_) return false;
        if (prefix.empty()) return true;
        for (size_type i = 0; i < prefix.size(); ++i) {
            if (data_[i] != prefix[i]) return false;
        }
        return true;
    }

    constexpr bool starts_with(char ch) const noexcept {
        return len_ > 0 && data_[0] == ch;
    }

    constexpr bool ends_with(std::string_view suffix) const noexcept {
        if (suffix.size() > len_) return false;
        if (suffix.empty()) return true;
        size_type offset = len_ - suffix.size();
        for (size_type i = 0; i < suffix.size(); ++i) {
            if (data_[offset + i] != suffix[i]) return false;
        }
        return true;
    }

    constexpr bool ends_with(char ch) const noexcept {
        return len_ > 0 && data_[len_ - 1] == ch;
    }

    constexpr bool contains(std::string_view sv) const noexcept {
        return find(sv) != npos;
    }

    constexpr bool contains(char ch) const noexcept {
        return find(ch) != npos;
    }

    constexpr size_type find(std::string_view sv, size_type pos = 0) const noexcept {
        return view().find(sv, pos);
    }

    constexpr size_type find(char ch, size_type pos = 0) const noexcept {
        return view().find(ch, pos);
    }

    constexpr size_type find(const char* s, size_type pos, size_type count) const noexcept {
        return view().find(s, pos, count);
    }

    constexpr size_type rfind(std::string_view sv, size_type pos = npos) const noexcept {
        return view().rfind(sv, pos);
    }

    constexpr size_type rfind(char ch, size_type pos = npos) const noexcept {
        return view().rfind(ch, pos);
    }

    constexpr std::string_view substr(size_type pos = 0, size_type count = npos) const {
        return view().substr(pos, count);
    }

    template <std::size_t SubN = N>
    constexpr fast_string<SubN> sub_string(size_type pos = 0, size_type count = npos) const {
        return fast_string<SubN>(view().substr(pos, count));
    }

    /* ---------------------------------------------------------------------- */
    /* Comparison                                                             */
    /* ---------------------------------------------------------------------- */

    constexpr int compare(std::string_view other) const noexcept {
        return view().compare(other);
    }

    template <std::size_t M>
    constexpr int compare(const fast_string<M>& other) const noexcept {
        return view().compare(other.view());
    }

    constexpr int compare(const char* s) const noexcept {
        return view().compare(s ? std::string_view(s) : std::string_view{});
    }
};

/* -------------------------------------------------------------------------- */
/* Non-Member Relational Operators                                            */
/* -------------------------------------------------------------------------- */

template <std::size_t N1, std::size_t N2>
constexpr bool operator==(const fast_string<N1>& lhs, const fast_string<N2>& rhs) noexcept {
    return lhs.view() == rhs.view();
}

template <std::size_t N1, std::size_t N2>
constexpr bool operator!=(const fast_string<N1>& lhs, const fast_string<N2>& rhs) noexcept {
    return lhs.view() != rhs.view();
}

template <std::size_t N1, std::size_t N2>
constexpr bool operator<(const fast_string<N1>& lhs, const fast_string<N2>& rhs) noexcept {
    return lhs.view() < rhs.view();
}

template <std::size_t N1, std::size_t N2>
constexpr bool operator<=(const fast_string<N1>& lhs, const fast_string<N2>& rhs) noexcept {
    return lhs.view() <= rhs.view();
}

template <std::size_t N1, std::size_t N2>
constexpr bool operator>(const fast_string<N1>& lhs, const fast_string<N2>& rhs) noexcept {
    return lhs.view() > rhs.view();
}

template <std::size_t N1, std::size_t N2>
constexpr bool operator>=(const fast_string<N1>& lhs, const fast_string<N2>& rhs) noexcept {
    return lhs.view() >= rhs.view();
}

#ifdef FSS_CPP20_OR_LATER
template <std::size_t N1, std::size_t N2>
constexpr auto operator<=>(const fast_string<N1>& lhs, const fast_string<N2>& rhs) noexcept {
    return lhs.view() <=> rhs.view();
}

template <std::size_t N>
constexpr auto operator<=>(const fast_string<N>& lhs, std::string_view rhs) noexcept {
    return lhs.view() <=> rhs;
}

template <std::size_t N>
constexpr auto operator<=>(std::string_view lhs, const fast_string<N>& rhs) noexcept {
    return lhs <=> rhs.view();
}
#endif

// Comparison with std::string_view
template <std::size_t N>
constexpr bool operator==(const fast_string<N>& lhs, std::string_view rhs) noexcept {
    return lhs.view() == rhs;
}

template <std::size_t N>
constexpr bool operator==(std::string_view lhs, const fast_string<N>& rhs) noexcept {
    return lhs == rhs.view();
}

template <std::size_t N>
constexpr bool operator!=(const fast_string<N>& lhs, std::string_view rhs) noexcept {
    return lhs.view() != rhs;
}

template <std::size_t N>
constexpr bool operator!=(std::string_view lhs, const fast_string<N>& rhs) noexcept {
    return lhs != rhs.view();
}

template <std::size_t N>
constexpr bool operator<(const fast_string<N>& lhs, std::string_view rhs) noexcept {
    return lhs.view() < rhs;
}

template <std::size_t N>
constexpr bool operator<(std::string_view lhs, const fast_string<N>& rhs) noexcept {
    return lhs < rhs.view();
}

// Comparison with const char*
template <std::size_t N>
constexpr bool operator==(const fast_string<N>& lhs, const char* rhs) noexcept {
    return lhs.view() == (rhs ? std::string_view(rhs) : std::string_view{});
}

template <std::size_t N>
constexpr bool operator==(const char* lhs, const fast_string<N>& rhs) noexcept {
    return (lhs ? std::string_view(lhs) : std::string_view{}) == rhs.view();
}

template <std::size_t N>
constexpr bool operator!=(const fast_string<N>& lhs, const char* rhs) noexcept {
    return lhs.view() != (rhs ? std::string_view(rhs) : std::string_view{});
}

template <std::size_t N>
constexpr bool operator!=(const char* lhs, const fast_string<N>& rhs) noexcept {
    return (lhs ? std::string_view(lhs) : std::string_view{}) != rhs.view();
}

/* -------------------------------------------------------------------------- */
/* Binary operator+ Overloads                                                 */
/* -------------------------------------------------------------------------- */

template <std::size_t N1, std::size_t N2>
constexpr fast_string<N1 + N2 - 1> operator+(const fast_string<N1>& lhs, const fast_string<N2>& rhs) {
    fast_string<N1 + N2 - 1> result(lhs);
    result += rhs;
    return result;
}

template <std::size_t N>
constexpr fast_string<N> operator+(const fast_string<N>& lhs, std::string_view rhs) {
    fast_string<N> result(lhs);
    result += rhs;
    return result;
}

template <std::size_t N>
constexpr fast_string<N> operator+(const fast_string<N>& lhs, const char* rhs) {
    fast_string<N> result(lhs);
    result += rhs;
    return result;
}

template <std::size_t N>
constexpr fast_string<N> operator+(const fast_string<N>& lhs, char rhs) {
    fast_string<N> result(lhs);
    result += rhs;
    return result;
}

/* -------------------------------------------------------------------------- */
/* Stream Output Operator                                                     */
/* -------------------------------------------------------------------------- */

template <std::size_t N>
inline std::ostream& operator<<(std::ostream& os, const fast_string<N>& str) {
    return os.write(str.data(), static_cast<std::streamsize>(str.size()));
}

} // namespace fss

// Also expose fast_string in global namespace for drop-in compatibility
template <std::size_t N>
using fast_string = fss::fast_string<N>;

/* -------------------------------------------------------------------------- */
/* std::hash Specialization                                                   */
/* -------------------------------------------------------------------------- */

namespace std {

template <std::size_t N>
struct hash<fss::fast_string<N>> {
    std::size_t operator()(const fss::fast_string<N>& str) const noexcept {
        return std::hash<std::string_view>{}(str.view());
    }
};

} // namespace std

#endif /* FAST_STRING_HPP */
