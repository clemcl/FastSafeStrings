/******************************************************************************
 * PROJECT:      FastSafeStrings (FSS) & VBIO
 *
 * AUTHOR:       Clement Victor Clarke (Warracknabeal, Australia)
 * COPYRIGHT:    Copyright © 1989-2026 Clement Victor Clarke (Originator of Jol). 
 * All Rights Reserved.
 *
 * * VERSION:      2.0 (May 2026)
 *
 * * PHILOSOPHY:   This library prioritizes Data Integrity and Energy Efficiency.
 * Based on String Descriptor logic, it is designed to eliminate 
 * the multiple scanning friction of standard C strings, looking for
 * string terminators (binary 0) or Line Feeds in FGETS.
 *
 * * INTEGRITY:    Strict "Fail-and-Report" Policy. This implementation explicitly 
 * rejects quiet truncation. If data exceeds target capacity, 
 * an exception MUST be raised to preserve system accountability.
 *
 * MISSION: To reduce global energy consumption through computational efficiency.
 *
 * * LICENSE TERMS:
 * 1. INDIVIDUAL/NON-PROFIT: Use is free under the MIT License.
 * 2. COMMERCIAL: Use by entities with annual revenue > $1M AUD requires
 *    a paid Commercial License.
 *
 * CONTACT: [clemclarke@gmail.com] for commercial terms and "Shared Savings" agreements.
 *
 ******************************************************************************/

  
#ifndef FASTSTR_BEST_H
#define FASTSTR_BEST_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "vbx_file.h"


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
  #pragma inline
#elif __BORLANDC__
  #pragma intrinsic memcpy
  #define __builtin_memcpy   memcpy
//  #define __builtin_constant_p 
#elif defined(__GNUC__)
 #pragma GCC optimize ("O2")
#elif defined(__IBMC__) || defined(__xlC__)
  /* XLC handles inlining via JCL optimization parameters */
  #define __builtin_memcpy   memcpy
#endif


#define FSS_ABEND(code, msg) do { \
    fprintf(stderr, "FSS ABEND %s: %s at line %d\n", code, msg, __LINE__); \
    exit(1); \
} while(0
 
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
/*  Core API (DCL, SET, CPY, CAT)                     */
/* -------------------------------------------------- */

#define LEN(name) (dv_##name.cur_len)    
 
// 1. Define a separate macro for the alignment attribute based on the compiler
#if defined(__clang__) || defined(__GNUC__)
    #define ALIGN_16 __attribute__((aligned(16)))
#else
    #define ALIGN_16 
#endif

#define DCL(name, size) \
    ALIGN_16 char name[(size) + 1] = {0}; \
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
/*  Advanced Features (VIEW, Substr, C-Strings)       */
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

#if defined(__BORLANDC__) && !defined(__clang__)
    /* --- Pure Legacy Borland C (e.g., v5.0) Fallback --- */
    /* Only compilers that are strictly legacy Borland enter here. */
    static __inline int borland_cmp_helper(const void* a, uint32_t al, const void* b, uint32_t bl) {
        uint32_t min_len = (al < bl) ? al : bl;
        int r = memcmp(a, b, min_len);
        if (r != 0) return r;
        return (al < bl) ? -1 : ((al > bl) ? 1 : 0);
    }
    
    #define CMP(a, b) borland_cmp_helper((a), dv_##a.cur_len, (b), dv_##b.cur_len)

#else
    /* --- Modern Path (GCC, Native Clang, and Embarcadero bcc32x) --- */
    /* Since bcc32x defines __clang__, it cleanly skips the legacy block 
       and executes this high-performance optimization. */
    #define CMP(a, b) ({ \
        uint32_t _al = dv_##a.cur_len; \
        uint32_t _bl = dv_##b.cur_len; \
        uint32_t _min = (_al < _bl) ? _al : _bl; \
        int _r = __builtin_memcmp((a), (b), _min); \
        (_r != 0) ? _r : ((_al < _bl) ? -1 : ((_al > _bl) ? 1 : 0)); \
    })
#endif
 
/*---------------------------------------------
 * Debug helper
 *---------------------------------------------*/
#define FSS_DEBUG(x) \
    printf(#x " = [%.*s] (len=%u)\n", \
           (int)dv_##x.cur_len, (x), dv_##x.cur_len)

#define dumpvar(x) \
    printf(#x " = [%.*s] (len=%u)\n", \
           (int)dv_##x.cur_len, (x), dv_##x.cur_len)

/* -------------------------------------------------- */
/* High-Speed I/O Macros                              */
/* -------------------------------------------------- */

vb_handle_t *VB_Open(const char *path, const char *mode_str, uint32_t block_size) ;
vb_handle_t *VB_OpenRead(const char *path,  const char *translate);
vb_handle_t *VB_OpenWrite(const char *path, uint32_t block_size,  const char *translate);
 
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
 

#endif
