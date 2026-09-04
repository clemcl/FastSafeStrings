/******************************************************************************
 * PROJECT:      FastSafeStrings (FSS) & VBIO
 *
 * AUTHOR:       Clement Victor Clarke (Warracknabeal, Australia)
 * COPYRIGHT:    Copyright (c) 1983-2026 Clement Victor Clarke (Originator of Jol).
 * All Rights Reserved.
 *
 * VERSION:      2.2 (August 2026)
 *
 * PHILOSOPHY:   This library prioritizes Data Integrity and Energy Efficiency.
 *               Based on String Descriptor logic, it is designed to eliminate
 *               the multiple scanning friction of standard C strings, looking for
 *               string terminators (binary 0) or Line Feeds in FGETS.
 *
 * INTEGRITY:    Quiet truncation policy. If data exceeds target capacity,
 *               the write is silently clamped to the maximum. No exception is
 *               raised. Use FSS_DEBUG() to inspect lengths at runtime.
 *
 * MISSION:      To reduce global energy consumption through computational
 *               efficiency.
 *
 * LICENSE TERMS:
 *   1. INDIVIDUAL/NON-PROFIT: Use is free under the MIT License.
 *   2. COMMERCIAL: Use by entities with annual revenue > $1M AUD requires
 *      a paid Commercial License.
 *
 * CONTACT:      clemclarke@gmail.com for commercial terms and
 *               "Shared Savings" agreements.
 *
 * CHANGES v2.1:
 *   - Fixed FSS_ABEND: missing closing ')' on while(0) (would not compile)
 *   - Fixed VIEW: self-reference of uninitialised dv_ struct (undefined behaviour)
 *   - Fixed SUBSTR: missing null terminator after memcpy
 *   - Fixed GET_REC: silently-ignored stat parameter documented and cleaned up
 *   - Removed CAT_LITold (dead code)
 *   - Removed no-op '#pragma inline' on Clang path
 *   - Added comment identifying Embarcadero (Clang-based Borland) path
 *   - Made dumpvar() an alias of FSS_DEBUG() (was a duplicate)
 *   - Added CLEAR() macro (documented in README but missing from header)
 *   - Added CATCHAR() macro (documented in README but missing from header)
 *   - Added FSS_VERSION_MAJOR / FSS_VERSION_MINOR / FSS_VERSION_STR
 *   - SET() documented: literals only — use CPY_CSTR() for char* variables
 *   - Added CPYCHAR(dst, ch) — overwrite dst with a single character
 *   - Added CMPCHAR(a, ch) — byte compare of a's first char against ch
 *   - CPYLIT: Same as SET 
 *
 * CHANGES v2.2:
 *   - Added lower case options for macros.  Example, dcl or DCL, cmp or CMP
 *   - Changed: Using SIZEOF instead of maxlen to generate and smaller faster code 
 *
 ******************************************************************************/

#define _CRT_SECURE_NO_WARNINGS

#ifndef FASTSTR_BEST_H
#define FASTSTR_BEST_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>       /* for exit() used in FSS_ABEND */
#include "vbx_file.h"

/*
   FastSafeStrings
   ----------------
   Descriptor-based safe & fast string system for C.

   Features:
       O(1) length access (no strlen)
       O(1) append (CAT) — no destination scanning
       Safe truncation — no buffer overruns
       Descriptor-based design (PL/I / MVS Dope Vector style)
       Zero-copy substring views (VIEW)
       C-string interoperability (CPY_CSTR, CAT_CSTR)
       Works alongside existing C code — incremental adoption

   NOTE on SET():
       SET(dst, "literal") uses sizeof() to get the literal length at
       compile time — zero cost, no scanning. Do NOT pass a char* variable
       to SET(); use CPY_CSTR(dst, charptr) instead.
*/

/* -------------------------------------------------- */
/* Version                                            */
/* -------------------------------------------------- */

#define FSS_VERSION_MAJOR 2
#define FSS_VERSION_MINOR 2
#define FSS_VERSION_STR   "2.2"

/* -------------------------------------------------- */
/* Compiler detection & intrinsic mapping             */
/* -------------------------------------------------- */

