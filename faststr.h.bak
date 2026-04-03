#ifndef FASTSTR_H
#define FASTSTR_H

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

/* -------------------------------------------------- */
/* Includes                                           */
/* -------------------------------------------------- */

#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* -------------------------------------------------- */
/* Terminator                                         */
/* -------------------------------------------------- */

#ifndef FASTSTR_KEEP_TERMINATOR
#define FASTSTR_TERM(dst,len) ((void)0)
#else
#define FASTSTR_TERM(dst,len) ((dst)[(len)] = '\0')
#endif

/* -------------------------------------------------- */
/* Metadata (descriptor / dope vector)                */
/* -------------------------------------------------- */

typedef struct
{
    uint32_t cur_len;
    uint32_t max_len;
} vb_meta_t;

/* -------------------------------------------------- */
/* Descriptor wrapper                                 */
/* -------------------------------------------------- */

typedef struct
{
    char *data;
    vb_meta_t *meta;
} fss_string;

/* -------------------------------------------------- */
/* Declaration                                        */
/* -------------------------------------------------- */

#define DCL(name,size) \
    char name[(size)+1] = {0}; \
    vb_meta_t dv_##name = {0,(size)}; \
    fss_string fs_##name = { name,&dv_##name }

/* helper */
#define STR(x) fs_##x

/* -------------------------------------------------- */
/* Length / capacity                                  */
/* -------------------------------------------------- */

#define LEN(x) (dv_##x.cur_len)
#define MAX(x) (dv_##x.max_len)

/* -------------------------------------------------- */
/* Optional small-copy optimisation                   */
/* -------------------------------------------------- */

static inline void fss_copy_small(char *dst,const char *src,uint32_t len)
{
    switch(len)
    {
        case 16: *(uint64_t*)(dst+8)=*(const uint64_t*)(src+8);
        case 8:  *(uint64_t*)dst   =*(const uint64_t*)src; break;

        case 7: dst[6]=src[6];
        case 6: dst[5]=src[5];
        case 5: dst[4]=src[4];
        case 4: *(uint32_t*)dst=*(const uint32_t*)src; break;

        case 3: dst[2]=src[2];
        case 2: *(uint16_t*)dst=*(const uint16_t*)src; break;

        case 1: dst[0]=src[0]; break;

        case 0: break;

        default:
            memcpy(dst,src,len);
    }
}

/* -------------------------------------------------- */
/* Core operations                                    */
/* -------------------------------------------------- */

static inline void fss_copy(char *dst,
                            vb_meta_t *dmeta,
                            const char *src,
                            const vb_meta_t *smeta)
{
    uint32_t len = smeta->cur_len;

    if (len > dmeta->max_len)
        len = dmeta->max_len;

#ifdef FSS_SMALL_COPY
    if (len <= 16)
        fss_copy_small(dst,src,len);
    else
#endif
        memcpy(dst,src,len);

    dmeta->cur_len = len;
    FASTSTR_TERM(dst,len);
}

/* literal */

static inline void fss_set_literal(char *dst,
                                   vb_meta_t *meta,
                                   const char *lit,
                                   uint32_t len)
{
    if (len > meta->max_len)
        len = meta->max_len;

#ifdef FSS_SMALL_COPY
    if (len <= 16)
        fss_copy_small(dst,lit,len);
    else
#endif
        memcpy(dst,lit,len);

    meta->cur_len = len;
    FASTSTR_TERM(dst,len);
}

/* -------------------------------------------------- */
/* APPEND (CAT)                                       */
/* -------------------------------------------------- */

static inline void fss_cat(char *dst,
                          vb_meta_t *dmeta,
                          const char *src,
                          const vb_meta_t *smeta)
{
    uint32_t dlen = dmeta->cur_len;
    uint32_t slen = smeta->cur_len;

    uint32_t space = dmeta->max_len - dlen;

    if (slen > space)
        slen = space;

    memcpy(dst + dlen, src, slen);

    dmeta->cur_len = dlen + slen;
    FASTSTR_TERM(dst,dmeta->cur_len);
}

/* -------------------------------------------------- */
/* Compare                                            */
/* -------------------------------------------------- */

