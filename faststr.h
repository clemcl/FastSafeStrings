#ifndef FASTSTR_BEST_H
#define FASTSTR_BEST_H

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

// Any __builtin_ function used here will optimize to raw assembly instructions

#if defined(__clang__)
  /* clang doesn't support builtin either */
//  #define __builtin_memcpy   memcpy
  #pragma inline
#elif __BORLANDC__
//  #pragma option -6
//  #pragma inline
  #pragma intrinsic memcpy
  #define __builtin_memcpy   memcpy
//  #define __builtin_constant_p 
#elif defined(__GNUC__)
 #pragma GCC optimize ("O2")
#elif defined(__xclang__)
  #define __builtin_memcmp   memcmp
#elif defined(__IBMC__) || defined(__xlC__)
  /* XLC handles inlining via JCL optimization parameters */
  #define __builtin_memcpy   memcpy
#endif

//  #define __builtinmemcpy   memcpy
 
/* -------------------------------------------------- */
/* 1. Metadata & Structures                           */
/* -------------------------------------------------- */

// Configurable null-termination (from faststr.h)
#ifndef FASTSTR_KEEP_TERMINATOR
#define FASTSTR_TERM(dst,len) ((void)0)
#else
#define FASTSTR_TERM(dst,len) ((dst)[(len)] = '\0')
#endif

typedef struct {
    uint32_t cur_len;
    uint32_t max_len;
} vb_meta_t; //

typedef struct {
    char *data;
    vb_meta_t *meta;
} fss_string; //

/* -------------------------------------------------- */
/* 2. Tiered Memory Movement                          */
/* -------------------------------------------------- */

#define FSS_MOVE_FAST(dest, src, len) do { \
    size_t _l = (len); \
    char *_d = (char*)(dest); \
    const char *_s = (const char*)(src); \
    /* Just do it. Modern CPUs handle the alignment internally. */ \
    while (_l >= 8) { \
        *(uint64_t*)_d = *(uint64_t*)_s; \
        _d += 8; _s += 8; _l -= 8; \
    } \
    while (_l--) *_d++ = *_s++; \
} while(0)
 
// 8-byte Unrolled Move for large blocks (from faststr.h)
// awful code. degrades to single byte copy if not on 8-byte boundary!
#define FSS_MOVE_FASTcvc(dest, src, len) do { \
    size_t _l = (len); \
    char *_d = (char*)(dest); \
    const char *_s = (const char*)(src); \
    if (_l >= 64 && (((uintptr_t)_d | (uintptr_t)_s) & 7) == 0) { \
        while (_l >= 8) { \
            *(uint64_t*)_d = *(uint64_t*)_s; \
            _d += 8; _s += 8; _l -= 8; \
        } \
    } \
    while (_l--) *_d++ = *_s++; \
} while(0)


/* -------------------------------------------------- */
/* 2. Tiered Memory Movement (Fixed for GCC)          */
/* -------------------------------------------------- */

#define FSS_COPY_BEST(dst, src, len) do { \
        __builtin_memcpy(dst, src, len); \
} while(0)
 
/* -------------------------------------------------- */
/* 3. Core API (DCL, SET, CPY, CAT)                   */
/* -------------------------------------------------- */

#define LEN(name) (dv_##name.cur_len)    
 
// 1. Define a separate macro for the alignment attribute based on the compiler
#if defined(__clang__) || defined(__GNUC__)
    #define ALIGN_16 __attribute__((aligned(16)))
#else
    #define ALIGN_16 
#endif

// 2. Use that cleanly inside your main macro definition
#define DCL(name, size) \
    ALIGN_16 char name[(size) + 1] = {0}; \
    vb_meta_t dv_##name = {0, (size)}; \
    fss_string fs_##name = { name, &dv_##name }
 
#define DCLx(name, size) \
    char name[(size) + 1] = {0}; \
    vb_meta_t dv_##name = {0, (size)}; \
    fss_string fs_##name = { name, &dv_##name } 