/*
 * __builtin_memcpy / __builtin_memcmp are used throughout.
 * On GCC and Clang these expand directly to inline SIMD instructions
 * at -O2 or higher without a function call.
 * On other compilers we alias them to the standard library equivalents.
 *
 * Clang: no pragma needed — Clang inlines aggressively at -O2/-O3.
 * GCC:   hint O2 via pragma so the header works even without -O flags.
 * Borland legacy (BCC32, no Clang): map builtins to memcpy/memcmp.
 * Embarcadero C++ Builder (Clang-based Borland): treated as Clang.
 * IBM XLC / z/OS: map builtins; inlining controlled via JCL OPTLEVEL.
 */

#if defined(__clang__)
    /* Clang (including Embarcadero C++ Builder): no pragma needed */

#elif defined(__BORLANDC__)
    /* Pure legacy Borland BCC32 (not Clang-based) */
    #pragma  -P          /* Force C++ mode for inline keyword support */
    #pragma  -Oi         /* Expand intrinsics inline */
    #pragma intrinsic(memcpy)
    #pragma intrinsic(memcmp)
    #define __builtin_memcpy   memcpy
    #define __builtin_memcmp   memcmp

#elif defined(__GNUC__)
    #pragma GCC optimize ("O2")

#elif defined(__IBMC__) || defined(__xlC__)
    /* IBM XLC / z/OS: inlining via JCL OPTLEVEL parameter */
    #define __builtin_memcpy   memcpy
    #define __builtin_memcmp   memcmp

#endif

/* -------------------------------------------------- */
/* Alignment helper                                   */
/* -------------------------------------------------- */

#if defined(__clang__) || defined(__GNUC__)
    #define ALIGN_16 __attribute__((aligned(16)))
#else
    #define ALIGN_16
#endif

/* -------------------------------------------------- */
/* Min helper (safe, no side effects)                 */
/* -------------------------------------------------- */

#define FSS_MIN(a,b) ((a) < (b) ? (a) : (b))

/* -------------------------------------------------- */
/* Abort macro                                        */
/* -------------------------------------------------- */

/* FIX: was missing closing ')' on while(0) — would not compile */
#define FSS_ABEND(code, msg) do { \
    fprintf(stderr, "FastSafeStrings ABEND %s: %s at line %d\n", \
            code, msg, __LINE__); \
    exit(1); \
} while(0)

/* -------------------------------------------------- */
/* 1. Metadata & Structures                           */
/* -------------------------------------------------- */

/*
 * vb_meta_t — the "Dope Vector" that travels with every FSS string.
 *   cur_len : current content length (bytes, excluding null terminator)
 *   max_len : allocated capacity (bytes, excluding null terminator)
 */
typedef struct {
    uint32_t cur_len;
    uint32_t max_len;
} vb_meta_t;

/*
 * fss_string — pointer-based handle for passing FSS strings to functions.
 * Declared by DCL() alongside every buffer.
 * Future use: pass fs_##name to functions that accept fss_string* so they
 * can operate on any FSS string without knowing its compile-time size.
 */
typedef struct {
    char     *data;
    vb_meta_t *meta;
} fss_string;

/* -------------------------------------------------- */
/*  Core API (DCL, SET, CPY, CAT, CLEAR, CATCHAR)     */
/* -------------------------------------------------- */

/* LEN(name)    — current length of string, O(1) */
#define LEN(name)    (dv_##name.cur_len)
#define len(name)    (dv_##name.cur_len)

/* MAXLEN(name) — maximum capacity of string, O(1) */
#define MAXLEN(name) (dv_##name.max_len)
#define maxlen(name) (dv_##name.max_len)

/*
 * DCL(name, size)
 *   Declares a stack-allocated FSS string buffer of 'size' bytes capacity.
 *   Also declares:
 *     dv_##name  — the Dope Vector (cur_len=0, max_len=size)
 *     fs_##name  — an fss_string handle for passing to functions
 *   Buffer is 16-byte aligned for SIMD efficiency.
 *   Always null-terminated (size+1 bytes allocated).
 */
     
#define DCL(name, size) \
    ALIGN_16 char name[(size) + 1]; \
    vb_meta_t dv_##name = { 0, (size) }; \
    name[0] = '\0'

