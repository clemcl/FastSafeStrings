#ifndef FASTSTR_H
#define FASTSTR_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>

/*
   FastSafeStrings
   ----------------

   Descriptor-based safe & fast string system for C

   Features:
       O(1) length
       safe truncation (no buffer overruns)
       descriptor-based design (PL/I-style)
       zero-copy substring views
       append (CAT) without strlen
       optional small-copy optimisation
*/

/* * Copyright (c) 1988-2026 Clement Victor Clarke (Originator of Jol)
 * Project: FastSafeStrings & vbio 
 * * LICENSE TERMS:
 * 1. INDIVIDUAL/NON-PROFIT: Use is free under the MIT License.
 * 2. COMMERCIAL: Use by entities with annual revenue > $1M AUD requires
 * a paid Commercial License.
 *
 * MISSION: To reduce global energy consumption through computational efficiency.
 * CONTACT: [clemclarke@gmail.com] for commercial terms and "Shared Savings" agreements.
 */
 
/* -------------------------------------------------- */
/* Terminator & Metadata                              */
/* -------------------------------------------------- */

#ifdef __BORLANDC__
  #pragma option -4
//  #pragma intrinsic memcpy
//  #pragma inline
  #define __builtin_memcpy   memcpy
  #define __builtin_constant_p 
//  #pragma intrinsic -a
#elif defined(__xlC__)
  /* XLC handles inlining via JCL optimization parameters */
  #define __builtin_memcpy   memcpy
#endif

 
#ifndef FASTSTR_KEEP_TERMINATOR
#define FASTSTR_TERM(dst,len) ((void)0)
#else
#define FASTSTR_TERM(dst,len) ((dst)[(len)] = '\0')
#endif

typedef struct {
    uint32_t cur_len;
    uint32_t max_len;
} vb_meta_t;

typedef struct {
    char *data;
    vb_meta_t *meta;
} fss_string;

/* -------------------------------------------------- */
/* The "Magic" Move Macros                            */
/* -------------------------------------------------- */

/* FSS_MOVE_PAD: Mainframe MVC style (Space Padded) */
#define FSS_MOVE_PAD(target, target_cap, src, src_len) {        \
    size_t _m = ((size_t)(src_len) > (size_t)(target_cap)) ?    \
                (size_t)(target_cap) : (size_t)(src_len);       \
    __builtin_memcpy((void*)(target), (const void*)(src), _m);  \
    if (_m < (size_t)(target_cap)) {                            \
        __builtin_memset((void*)((char*)(target) + _m), ' ',    \
                         (size_t)(target_cap) - _m);            \
    }                                                           \
}

/* FSS_MOVE_LEAN: Variable Length style (No Padding) */
#define FSS_MOVE_LEAN(target, target_cap, src, src_len) {       \
    size_t _m = ((size_t)(src_len) > (size_t)(target_cap)) ?    \
                (size_t)(target_cap) : (size_t)(src_len);       \
    __builtin_memcpy((void*)(target), (const void*)(src), _m);  \
}

#define FSS_MOVE_FAST(dest, src, len) do { \
    size_t _l = (len); \
    char *_d = (char*)(dest); \
    const char *_s = (const char*)(src); \
    /* Guard: Only use 8-byte move if pointers are aligned */ \
    if (_l >= 64 && (((uintptr_t)_d | (uintptr_t)_s) & 7) == 0) { \
        while (_l >= 8) { \
            *(uint64_t*)_d = *(uint64_t*)_s; \
            _d += 8; _s += 8; _l -= 8; \
        } \
    } \
    while (_l--) *_d++ = *_s++; \
} while(0)
 
#define FSS_MOVE_FASTold(dest, src, len) do { \
    size_t _l = (len); \
    char *_d = (char*)(dest); \
    const char *_s = (const char*)(src); \
    if (_l >= 64) { \
        /* Your 8-byte unrolled logic here for big blocks */ \
        while (_l >= 8) { \
            *(uint64_t*)_d = *(uint64_t*)_s; \
            _d += 8; _s += 8; _l -= 8; \
        } \
    } \
    /* Finish the remaining 0-7 bytes */ \
    while (_l--) *_d++ = *_s++; \
} while(0)
 
/* -------------------------------------------------- */
/* Declaration & Length                               */
/* -------------------------------------------------- */

#define DCLold(name,size) \
    char name[(size)+1] = {0}; \
    vb_meta_t dv_##name = {0,(size)}; \
    fss_string fs_##name = { name,&dv_##name }

