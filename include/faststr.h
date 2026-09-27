/******************************************************************************
 * PROJECT:       FastSafeStrings (FSS) & VBIO
 * DESCRIPTION:  Modernized, high-performance, length-aware C string library
 *               with O(1) operations, bounded safety, and zero heap overhead.
 *
 * FILE:         faststr.h
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
 
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#ifndef FASTSTR_H
#define FASTSTR_H

#ifndef FASTSTR_BEST_H
#define FASTSTR_BEST_H
#endif

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Version Information                                                        */
/* -------------------------------------------------------------------------- */

#define FSS_VERSION_MAJOR 3
#define FSS_VERSION_MINOR 0
#define FSS_VERSION_PATCH 0
#define FSS_VERSION_STR   "3.0.0"

/* -------------------------------------------------------------------------- */
/* Compiler Portability, Intrinsics & Alignment                               */
/* -------------------------------------------------------------------------- */

#if defined(_MSC_VER)
    #define FSS_INLINE static __inline
    #define FSS_FORCEINLINE static __forceinline
    #define FSS_ALIGN16 __declspec(align(16))
    #define FSS_RESTRICT __restrict
    #define FSS_LIKELY(x)   (x)
    #define FSS_UNLIKELY(x) (x)
#elif defined(__GNUC__) || defined(__clang__)
    #define FSS_INLINE static inline
    #define FSS_FORCEINLINE static inline __attribute__((always_inline))
    #define FSS_ALIGN16 __attribute__((aligned(16)))
    #define FSS_RESTRICT __restrict__
    #define FSS_LIKELY(x)   __builtin_expect(!!(x), 1)
    #define FSS_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define FSS_INLINE static inline
    #define FSS_FORCEINLINE static inline
    #define FSS_ALIGN16
    #define FSS_RESTRICT
    #define FSS_LIKELY(x)   (x)
    #define FSS_UNLIKELY(x) (x)
#endif

#ifndef ALIGN_16
#define ALIGN_16 FSS_ALIGN16
#endif

#define FSS_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define FSS_MAX(a, b) (((a) > (b)) ? (a) : (b))

/* Structured error abort */
#define FSS_ABEND(code, msg) do { \
    fprintf(stderr, "FastSafeStrings ABEND %s: %s at %s:%d\n", \
            (code), (msg), __FILE__, __LINE__); \
    exit(1); \
} while(0)

#ifndef MVS_ABEND
#define MVS_ABEND(code, msg) FSS_ABEND(code, msg)
#endif

/* -------------------------------------------------------------------------- */
/* Type Descriptors                                                           */
/* -------------------------------------------------------------------------- */

/**
 * vb_meta_t / fss_meta_t:
 * The 8-byte metadata descriptor ("Dope Vector") tracking length and capacity.
 *   cur_len : current content length in bytes (excluding null terminator)
 *   max_len : allocated capacity in bytes (excluding null terminator)
 */
typedef struct vb_meta {
    uint32_t cur_len;
    uint32_t max_len;
} vb_meta_t;

typedef vb_meta_t fss_meta_t;

/**
 * fss_string:
 * Pointer-based handle for passing FSS strings dynamically across ABI boundaries.
 */
typedef struct fss_string {
    char      *data;
    vb_meta_t *meta;
} fss_string;

/**
 * fss_view_t:
 * Non-owning view of a string slice with length tracking (similar to string_view).
 */
typedef struct fss_view {
    const char *data;
    uint32_t   len;
} fss_view_t;

/* -------------------------------------------------------------------------- */
/* Low-Level Memory Helpers                                                   */
/* -------------------------------------------------------------------------- */

FSS_FORCEINLINE void fss_memcpy_inline(void *FSS_RESTRICT dst, const void *FSS_RESTRICT src, size_t n) {
#if defined(__GNUC__) || defined(__clang__)
    __builtin_memcpy(dst, src, n);
#else
    memcpy(dst, src, n);
#endif
}

FSS_FORCEINLINE int fss_memcmp_inline(const void *a, const void *b, size_t n) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_memcmp(a, b, n);
#else
    return memcmp(a, b, n);