#define dcl DCL 

/*
 * SET(dst, "literal")
 *   Assign a string literal to dst. Uses sizeof() at compile time —
 *   zero scanning cost. Truncates silently if literal > capacity.
 *
 *   WARNING: 'lit' MUST be a string literal, not a char* variable.
 *            sizeof(char*) is 8 on 64-bit — not the string length.
 *            Use CPY_CSTR(dst, charptr) for runtime char* sources.
 */
#define SET(dst, lit) do { \
    uint32_t _llen = (uint32_t)(sizeof(lit) - 1); \
    uint32_t _m = (_llen > dv_##dst.max_len) ? dv_##dst.max_len : _llen; \
    __builtin_memcpy(dst, lit, _m); \
    dv_##dst.cur_len = _m; \
    (dst)[_m] = '\0'; \
} while(0)

#define set SET 

/*
 * CPY(dst, src)
 *   Copy FSS string src into dst. Truncates if src > dst capacity.
 *   Both src and dst must be DCL'd strings.
 */
#define CPY(dst, src) do { \
    if (__builtin_constant_p(dv_##src.cur_len) && dv_##src.cur_len <= (sizeof(dst) - 1)) { \
        /* STATIC FAST-PATH: The compiler proves it fits ahead of time. */ \
        /* No runtime bounds-checking or conditional branches are generated! */ \
        __builtin_memcpy(dst, src, dv_##src.cur_len); \
        dv_##dst.cur_len = dv_##src.cur_len; \
        (dst)[dv_##src.cur_len] = '\0'; \
    } else { \
        /* DYNAMIC SAFE-PATH: Fallback for unpredictable runtime lengths. */ \
        uint32_t _m = (dv_##src.cur_len > (sizeof(dst) - 1)) \
                      ? (sizeof(dst) - 1) : dv_##src.cur_len; \
        __builtin_memcpy(dst, src, _m); \
        dv_##dst.cur_len = _m; \
        (dst)[_m] = '\0'; \
    } \
} while(0)
 
#define CPYborl(dst, src) do { \
    uint32_t _m = (dv_##src.cur_len > (sizeof(dst) - 1)) \
                  ? (sizeof(dst) - 1) : dv_##src.cur_len; \
    __builtin_memcpy(dst, src, _m); \
    dv_##dst.cur_len = _m; \
    (dst)[_m] = '\0'; \
} while(0)

#define cpy CPY               

/*
 * CPYCHAR(dst, ch)
 *   Overwrite dst with a single character. dst becomes length 1.
 *   If dst has zero capacity (max_len==0), dst is left empty instead.
 */
#define CPYCHAR(dst, ch) do { \
    if (dv_##dst.max_len >= 1) { \
        (dst)[0] = (char)(ch); \
        dv_##dst.cur_len = 1; \
        (dst)[1] = '\0'; \
    } else { \
        dv_##dst.cur_len = 0; \
        (dst)[0] = '\0'; \
    } \
} while(0)

#define cpychar CPYCHAR               

/*
 * CAT(dst, src)
 *   Append FSS string src to dst. O(1) — uses stored lengths, no scanning.
 *   Truncates silently if combined length exceeds dst capacity.
 */
#define CAT(dst, src) do { \
    if (__builtin_constant_p(dv_##src.cur_len)) { \
        /* STATIC CONSTANT FAST-PATH */ \
        /* The compiler knows src's length at compile-time. */ \
        uint32_t _slen  = dv_##src.cur_len; \
        uint32_t _dlen  = dv_##dst.cur_len; \
        uint32_t _space = (sizeof(dst) - 1) - _dlen; \
        uint32_t _m     = (_slen < _space) ? _slen : _space; \
        if (_m) { \
            __builtin_memcpy((dst) + _dlen, (src), _m); \
            dv_##dst.cur_len = _dlen + _m; \
            (dst)[_dlen + _m] = '\0'; \
        } \
    } else { \
        /* DYNAMIC SAFE RUNTIME FALLBACK */ \
        uint32_t _dlen  = dv_##dst.cur_len; \
        uint32_t _slen  = dv_##src.cur_len; \
        uint32_t _space = (sizeof(dst) - 1) - _dlen; \
        uint32_t _m     = (_slen < _space) ? _slen : _space; \
        if (_m) { \
            __builtin_memcpy((dst) + _dlen, (src), _m); \
            _dlen += _m; \
            dv_##dst.cur_len = _dlen; \
            (dst)[_dlen] = '\0'; \
        } \
    } \
} while (0)
 
#define CATborland(dst, src) do { \
    uint32_t _dlen  = dv_##dst.cur_len; \
    uint32_t _slen  = dv_##src.cur_len; \
    uint32_t _space = (sizeof(dst) - 1) - _dlen; \
    uint32_t _m     = (_slen < _space) ? _slen : _space; \
    if (_m) { \
        __builtin_memcpy((dst) + _dlen, (src), _m); \
        _dlen += _m; \
        dv_##dst.cur_len = _dlen; \
        (dst)[_dlen] = '\0'; \
    } \
} while (0)
 
#define cat CAT                

/*
 * CATCHAR(dst, ch)
 *   Append a single character to dst. O(1).
 *   Does nothing silently if dst is full.
 */
#define CATCHAR(dst, ch) do { \
    if (dv_##dst.cur_len < dv_##dst.max_len) { \
        (dst)[dv_##dst.cur_len++] = (char)(ch); \
        (dst)[dv_##dst.cur_len]   = '\0'; \
    } \
} while(0)

#define catchar CATCHAR

/*
 * CLEAR(name)
 *   Reset string to empty (length=0). Does not zero the buffer,
 *   just resets the Dope Vector and writes a null at position 0.
 */
#define CLEAR(name) do { \
    dv_##name.cur_len = 0; \
    (name)[0] = '\0'; \
} while(0)

#define clear CLEAR

/* -------------------------------------------------- */
/*  Advanced Features (VIEW, SUBSTR, C-String interop)*/
/* -------------------------------------------------- */

/*
 * VIEW(name, src, start, len)
 *   Zero-copy substring view into src. No memory is allocated or copied.
 *   'name' becomes a char* pointing into src's buffer, with its own
 *   Dope Vector reflecting the view's length.
 *
 *   The view is READ-ONLY in intent — writing through it modifies src.
 *   Do not CAT or SET into a VIEW; use SUBSTR for a writable copy.
 *
 *   FIX: previous version referenced dv_##name.cur_len during its own
 *   initialisation (undefined behaviour). Now uses a staged local variable.
 */
#define VIEW(name, src, start, len) \
    uint32_t _vstart_##name = ((uint32_t)(start) > dv_##src.cur_len \
                               ? dv_##src.cur_len : (uint32_t)(start)); \
    uint32_t _vlen_##name   = FSS_MIN((uint32_t)(len), \
                                      dv_##src.cur_len - _vstart_##name); \
    char *name = (src) + _vstart_##name; \
    vb_meta_t dv_##name = { _vlen_##name, _vlen_##name }

/*
 * SUBSTR(dst, src, start, len)
 *   Copy a substring of src into dst (writable copy).
 *   start and len are zero-based byte offsets.
 *   Clamps to available data and dst capacity.
 *
 */
#define SUBSTR(dst, src, start, len) do { \
    uint32_t _s      = (uint32_t)(start); \
    uint32_t _srclen = dv_##src.cur_len; \
    if (_s > _srclen) _s = _srclen; \
    uint32_t _avail  = _srclen - _s; \
    uint32_t _l      = FSS_MIN((uint32_t)(len), _avail); \
    uint32_t _m      = FSS_MIN(_l, dv_##dst.max_len); \
    if (_m > 0) \
        __builtin_memcpy((dst), (src) + _s, _m); \
    dv_##dst.cur_len = _m; \
    (dst)[_m] = '\0'; \
} while(0)

#define substr SUBSTR

/*
 * CPY_CSTR(dst, cstr)
 *   Copy a standard null-terminated C string into an FSS string.
 *   Use this (not SET) when the source is a runtime char* variable.
 *   Requires one strlen() scan — unavoidable for unknown-length C strings.
 */
#define CPY_CSTR(dst, cstr) do { \
    const char *_cs  = (cstr); \
    uint32_t _clen   = (uint32_t)strlen(_cs); \
    uint32_t _m      = (_clen < dv_##dst.max_len) \
                       ? _clen : dv_##dst.max_len; \
    __builtin_memcpy(dst, _cs, _m); \
    dv_##dst.cur_len = _m; \
    dst[_m] = '\0'; \
} while(0)

#define cpy_cstr CPY_CSTR

/*
 * CAT_CSTR(dst, cstr)
 *   Append a null-terminated C string to an FSS string.
 *   Requires one strlen() scan on the source — unavoidable.
 *   Truncates silently if combined length exceeds dst capacity.
 */
#define CAT_CSTR(dst, cstr) do { \
    const char *_cs   = (cstr); \
    uint32_t _dlen    = dv_##dst.cur_len; \
    uint32_t _space   = (_dlen < dv_##dst.max_len) \
                        ? (dv_##dst.max_len - _dlen) : 0; \
    if (_space > 0) { \
        uint32_t _clen = (uint32_t)strlen(_cs); \
        uint32_t _m    = (_clen < _space) ? _clen : _space; \
        if (_m > 0) { \
            __builtin_memcpy(dst + _dlen, _cs, _m); \
            dv_##dst.cur_len = _dlen + _m; \
            dst[dv_##dst.cur_len] = '\0'; \
        } \
    } \
} while(0)

#define cat_cstr CAT_CSTR

/*
 * CAT_LIT(dst, "literal")
 *   Append a string literal to dst. Uses sizeof() at compile time —
 *   zero scanning cost.
 *   WARNING: 'lit' must be a string literal. See SET() note above.
 */
#define CAT_LIT(dst, lit) do { \
    uint32_t _cur     = dv_##dst.cur_len; \
    uint32_t _max     = dv_##dst.max_len; \
    uint32_t _litlen  = (uint32_t)(sizeof(lit) - 1); \
    if (_cur < _max) { \
        uint32_t _rem = _max - _cur; \
        uint32_t _cpy = (_litlen < _rem) ? _litlen : _rem; \
        __builtin_memcpy((char *)(dst) + _cur, "" lit, _cpy); \
        _cur += _cpy; \
        dv_##dst.cur_len = _cur; \
        ((char *)(dst))[_cur] = '\0'; \
    } \
} while(0)

#define cat_lit CAT_LIT

/* -------------------------------------------------- */
/*  Comparison                                        */
/* -------------------------------------------------- */

/*
 * CMP(a, b)    — compare two FSS strings (like strcmp: <0, 0, >0)
 * CMP_LIT(a, "literal") — compare FSS string to a literal
 *
 * Borland legacy path uses an inline helper function (C++ mode, -P flag).
 * GCC/Clang path uses a statement expression for single-expression use.
 */

#if defined(__BORLANDC__) && !defined(__clang__)
    /* Legacy Borland BCC32 — C++ mode inline function */
    inline int fss_cmp_impl(const void *a, uint32_t al,
                            const void *b, uint32_t bl) {
        uint32_t minlen = (al < bl) ? al : bl;
        int r = memcmp(a, b, minlen);
        if (r != 0) return r;
        return (al < bl) ? -1 : ((al > bl) ? 1 : 0);
    }
    #define CMP(a, b) \
        fss_cmp_impl((a), dv_##a.cur_len, (b), dv_##b.cur_len)
    #define CMP_LIT(a, lit) \
        fss_cmp_impl((a), dv_##a.cur_len, (lit), (uint32_t)(sizeof(lit)-1))

#else
    /* GCC / Clang (including Embarcadero): statement expression */
 
    
    #define CMP(a, b) ({ \
        uint32_t _al  = dv_##a.cur_len; \
        uint32_t _bl  = dv_##b.cur_len; \
        uint32_t _min = (_al < _bl) ? _al : _bl; \
        int _r = __builtin_memcmp((a), (b), _min); \
        (_r != 0) ? _r : ((_al < _bl) ? -1 : ((_al > _bl) ? 1 : 0)); \
    })

    #define CMP_LIT(a, lit) ({ \
        uint32_t _al  = dv_##a.cur_len; \
        uint32_t _bl  = (uint32_t)(sizeof(lit) - 1); \
        uint32_t _min = (_al < _bl) ? _al : _bl; \
        int _r = __builtin_memcmp((a), (lit), _min); \
        (_r != 0) ? _r : ((_al < _bl) ? -1 : ((_al > _bl) ? 1 : 0)); \
    })