#define DCL(name, size) \
    char name[(size) + 1] = {0}; \
    enum { name##_maxlen = (size) }; \
    vb_meta_t dv_##name = {0, (size)}; \
    fss_string fs_##name = { name, &dv_##name }
     
#define LEN(x) (dv_##x.cur_len)
#define MAX(x) (dv_##x.max_len)

/* -------------------------------------------------- */
/* Optimized Public Macros (Defaulting to LEAN)       */
/* -------------------------------------------------- */

/* Added to SET macro */
#define SET(dst, lit) do { \
    uint32_t _llen = (uint32_t)(sizeof(lit)-1); \
    if (_llen > dv_##dst.max_len) { \
        fprintf(stderr, "FSS WARNING: SET truncation on %s (%u > %u)\n", #dst, _llen, dv_##dst.max_len); \
        /* Optional: MVS_ABEND("S013", "Data Truncation"); */ \
    } \
    FSS_MOVE_LEAN(dst, dv_##dst.max_len, lit, _llen); \
    dv_##dst.cur_len = (_llen > dv_##dst.max_len) ? dv_##dst.max_len : _llen; \
    FASTSTR_TERM(dst, dv_##dst.cur_len); \
} while(0)
 
/* SET: Literal assignment */
#define SETold(dst, lit) do { \
    uint32_t _llen = (uint32_t)(sizeof(lit)-1); \
    FSS_MOVE_LEAN(dst, dv_##dst.max_len, lit, _llen); \
    dv_##dst.cur_len = (_llen > dv_##dst.max_len) ? dv_##dst.max_len : _llen; \
    FASTSTR_TERM(dst, dv_##dst.cur_len); \
} while(0)

#define CPYnew(dst, src) do { \
    uint32_t _slen = dv_##src.cur_len; \
    /* Optimization: If maxlen <= 256, compiler drops a single MVC/VMOV */ \
    if (__builtin_constant_p(_slen) || dst##_maxlen <= 256) { \
        uint32_t _m = (_slen > (uint32_t)dst##_maxlen) ? \
                      (uint32_t)dst##_maxlen : _slen; \
        __builtin_memcpy((void*)(dst), (const void*)(src), _m); \
        dv_##dst.cur_len = _m; \
    } else { \
        /* Fallback for very large buffers */ \
        FSS_MOVE_LEAN(dst, dv_##dst.max_len, src, _slen); \
        dv_##dst.cur_len = (_slen > dv_##dst.max_len) ? \
                           dv_##dst.max_len : _slen; \
    } \
    FASTSTR_TERM(dst, dv_##dst.cur_len); \
} while(0)
 
/* CPY: String to String copy */
#define CPY(dst, src) do { \
    FSS_MOVE_LEAN(dst, dv_##dst.max_len, src, dv_##src.cur_len); \
    dv_##dst.cur_len = (dv_##src.cur_len > dv_##dst.max_len) ? dv_##dst.max_len : dv_##src.cur_len; \
    FASTSTR_TERM(dst, dv_##dst.cur_len); \
} while(0)

/* SUBSTR: Extraction */
#define SUBSTR(dst, src, start, length) do { \
    uint32_t _s = (start); \
    uint32_t _l = (length); \
    if (_s < dv_##src.cur_len) { \
        uint32_t _avail = dv_##src.cur_len - _s; \
        uint32_t _to_mov = (_l > _avail) ? _avail : _l; \
        FSS_MOVE_LEAN(dst, dv_##dst.max_len, (src + _s), _to_mov); \
        dv_##dst.cur_len =                              \
        (_to_mov > dv_##dst.max_len) ? dv_##dst.max_len : _to_mov; \
    } else { \
        dv_##dst.cur_len = 0; \
    } \
    FASTSTR_TERM(dst, dv_##dst.cur_len); \
} while(0)

/* CAT: Append (Built-in memcpy) */
#define CAT(dst, src) do { \
    uint32_t _dlen = dv_##dst.cur_len; \
    uint32_t _slen = dv_##src.cur_len; \
    uint32_t _space = dv_##dst.max_len - _dlen; \
    uint32_t _mov = (_slen > _space) ? _space : _slen; \
    __builtin_memcpy((void*)(dst + _dlen), (void*)src, _mov); \
    dv_##dst.cur_len = _dlen + _mov; \
    FASTSTR_TERM(dst, dv_##dst.cur_len); \
} while(0)


/* -------------------------------------------------- */
/* High-Speed I/O Macros                              */
/* -------------------------------------------------- */

/* Reads a record directly into a DCL'd buffer
   and updates its Dope Vector.*/
/*  GET_REC(handle, dcl_name, status_var) */

#define GET_REC(h, name, stat)  \
    VB_Get(h, name, sizeof(name), &stat)

#define PUT_REC(h, name) \
    VB_Put((h), (name), dv_##name.cur_len)

#define FIND_REC(h, skip_count, stat) do { \
    uint32_t _i; (stat) = 1; \
    for (_i = 0; _i < (skip_count); _i++) { \
        if (VB_Skip((h), NULL) <= 0) { (stat) = 0; break; } \
    } \
} while(0)
 
/* -------------------------------------------------- */
/* Comparisons & Helpers                              */
/* -------------------------------------------------- */

#define CMP(a,b) fss_cmp(a,&dv_##a,b,&dv_##b)
static inline int fss_cmp(const char *a,
    const vb_meta_t *am, const char *b, const vb_meta_t *bm) {
    uint32_t al = am->cur_len, bl = bm->cur_len;
    uint32_t min = (al < bl) ? al : bl;
    int r = memcmp(a, b, min);
    if (r != 0) return r;
    return (al < bl) ? -1 : (al > bl ? 1 : 0);
}

#define EQ(a,b) (CMP(a,b)==0)
#define dumpvar(x)   \
printf("VARDUMP %-10s len=%u max=%u value='%.*s'\n", \
#x, dv_##x.cur_len, dv_##x.max_len, dv_##x.cur_len, x)

#endif