#endif
}

/* -------------------------------------------------------------------------- */
/* Core Static Inline API                                                     */
/* -------------------------------------------------------------------------- */

/**
 * fss_len: Returns current length in bytes. O(1).
 */
FSS_FORCEINLINE uint32_t fss_len(const vb_meta_t *meta) {
    return meta ? meta->cur_len : 0;
}

/**
 * fss_maxlen: Returns allocated capacity in bytes. O(1).
 */
FSS_FORCEINLINE uint32_t fss_maxlen(const vb_meta_t *meta) {
    return meta ? meta->max_len : 0;
}

/**
 * fss_clear: Clears string to empty in O(1). Null-terminates at index 0.
 */
FSS_FORCEINLINE void fss_clear(char *dst, vb_meta_t *meta) {
    if (meta) meta->cur_len = 0;
    if (dst) dst[0] = '\0';
}

/**
 * fss_set_lit: Copies literal to destination with truncation and null-termination.
 */
FSS_FORCEINLINE uint32_t fss_set_lit(char *dst, vb_meta_t *meta, const char *lit, uint32_t lit_len) {
    if (!dst || !meta) return 0;
    uint32_t m = (lit_len > meta->max_len) ? meta->max_len : lit_len;
    if (m > 0 && lit) {
        fss_memcpy_inline(dst, lit, m);
    }
    meta->cur_len = m;
    dst[m] = '\0';
    return m;
}

/**
 * fss_set_cstr: Copies null-terminated C-string to destination.
 */
FSS_FORCEINLINE uint32_t fss_set_cstr(char *dst, vb_meta_t *meta, const char *cstr) {
    if (!dst || !meta) return 0;
    if (!cstr) {
        meta->cur_len = 0;
        dst[0] = '\0';
        return 0;
    }
    uint32_t clen = (uint32_t)strlen(cstr);
    uint32_t m = (clen > meta->max_len) ? meta->max_len : clen;
    if (m > 0) {
        fss_memcpy_inline(dst, cstr, m);
    }
    meta->cur_len = m;
    dst[m] = '\0';
    return m;
}

/**
 * fss_cpy: Copies FSS string src to dst. O(1) length check, direct memcpy.
 */
FSS_FORCEINLINE uint32_t fss_cpy(char *dst, vb_meta_t *dst_meta, const char *src, const vb_meta_t *src_meta) {
    if (!dst || !dst_meta) return 0;
    uint32_t slen = src_meta ? src_meta->cur_len : 0;
    uint32_t m = (slen > dst_meta->max_len) ? dst_meta->max_len : slen;
    if (m > 0 && src) {
        fss_memcpy_inline(dst, src, m);
    }
    dst_meta->cur_len = m;
    dst[m] = '\0';
    return m;
}

/**
 * fss_cpy_raw: Copies buffer of known length to dst with truncation.
 */
FSS_FORCEINLINE uint32_t fss_cpy_raw(char *dst, vb_meta_t *dst_meta, const char *src, uint32_t src_len) {
    if (!dst || !dst_meta) return 0;
    uint32_t m = (src_len > dst_meta->max_len) ? dst_meta->max_len : src_len;
    if (m > 0 && src) {
        fss_memcpy_inline(dst, src, m);
    }
    dst_meta->cur_len = m;
    dst[m] = '\0';
    return m;
}

/**
 * fss_cpychar: Overwrites dst with a single character.
 */
FSS_FORCEINLINE uint32_t fss_cpychar(char *dst, vb_meta_t *meta, char ch) {
    if (!dst || !meta) return 0;
    if (meta->max_len >= 1) {
        dst[0] = ch;
        dst[1] = '\0';
        meta->cur_len = 1;
        return 1;
    } else {
        dst[0] = '\0';
        meta->cur_len = 0;
        return 0;
    }
}

/**
 * fss_cat: Appends FSS string src to dst. O(1) concatenation without scanning.
 */