#endif

#define cmp_lit CMP_LIT

/*
 * CMPCHAR(a, ch)
 *   Compare the first byte of FSS string a against a single character.
 *   Returns 0 if equal, negative if a is empty or a[0] < ch, positive if
 *   a[0] > ch. This is a plain byte compare — it does NOT check whether
 *   a's length is exactly 1; use CMP/CMP_LIT if you need a true
 *   length-aware comparison. Same compiler portability as CPYCHAR — no
 *   statement-expression needed, works identically everywhere.
 */
#define CMPCHAR(a, ch) \
    ((dv_##a.cur_len == 0) ? -1 : \
     ((int)(unsigned char)(a)[0] - (int)(unsigned char)(ch)))

#define cmpchar CMPCHAR

/* -------------------------------------------------- */
/*  Debug helpers                                     */
/* -------------------------------------------------- */

/*
 * FSS_DEBUG(name) — print name, content, cur_len, max_len to stdout.
 * dumpvar(name)   — alias of FSS_DEBUG for backwards compatibility.
 */
#define FSS_DEBUG(x) \
    printf(#x " = [%.*s] (cur_len=%u max_len=%u)\n", \
           (int)dv_##x.cur_len, (x), dv_##x.cur_len, dv_##x.max_len)

#define dumpvar(x) FSS_DEBUG(x)

/* -------------------------------------------------- */
/* High-Speed I/O Macros                              */
/* -------------------------------------------------- */

/*
 * VB file API — forward declarations.
 * Implemented in vb_io.c / vbx_file.c.
 */
vb_handle_t *VB_Open(const char *path, const char *mode_str,
                     uint32_t block_size);
vb_handle_t *VB_OpenRead(const char *path, const char *translate);
vb_handle_t *VB_OpenWrite(const char *path, uint32_t block_size,
                           const char *translate);

/*
 * GET_REC(h, name)
 *   Read one VB record into DCL'd buffer 'name'.
 *   Automatically updates dv_##name.cur_len from the record length.
 *   Returns the number of bytes read, or <= 0 at end of file / error.
 *
 *   FIX: previous version accepted a 'stat' parameter that was silently
 *   ignored (commented out internally). The parameter has been removed.
 *   Check the return value directly if you need status:
 *       if (GET_REC(h, buf) <= 0) { ... EOF or error ... }
 */
#define GET_REC(h, name) \
    VB_Get((h), (name), (uint32_t)sizeof(name), &dv_##name.cur_len)

/*
 * PUT_REC(h, name)
 *   Write FSS string 'name' as a VB record.
 *   Uses dv_##name.cur_len — no scanning.
 */
#define PUT_REC(h, name) \
    VB_Put((h), (name), dv_##name.cur_len)

/*
 * FIND_REC(h, skip_count, stat)
 *   Skip 'skip_count' records in VB file h.
 *   Sets stat=1 if all skips succeeded, stat=0 on early EOF/error.
 */
#define FIND_REC(h, skip_count, stat) do { \
    uint32_t _i; (stat) = 1; \
    for (_i = 0; _i < (uint32_t)(skip_count); _i++) { \
        if (VB_Skip((h), NULL) <= 0) { (stat) = 0; break; } \
    } \
} while(0)

/* -------------------------------------------------- */
/* BCC32 Optimisation Reference (documentation only)  */
/* -------------------------------------------------- */
/*
    -O1  Optimize for small code size
    -O2  Optimize for fast speed
    -Ob  Eliminate dead stores and unreachable code
    -Oc  Common subexpression elimination
    -Oi  Expand intrinsics inline (memcpy, memset, etc.)
    -Ol  Loop optimisation (register-based loops)
    -Om  Invariant code motion
    -Op  Promote variables to registers
    -Ov  Induction variable optimisation / pointer simplification
*/

#endif /* FASTSTR_BEST_H */
