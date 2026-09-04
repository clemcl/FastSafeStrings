#ifndef FAST_STRING_HPP
#define FAST_STRING_HPP

#include <cstdint>
#include <cstring>
#include <algorithm>

template <std::size_t N>
class fast_string
{
    static_assert(N > 1, "Size must be greater than 1");

    char data_[N];
    std::uint32_t len_;

public:
    /* ---------- Core ---------- */

    constexpr std::size_t capacity() const noexcept { return N - 1; }
    std::uint32_t size() const noexcept { return len_; }
    bool empty() const noexcept { return len_ == 0; }

    fast_string() noexcept : len_(0)
    {
        data_[0] = '\0';
    }

    /* ---------- Constructors ---------- */

    template <std::size_t M>
    fast_string(const char (&lit)[M])
    {
        assign_literal(lit);
    }

    explicit fast_string(const char* s)
    {
        assign_runtime(s);
    }

    /* ---------- Assignment ---------- */

    template <std::size_t M>
    fast_string& operator=(const char (&lit)[M])
    {
        assign_literal(lit);
        return *this;
    }

    fast_string& operator=(const char* s)
    {
        assign_runtime(s);
        return *this;
    }

    fast_string& operator=(const fast_string& other)
    {
        assign_fast(other);
        return *this;
    }

    /* ---------- Append ---------- */

    template <std::size_t M>
    fast_string& operator+=(const char (&lit)[M])
    {
        append_literal(lit);
        return *this;
    }

    fast_string& operator+=(const char* s)
    {
        append_runtime(s);
        return *this;
    }

    fast_string& operator+=(const fast_string& other)
    {
        append_fast(other);
        return *this;
    }

    /* ---------- Compare ---------- */

    int compare(const fast_string& other) const noexcept
    {
        std::size_t min = std::min(len_, other.len_);
        int r = std::memcmp(data_, other.data_, min);
        if (r != 0) return r;
        if (len_ < other.len_) return -1;
        if (len_ > other.len_) return 1;
        return 0;
    }

    bool operator==(const fast_string& other) const noexcept
    {
        return compare(other) == 0;
    }

    /* ---------- Access ---------- */

    const char* c_str() const noexcept { return data_; }
    char* data() noexcept { return data_; }

    /* ---------- VB Integration ---------- */

    std::uint32_t length() const noexcept { return len_; }

    void set_length(std::uint32_t l) noexcept
    {
        if (l <= capacity())
        {
            len_ = l;
            data_[len_] = '\0';
        }
    }

    void clear() noexcept
    {
        len_ = 0;
        data_[0] = '\0';
    }

private:

    /* ---------- Internal ---------- */

    template <std::size_t M>
    void assign_literal(const char (&lit)[M])
    {
        std::size_t len = M - 1;
        if (len > capacity()) len = capacity();

        std::memcpy(data_, lit, len);
        data_[len] = '\0';
        len_ = static_cast<std::uint32_t>(len);
    }

    void assign_runtime(const char* s)
    {
        std::size_t len = std::strlen(s);
        if (len > capacity()) len = capacity();

        std::memcpy(data_, s, len);
        data_[len] = '\0';
        len_ = static_cast<std::uint32_t>(len);
    }

    void assign_fast(const fast_string& other)
    {
        std::size_t len = other.len_;
        if (len > capacity()) len = capacity();

        std::memcpy(data_, other.data_, len);
        data_[len] = '\0';
        len_ = static_cast<std::uint32_t>(len);
    }

    template <std::size_t M>
    void append_literal(const char (&lit)[M])
    {
        std::size_t add = M - 1;
        std::size_t space = capacity() - len_;
        if (add > space) add = space;

        std::memcpy(data_ + len_, lit, add);
        len_ += static_cast<std::uint32_t>(add);
        data_[len_] = '\0';
    }

    void append_runtime(const char* s)
    {
        std::size_t add = std::strlen(s);
        std::size_t space = capacity() - len_;
        if (add > space) add = space;

        std::memcpy(data_ + len_, s, add);
        len_ += static_cast<std::uint32_t>(add);
        data_[len_] = '\0';
    }

    void append_fast(const fast_string& other)
    {
        std::size_t add = other.len_;
        std::size_t space = capacity() - len_;
        if (add > space) add = space;

        std::memcpy(data_ + len_, other.data_, add);
        len_ += static_cast<std::uint32_t>(add);
        data_[len_] = '\0';
    }
};

#endif