FSS_FORCEINLINE uint32_t fss_cat(char *dst, vb_meta_t *dst_meta, const char *src, const vb_meta_t *src_meta) {
    if (!dst || !dst_meta) return 0;
    uint32_t cur = dst_meta->cur_len;
    uint32_t max = dst_meta->max_len;
    if (cur >= max) return cur;
    uint32_t slen = src_meta ? src_meta->cur_len : 0;
    uint32_t space = max - cur;
    uint32_t m = (slen < space) ? slen : space;
    if (m > 0 && src) {
        fss_memcpy_inline(dst + cur, src, m);
        cur += m;
        dst_meta->cur_len = cur;
        dst[cur] = '\0';
    }
    return cur;
}

/**
 * fss_cat_raw: Appends raw buffer of known length to dst.
 */
FSS_FORCEINLINE uint32_t fss_cat_raw(char *dst, vb_meta_t *dst_meta, const char *src, uint32_t src_len) {
    if (!dst || !dst_meta) return 0;
    uint32_t cur = dst_meta->cur_len;
    uint32_t max = dst_meta->max_len;
    if (cur >= max) return cur;
    uint32_t space = max - cur;
    uint32_t m = (src_len < space) ? src_len : space;
    if (m > 0 && src) {
        fss_memcpy_inline(dst + cur, src, m);
        cur += m;
        dst_meta->cur_len = cur;
        dst[cur] = '\0';
    }
    return cur;
}

/**
 * fss_cat_cstr: Appends null-terminated C-string to dst.
 */
FSS_FORCEINLINE uint32_t fss_cat_cstr(char *dst, vb_meta_t *dst_meta, const char *cstr) {
    if (!dst || !dst_meta || !cstr) return dst_meta ? dst_meta->cur_len : 0;
    uint32_t cur = dst_meta->cur_len;
    uint32_t max = dst_meta->max_len;
    if (cur >= max) return cur;
    uint32_t space = max - cur;
    uint32_t clen = (uint32_t)strlen(cstr);
    uint32_t m = (clen < space) ? clen : space;
    if (m > 0) {
        fss_memcpy_inline(dst + cur, cstr, m);
        cur += m;
        dst_meta->cur_len = cur;
        dst[cur] = '\0';
    }
    return cur;
}

/**
 * fss_cat_lit: Appends literal to dst.
 */
FSS_FORCEINLINE uint32_t fss_cat_lit(char *dst, vb_meta_t *dst_meta, const char *lit, uint32_t lit_len) {
    return fss_cat_raw(dst, dst_meta, lit, lit_len);
}

/**
 * fss_catchar: Appends a single character to dst in O(1).
 */
FSS_FORCEINLINE uint32_t fss_catchar(char *dst, vb_meta_t *meta, char ch) {
    if (!dst || !meta) return 0;
    uint32_t cur = meta->cur_len;
    if (cur < meta->max_len) {
        dst[cur++] = ch;
        dst[cur] = '\0';
        meta->cur_len = cur;
        return cur;
    }
    return cur;
}

/* -------------------------------------------------------------------------- */
/* Comparison Operations                                                      */
/* -------------------------------------------------------------------------- */

/**
 * fss_cmp: Length-aware lexicographical string comparison.
 * Returns <0 if a < b, 0 if a == b, >0 if a > b.
 */
FSS_FORCEINLINE int fss_cmp(const char *a, uint32_t al, const char *b, uint32_t bl) {
    if (a == b) {
        return (al < bl) ? -1 : ((al > bl) ? 1 : 0);
    }
    uint32_t min_l = (al < bl) ? al : bl;
    if (min_l > 0) {
        int r = fss_memcmp_inline(a, b, min_l);
        if (r != 0) return r;
    }
    return (al < bl) ? -1 : ((al > bl) ? 1 : 0);
}

/**
 * fss_cmp_meta: Compares two strings using their metadata descriptors.
 */
FSS_FORCEINLINE int fss_cmp_meta(const char *a, const vb_meta_t *ma, const char *b, const vb_meta_t *mb) {
    uint32_t al = ma ? ma->cur_len : 0;
    uint32_t bl = mb ? mb->cur_len : 0;
    return fss_cmp(a, al, b, bl);
}