static inline int fss_cmp(const char *a,
                          const vb_meta_t *am,
                          const char *b,
                          const vb_meta_t *bm)
{
    uint32_t al = am->cur_len;
    uint32_t bl = bm->cur_len;

    uint32_t min = (al < bl) ? al : bl;

    int r = memcmp(a,b,min);

    if (r != 0)
        return r;

    if (al < bl) return -1;
    if (al > bl) return 1;

    return 0;
}

/* -------------------------------------------------- */
/* Zero-copy views                                    */
/* -------------------------------------------------- */

typedef struct
{
    const char *data;
    uint32_t len;
} fss_view;

static inline fss_view fss_view_of(fss_string s)
{
    fss_view v = { s.data, s.meta->cur_len };
    return v;
}

static inline fss_view fss_view_sub(const char *base,
                                    uint32_t start,
                                    uint32_t len)
{
    fss_view v = { base + start, len };
    return v;
}

static inline int fss_view_cmp(fss_view a,fss_view b)
{
    uint32_t min = (a.len < b.len) ? a.len : b.len;

    int r = memcmp(a.data,b.data,min);

    if (r != 0) return r;

    if (a.len < b.len) return -1;
    if (a.len > b.len) return 1;

    return 0;
}

/* copy view → string */

static inline void fss_copy_view(char *dst,
                                 vb_meta_t *meta,
                                 fss_view v)
{
    uint32_t len = v.len;

    if (len > meta->max_len)
        len = meta->max_len;

#ifdef FSS_SMALL_COPY
    if (len <= 16)
        fss_copy_small(dst,v.data,len);
    else
#endif
        memcpy(dst,v.data,len);

    meta->cur_len = len;
    FASTSTR_TERM(dst,len);
}

/* -------------------------------------------------- */
/* Public macros                                      */
/* -------------------------------------------------- */

/* High-Speed I/O Macros --- */
/* Reads a record directly into a DCL'd buffer and updates its Dope Vector.*/
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
 
#define SET(dst,lit) \
    fss_set_literal(dst,&dv_##dst,lit,sizeof(lit)-1)

#define CPY(dst,src) \
_Generic((src), \
    fss_view: fss_copy_view, \
    default: fss_copy \
)(dst,&dv_##dst,src,&dv_##src)

#define CAT(dst,src) \
    fss_cat(dst,&dv_##dst,src,&dv_##src)

#define CMP(a,b) \
    fss_cmp(a,&dv_##a,b,&dv_##b)

#define EQ(a,b) (CMP(a,b)==0)
#define NE(a,b) (CMP(a,b)!=0)
#define LT(a,b) (CMP(a,b)<0)
#define GT(a,b) (CMP(a,b)>0)

/* -------------------------------------------------- */
/* Substring                                          */
/* -------------------------------------------------- */

#define SUBSTR(dst,src,start,len)                  \
do {                                               \
    uint32_t _s = (start);                         \
    uint32_t _l = (len);                           \
                                                   \
    if (_s < dv_##src.cur_len) {                   \
        uint32_t _avail = dv_##src.cur_len - _s;   \
        if (_l > _avail) _l = _avail;              \
        if (_l > dv_##dst.max_len) _l = dv_##dst.max_len; \
                                                   \
        memcpy(dst, src + _s, _l);                 \
        dv_##dst.cur_len = _l;                     \
    }                                              \
    else                                           \
        dv_##dst.cur_len = 0;                      \
                                                   \
    FASTSTR_TERM(dst,dv_##dst.cur_len);            \
} while(0)

/* -------------------------------------------------- */
/* strlen dispatch                                    */
/* -------------------------------------------------- */

static inline size_t fss_strlen_impl(fss_string s)
{
    return s.meta->cur_len;
}

#define fstrlen(x) \
_Generic((x), \
    fss_string: fss_strlen_impl, \
    default: strlen \
)(x)

/* -------------------------------------------------- */
/* Debug                                              */
/* -------------------------------------------------- */

#define dumpvar(x) \
printf("VARDUMP %-10s len=%u max=%u value='%s'\n", \
       #x, dv_##x.cur_len, dv_##x.max_len, x)

#endif