#define SET(dst, lit) do { \
    uint32_t _llen = (uint32_t)(sizeof(lit)-1); \
    uint32_t _m = (_llen > dv_##dst.max_len) ? dv_##dst.max_len : _llen; \
    __builtin_memcpy(dst, lit, _m); \
    dv_##dst.cur_len = _m; \
    FASTSTR_TERM(dst, _m); \
} while(0)
 

#define CPY(dst, src) do { \
    uint32_t _m = (dv_##src.cur_len > dv_##dst.max_len) ? dv_##dst.max_len : dv_##src.cur_len; \
    __builtin_memcpy(dst, src, _m ); \
    dv_##dst.cur_len = _m; \
    FASTSTR_TERM(dst, _m); \
} while(0) //

 
 
#define CAT(dst, src) do { \
    uint32_t _dlen = dv_##dst.cur_len; \
    uint32_t _space = (dv_##dst.max_len > _dlen) ? (dv_##dst.max_len - _dlen) : 0; \
    uint32_t _m = (dv_##src.cur_len > _space) ? _space : dv_##src.cur_len; \
    if (_m > 0) { \
        __builtin_memcpy((dst + _dlen), src, _m); \
        dv_##dst.cur_len = _dlen + _m; \
        FASTSTR_TERM(dst, dv_##dst.cur_len); \
    } \
} while(0) //

/* -------------------------------------------------- */
/* 4. Advanced Features (VIEW, Substr, C-Strings)     */
/* -------------------------------------------------- */

// Zero-copy substring (from faststrnew.h)
#define VIEW(name, src, start, len) \
    char *name = (src) + ((start) > dv_##src.cur_len ? dv_##src.cur_len : (start)); \
    vb_meta_t dv_##name = { \
        ((len) < (dv_##src.cur_len - ((start) > dv_##src.cur_len ? dv_##src.cur_len : (start)))) ? \
        (len) : (dv_##src.cur_len - ((start) > dv_##src.cur_len ? dv_##src.cur_len : (start))), \
        dv_##name.cur_len \
    }

// C-String Interoperability (from faststrnew.h)
#define CPY_CSTR(dst, cstr) do { \
    const char *_scstr = (cstr); \
    uint32_t _lencstr = (uint32_t)strlen(_scstr); \
    uint32_t _mcstr = (_lencstr < dv_##dst.max_len) ? _lencstr : dv_##dst.max_len; \
    __builtin_memcpy(dst, _scstr, _mcstr); \
    dv_##dst.cur_len = _mcstr; \
    FASTSTR_TERM(dst, _mcstr); \
} while(0)

/* -------------------------------------------------- */
/* 5. Comparison & I/O Helpers                        */
/* -------------------------------------------------- */

/* 
#define CMP(a,b) FSS_CMP(a,&dv_##a,b,&dv_##b)


#define FSS_CMP(a, am, b, bm, ret) do { \
    uint32_t _al = (am)->cur_len; \
    uint32_t _bl = (bm)->cur_len; \
    uint32_t _min = (_al < _bl) ? _al : _bl; \
    int _r = __builtin_memcmp((a), (b), _min); \
    if (_r != 0) { \
        (ret) = _r; \
    } else { \
        (ret) = (_al < _bl) ? -1 : ((_al > _bl) ? 1 : 0); \
    } \
} while (0)
*/ 

#define CMP(a, b) ({ \
    uint32_t _al = dv_##a.cur_len; \
    uint32_t _bl = dv_##b.cur_len; \
    uint32_t _min = (_al < _bl) ? _al : _bl; \
    int _r = __builtin_memcmp((a), (b), _min); \
    if (_r != 0) \
        _r; \
    else \
        (_al < _bl) ? -1 : ((_al > _bl) ? 1 : 0); \
})
 
/*---------------------------------------------
 * Debug helper
 *---------------------------------------------*/
#define FSS_DEBUG(x) \
    printf(#x " = [%.*s] (len=%u)\n", \
           (int)dv_##x.cur_len, (x), dv_##x.cur_len)

#define dumpvar(x) \
    printf(#x " = [%.*s] (len=%u)\n", \
           (int)dv_##x.cur_len, (x), dv_##x.cur_len)

#define GET_REC(h, name, stat) VB_Get(h, name, sizeof(name), &stat) //

#endif