/**
 * fss_cmp_lit: Compares string against literal.
 */
FSS_FORCEINLINE int fss_cmp_lit(const char *a, uint32_t al, const char *lit, uint32_t lit_len) {
    return fss_cmp(a, al, lit, lit_len);
}

/**
 * fss_cmpchar: Compares first char of string against ch.
 */
FSS_FORCEINLINE int fss_cmpchar(const char *a, uint32_t al, char ch) {
    if (al == 0 || !a) return -1;
    return (int)(unsigned char)a[0] - (int)(unsigned char)ch;
}

/* -------------------------------------------------------------------------- */
/* Substring and View Slicing                                                 */
/* -------------------------------------------------------------------------- */

/**
 * fss_substr: Extracts substring into dst buffer.
 */
FSS_FORCEINLINE uint32_t fss_substr(char *dst, vb_meta_t *dst_meta,
                                    const char *src, uint32_t src_len,
                                    uint32_t start, uint32_t len) {
    if (!dst || !dst_meta) return 0;
    if (start >= src_len || !src) {
        dst_meta->cur_len = 0;
        dst[0] = '\0';
        return 0;
    }
    uint32_t avail = src_len - start;
    uint32_t l = (len < avail) ? len : avail;
    uint32_t m = (l < dst_meta->max_len) ? l : dst_meta->max_len;
    if (m > 0) {
        fss_memcpy_inline(dst, src + start, m);
    }
    dst_meta->cur_len = m;
    dst[m] = '\0';
    return m;
}

/**
 * fss_view_calc_start: Computes safe view starting offset.
 */
FSS_FORCEINLINE uint32_t fss_view_calc_start(uint32_t src_len, uint32_t start) {
    return (start >= src_len) ? src_len : start;
}

/**
 * fss_view_calc_len: Computes safe view length bounded by source.
 */
FSS_FORCEINLINE uint32_t fss_view_calc_len(uint32_t src_len, uint32_t start, uint32_t req_len) {
    if (start >= src_len) return 0;
    uint32_t avail = src_len - start;
    return (req_len < avail) ? req_len : avail;
}

/**
 * fss_view: Returns non-owning slice view.
 */
FSS_FORCEINLINE fss_view_t fss_view(const char *src, uint32_t src_len, uint32_t start, uint32_t len) {
    fss_view_t v;
    if (!src || start >= src_len) {
        v.data = "";
        v.len = 0;
        return v;
    }
    uint32_t avail = src_len - start;
    v.data = src + start;
    v.len = (len < avail) ? len : avail;
    return v;
}

/* -------------------------------------------------------------------------- */
/* Search & Transformation Operations                                         */
/* -------------------------------------------------------------------------- */

/**
 * fss_find_char: Finds first occurrence of ch in string. Returns 0-based index or -1.
 */
FSS_FORCEINLINE int32_t fss_find_char(const char *src, uint32_t src_len, char ch) {
    if (!src || src_len == 0) return -1;
    const void *p = memchr(src, (unsigned char)ch, src_len);
    if (!p) return -1;
    return (int32_t)((const char *)p - src);
}

/**
 * fss_rfind_char: Finds last occurrence of ch in string. Returns 0-based index or -1.
 */
FSS_FORCEINLINE int32_t fss_rfind_char(const char *src, uint32_t src_len, char ch) {
    if (!src || src_len == 0) return -1;
    for (uint32_t i = src_len; i > 0; --i) {
        if (src[i - 1] == ch) return (int32_t)(i - 1);
    }
    return -1;
}

/**
 * fss_find_str: Finds first occurrence of substring needle in haystack.
 */
FSS_FORCEINLINE int32_t fss_find_str(const char *haystack, uint32_t hlen, const char *needle, uint32_t nlen) {
    if (!haystack || !needle) return -1;
    if (nlen == 0) return 0;
    if (nlen > hlen) return -1;
    if (nlen == 1) return fss_find_char(haystack, hlen, needle[0]);

    const char first = needle[0];
    uint32_t max_i = hlen - nlen;
    for (uint32_t i = 0; i <= max_i; ++i) {
        const char *p = (const char *)memchr(haystack + i, (unsigned char)first, max_i - i + 1);
        if (!p) return -1;
        i = (uint32_t)(p - haystack);
        if (fss_memcmp_inline(p, needle, nlen) == 0) {
            return (int32_t)i;
        }
    }
    return -1;
}

/**
 * fss_starts_with: Checks if string starts with prefix.
 */
FSS_FORCEINLINE int fss_starts_with(const char *src, uint32_t src_len, const char *prefix, uint32_t prefix_len) {
    if (!src || !prefix) return 0;
    if (prefix_len > src_len) return 0;
    if (prefix_len == 0) return 1;
    return fss_memcmp_inline(src, prefix, prefix_len) == 0 ? 1 : 0;
}

/**
 * fss_ends_with: Checks if string ends with suffix.
 */
FSS_FORCEINLINE int fss_ends_with(const char *src, uint32_t src_len, const char *suffix, uint32_t suffix_len) {
    if (!src || !suffix) return 0;
    if (suffix_len > src_len) return 0;
    if (suffix_len == 0) return 1;
    return fss_memcmp_inline(src + (src_len - suffix_len), suffix, suffix_len) == 0 ? 1 : 0;
}

/**
 * fss_toupper: Converts string to uppercase.
 */
FSS_FORCEINLINE uint32_t fss_toupper(char *dst, vb_meta_t *dst_meta, const char *src, uint32_t src_len) {
    if (!dst || !dst_meta || !src) return 0;
    uint32_t m = (src_len < dst_meta->max_len) ? src_len : dst_meta->max_len;
    for (uint32_t i = 0; i < m; ++i) {
        unsigned char c = (unsigned char)src[i];
        dst[i] = (char)((c >= 'a' && c <= 'z') ? (c - ('a' - 'A')) : c);
    }
    dst_meta->cur_len = m;
    dst[m] = '\0';
    return m;
}
/**
 * fss_tolower: Converts string to lowercase.
 */
FSS_FORCEINLINE uint32_t fss_tolower(char *dst, vb_meta_t *dst_meta, const char *src, uint32_t src_len) {
    if (!dst || !dst_meta || !src) return 0;
    uint32_t m = (src_len < dst_meta->max_len) ? src_len : dst_meta->max_len;
    for (uint32_t i = 0; i < m; ++i) {
        unsigned char c = (unsigned char)src[i];
        dst[i] = (char)((c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c);
    }
    dst_meta->cur_len = m;
    dst[m] = '\0';
    return m;
}

/**
 * fss_ltrim: Returns view with leading whitespace trimmed.
 */
FSS_FORCEINLINE fss_view_t fss_ltrim(const char *src, uint32_t src_len) {
    fss_view_t v;
    if (!src || src_len == 0) {
        v.data = "";
        v.len = 0;
        return v;
    }
    uint32_t start = 0;
    while (start < src_len && ((unsigned char)src[start] <= ' ' && isspace((unsigned char)src[start]))) {
        start++;
    }
    v.data = src + start;
    v.len = src_len - start;
    return v;
}

/**
 * fss_rtrim: Returns view with trailing whitespace trimmed.
 */
FSS_FORCEINLINE fss_view_t fss_rtrim(const char *src, uint32_t src_len) {
    fss_view_t v;
    if (!src || src_len == 0) {
        v.data = "";
        v.len = 0;
        return v;
    }
    uint32_t end = src_len;
    while (end > 0 && ((unsigned char)src[end - 1] <= ' ' && isspace((unsigned char)src[end - 1]))) {
        end--;
    }
    v.data = src;
    v.len = end;
    return v;
}

/**
 * fss_trim: Returns view with both leading and trailing whitespace trimmed.
 */
FSS_FORCEINLINE fss_view_t fss_trim(const char *src, uint32_t src_len) {
    fss_view_t v;
    if (!src || src_len == 0) {
        v.data = "";
        v.len = 0;
        return v;
    }
    uint32_t start = 0;
    while (start < src_len && ((unsigned char)src[start] <= ' ' && isspace((unsigned char)src[start]))) {
        start++;
    }
    uint32_t end = src_len;
    while (end > start && ((unsigned char)src[end - 1] <= ' ' && isspace((unsigned char)src[end - 1]))) {
        end--;
    }
    v.data = src + start;
    v.len = end - start;
    return v;
}

/**
 * fss_from_uint: Formats unsigned 64-bit integer into dst.
 */
FSS_FORCEINLINE uint32_t fss_from_uint(char *dst, vb_meta_t *meta, uint64_t val) {
    if (!dst || !meta || meta->max_len == 0) return 0;
    char buf[32];
    uint32_t pos = 32;
    if (val == 0) {
        buf[--pos] = '0';
    } else {
        while (val > 0) {
            buf[--pos] = (char)('0' + (val % 10));
            val /= 10;
        }
    }
    uint32_t len = 32 - pos;
    return fss_cpy_raw(dst, meta, buf + pos, len);
}

/**
 * fss_from_int: Formats signed 64-bit integer into dst.
 */
FSS_FORCEINLINE uint32_t fss_from_int(char *dst, vb_meta_t *meta, int64_t val) {
    if (!dst || !meta || meta->max_len == 0) return 0;
    char buf[32];
    uint32_t pos = 32;
    uint64_t uval;
    int neg = 0;
    if (val < 0) {
        neg = 1;
        uval = (uint64_t)(-(val + 1)) + 1;
    } else {
        uval = (uint64_t)val;
    }
    if (uval == 0) {
        buf[--pos] = '0';
    } else {
        while (uval > 0) {
            buf[--pos] = (char)('0' + (uval % 10));
            uval /= 10;
        }
    }
    if (neg) {
        buf[--pos] = '-';
    }
    uint32_t len = 32 - pos;
    return fss_cpy_raw(dst, meta, buf + pos, len);
}

/* -------------------------------------------------------------------------- */
/* Backwards-Compatible Macro API (100% Standard C / C++ Compliant)           */
/* -------------------------------------------------------------------------- */

/**
 * DCL(name, size) / dcl(name, size):
 * Declares stack buffer and associated metadata dope vector dv_<name>.
 */
#define DCL(name, size) \
    ALIGN_16 char name[(size) + 1]; \
    vb_meta_t dv_##name = { 0, (uint32_t)(size) }; \
    (name)[0] = '\0'

#define dcl DCL

#define LEN(name)    ((dv_##name).cur_len)
#define len(name)    ((dv_##name).cur_len)

#define MAXLEN(name) ((dv_##name).max_len)
#define maxlen(name) ((dv_##name).max_len)

/**
 * SET(dst, "literal") / set(dst, "literal")
 */
#define SET(dst, lit) do { \
    fss_set_lit((dst), &(dv_##dst), ("" lit), (uint32_t)(sizeof(lit) - 1)); \
} while(0)

#define set SET

/**
 * CPY(dst, src) / cpy(dst, src)
 */
#define CPY(dst, src) do { \
    fss_cpy((dst), &(dv_##dst), (src), &(dv_##src)); \
} while(0)

#define cpy CPY

/**
 * CPYCHAR(dst, ch) / cpychar(dst, ch)
 */
#define CPYCHAR(dst, ch) do { \
    fss_cpychar((dst), &(dv_##dst), (char)(ch)); \
} while(0)

#define cpychar CPYCHAR

/**
 * CAT(dst, src) / cat(dst, src)
 */
#define CAT(dst, src) do { \
    fss_cat((dst), &(dv_##dst), (src), &(dv_##src)); \
} while(0)

#define cat CAT

/**
 * CATCHAR(dst, ch) / catchar(dst, ch)
 */
#define CATCHAR(dst, ch) do { \
    fss_catchar((dst), &(dv_##dst), (char)(ch)); \
} while(0)

#define catchar CATCHAR

/**
 * CLEAR(name) / clear(name)
 */
#define CLEAR(name) do { \
    fss_clear((name), &(dv_##name)); \
} while(0)

#define clear CLEAR

/**
 * VIEW(name, src, start, len):
 * Zero-copy slice creation creating local pointer and metadata descriptor.
 */
#define VIEW(name, src, start, len) \
    uint32_t _vstart_##name = fss_view_calc_start((dv_##src).cur_len, (uint32_t)(start)); \
    uint32_t _vlen_##name   = fss_view_calc_len((dv_##src).cur_len, (uint32_t)(start), (uint32_t)(len)); \
    char *name = (char *)(src) + _vstart_##name; \
    vb_meta_t dv_##name = { _vlen_##name, _vlen_##name }

/**
 * SUBSTR(dst, src, start, len) / substr(dst, src, start, len)
 */
#define SUBSTR(dst, src, start, len) do { \
    fss_substr((dst), &(dv_##dst), (src), (dv_##src).cur_len, (uint32_t)(start), (uint32_t)(len)); \
} while(0)

#define substr SUBSTR

/**
 * C-String Interop: CPY_CSTR, CAT_CSTR, CAT_LIT
 */
#define CPY_CSTR(dst, cstr) do { \
    fss_set_cstr((dst), &(dv_##dst), (cstr)); \
} while(0)

#define cpy_cstr CPY_CSTR

#define CAT_CSTR(dst, cstr) do { \
    fss_cat_cstr((dst), &(dv_##dst), (cstr)); \
} while(0)

#define cat_cstr CAT_CSTR

#define CAT_LIT(dst, lit) do { \
    fss_cat_lit((dst), &(dv_##dst), ("" lit), (uint32_t)(sizeof(lit) - 1)); \
} while(0)

#define cat_lit CAT_LIT

/**
 * Comparison Macros: CMP, CMP_LIT, CMPCHAR
 * Safe expressions without non-standard GCC statement expressions.
 */
#define CMP(a, b) (fss_cmp((a), (dv_##a).cur_len, (b), (dv_##b).cur_len))
#define cmp CMP

#define CMP_LIT(a, lit) (fss_cmp_lit((a), (dv_##a).cur_len, ("" lit), (uint32_t)(sizeof(lit) - 1)))
#define cmp_lit CMP_LIT

#define CMPCHAR(a, ch) (fss_cmpchar((a), (dv_##a).cur_len, (char)(ch)))
#define cmpchar CMPCHAR

/**
 * Debugging / Diagnostic Macro
 */
#define FSS_DEBUG(x) \
    printf(#x " = [%.*s] (cur_len=%u max_len=%u)\n", \
           (int)(dv_##x).cur_len, (x), (unsigned)(dv_##x).cur_len, (unsigned)(dv_##x).max_len)

#define dumpvar(x) FSS_DEBUG(x)

/* -------------------------------------------------------------------------- */
/* High-Speed I/O Macro Integration                                           */
/* -------------------------------------------------------------------------- */

#if defined(__has_include)
  #if __has_include("vbx_file.h")
    #include "vbx_file.h"
  #endif
#endif

#ifndef VBX_FILE_H
struct vb_handle;
typedef struct vb_handle vb_handle_t;
int VB_Get(vb_handle_t *vb, void *buf, uint32_t max_len, uint32_t *out_len);
int VB_Put(vb_handle_t *vb, const void *data, uint32_t len);
int VB_Skip(vb_handle_t *vb, uint32_t *skipped_len);
#endif
#ifndef GET_REC
#define GET_REC(h, name) \
    VB_Get((h), (name), (uint32_t)(sizeof(name) - 1), &(dv_##name).cur_len)
#endif

#ifndef PUT_REC
#define PUT_REC(h, name) \
    VB_Put((h), (name), (dv_##name).cur_len)
#endif

#ifndef FIND_REC
#define FIND_REC(h, skip_count, stat) do { \
    uint32_t _i; (stat) = 1; \
    for (_i = 0; _i < (uint32_t)(skip_count); _i++) { \
        if (VB_Skip((h), NULL) <= 0) { (stat) = 0; break; } \
    } \
} while(0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* FASTSTR_H */
