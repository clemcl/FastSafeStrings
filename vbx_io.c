/******************************************************************************
 * PROJECT:      FastSafeStrings (FSS) & VBIO
 *
 * AUTHOR:       Clement Victor Clarke (Warracknabeal, Australia)
 * COPYRIGHT:    Copyright (c) 1989-2026 Clement Victor Clarke (Originator of Jol).
 * All Rights Reserved.
 *
 * VERSION:      2.1 (June 2026)
 *
 * PHILOSOPHY:   This library prioritizes Data Integrity and Energy Efficiency.
 *               Based on String Descriptor logic, it eliminates the repeated
 *               scanning friction of standard C strings (searching for null
 *               terminators or newlines in fgets).
 *
 * INTEGRITY:    "Fail loudly" policy for I/O errors. Unrecoverable conditions
 *               (RDW corruption, record exceeds block size) call MVS_ABEND
 *               and terminate immediately. Recoverable conditions (EOF,
 *               record larger than the caller's buffer) return -1 or 0 for
 *               the caller to handle.
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
 * NOTES ON THIS VERSION:
 *   This is a deliberately simple baseline. It uses plain stdio
 *   (fopen/fread/fwrite/fclose) throughout — no platform-specific headers,
 *   no raw POSIX descriptors. Benchmarked against raw read()/write() at
 *   32KB+ block sizes: no measurable speed difference (within run-to-run
 *   noise), so there is no performance cost to keeping this simple.
 *
 *   The experimental MVS-native text-mode backend (which lets a runtime
 *   like JCC or PDPCLIB strip RDWs transparently via fgets()) has been
 *   deliberately left OUT of this version. It will come back as a clearly
 *   separated, opt-in addition once it's ready to be re-integrated — this
 *   file is the stable foundation that work will sit on top of, not a
 *   replacement for it.
 *
 ******************************************************************************/

#define _CRT_SECURE_NO_WARNINGS

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "vbx_file.h"

/* -------------------------------------------------- */
/* Compiler intrinsic mapping                         */
/* -------------------------------------------------- */

/*
 * __builtin_memcpy is used below for raw (non-translated) copies. On GCC
 * and Clang this expands to an inlined SIMD copy at -O2 or higher with no
 * function call. Other compilers don't have this builtin, so it's mapped
 * back to plain memcpy for them.
 */
#ifdef __BORLANDC__
  #pragma option -6
  #define __builtin_memcpy   memcpy
#elif defined(__xlC__)
  /* XLC handles inlining via JCL optimization parameters */
  #define __builtin_memcpy   memcpy
#endif

/* -------------------------------------------------- */
/* Internal: block I/O                                */
/* -------------------------------------------------- */

/**
 * vb_flush — write the current block buffer to disk.
 *
 * Every physical block on disk is prefixed with a 4-byte Block Descriptor
 * Word (BDW):
 *   bytes 0-1: total block length (payload + 4 for the BDW itself), big-endian
 *   bytes 2-3: reserved (zero)
 *
 * This matches the IBM Variable Blocked (VB) physical record format used
 * on z/OS and in VB dataset transfers — it's what lets a reader figure out
 * where one block ends and the next begins without scanning for a delimiter.
 *
 * Returns 0 on success, -1 on write error.
 */
static int vb_flush(vb_handle_t *vb) {
    if (vb->block_used == 0) return 0;   /* nothing buffered, nothing to do */

    uint32_t bdw_len = vb->block_used + 4;
    uint8_t  bdw[4]  = {
        (uint8_t)(bdw_len >> 8),
        (uint8_t)(bdw_len & 0xFF),
        0, 0
    };

    if (fwrite(bdw, 1, 4, vb->fp) != 4)                       return -1;
    if (fwrite(vb->block_buf, 1, vb->block_used, vb->fp)
              != vb->block_used)                              return -1;

    vb->block_used = 0;   /* buffer is now empty, ready for the next block */
    return 0;
}

/**
 * vb_fill_block — read the next physical block from disk into block_buf.
 *
 * Reads the 4-byte BDW first to find out how much payload follows, then
 * reads exactly that many bytes. This is the read-side mirror of vb_flush.
 *
 * Returns 1 on success, 0 at EOF, -1 on format or I/O error.
 */
static int vb_fill_block(vb_handle_t *vb) {
    uint8_t bdw[4];

    if (fread(bdw, 1, 4, vb->fp) != 4) return 0;   /* EOF or short read */

    uint32_t block_len   = ((uint32_t)bdw[0] << 8) | (uint32_t)bdw[1];
    uint32_t payload_len = block_len - 4;

    if (payload_len > vb->block_size) return -1;  /* corrupt or oversized block */

    if (fread(vb->block_buf, 1, payload_len, vb->fp) != payload_len) return -1;

    vb->block_used = payload_len;
    vb->block_pos  = 0;
    return 1;
}

/* -------------------------------------------------- */
/* Public API — open / close                          */
/* -------------------------------------------------- */

/**
 * VB_Open — open a VB file for reading or writing.
 *
 * mode_str examples:
 *   "r"              — read, no translation
 *   "w"              — write, no translation
 *   "r,codeset=037"  — read with EBCDIC-037 -> ASCII translation
 *   "w,codeset=1047" — write with ASCII -> EBCDIC-1047 translation
 *
 * Returns a heap-allocated vb_handle_t on success, NULL on failure.
 * Most callers should use VB_OpenRead / VB_OpenWrite instead of calling
 * this directly — they build the mode_str for you.
 */
vb_handle_t *VB_Open(const char *path, const char *mode_str,
                     uint32_t block_size) {
    vb_handle_t *vb = (vb_handle_t *)calloc(1, sizeof(vb_handle_t));
    if (!vb) return NULL;

    /* Determine read or write mode from the first character of mode_str */
    if (mode_str[0] == 'w') {
        vb->fp   = fopen(path, "wb");
        vb->mode = VB_MODE_WRITE;
    } else {
        vb->fp   = fopen(path, "rb");
        vb->mode = VB_MODE_READ;
    }

    if (vb->fp == NULL) {
        perror("Error opening file");          
        free(vb);
        MVS_ABEND("S013", "File open failed");
        return NULL;   /* unreachable after ABEND, but satisfies the compiler */
    }

    /* Parse optional codepage specification */
    if (strstr(mode_str, "codeset=037")) {
        vb->trans_table = (vb->mode == VB_MODE_WRITE)
                          ? ASCII_TO_EBCDIC_037
                          : EBCDIC_037_TO_ASCII;
    } else if (strstr(mode_str, "codeset=1047")) {
        vb->trans_table = (vb->mode == VB_MODE_WRITE)
                          ? ASCII_TO_EBCDIC_1047
                          : EBCDIC_1047_TO_ASCII;
    }
    /* trans_table remains NULL (no translation) if no codeset was given */

    vb->block_size = block_size;
    vb->block_buf  = (uint8_t *)malloc(block_size);

    if (!vb->block_buf) {   /* out of memory — clean up rather than crash later */
        fclose(vb->fp);
        free(vb);
        return NULL;
    }

    return vb;
}

/**
 * VB_OpenRead — convenience wrapper for read-mode opens.
 *   translate: "" (none), "037", or "1047"
 */
vb_handle_t *VB_OpenRead(const char *path, const char *translate) {
    char openmode[20] = "r";

    if (translate[0] != '\0') {
        if (strcmp(translate, "037") == 0) {
            strcpy(openmode, "r,codeset=037");
        } else if (strcmp(translate, "1047") == 0) {
            strcpy(openmode, "r,codeset=1047");
        } else {
            MVS_ABEND("S013",
                "Invalid translate table: must be \"\", \"037\", or \"1047\"");
        }
    }

    return VB_Open(path, openmode, 32768);
}

/**
 * VB_OpenWrite — convenience wrapper for write-mode opens.
 *   block_size: output block buffer size (e.g. 32768)
 *   translate:  "" (none), "037", or "1047"
 */
vb_handle_t *VB_OpenWrite(const char *path, uint32_t block_size,
                           const char *translate) {
    char openmode[20] = "w";

    if (translate[0] != '\0') {
        if (strcmp(translate, "037") == 0) {
            strcpy(openmode, "w,codeset=037");
        } else if (strcmp(translate, "1047") == 0) {
            strcpy(openmode, "w,codeset=1047");
        } else {
            MVS_ABEND("S013",
                "Invalid translate table: must be \"\", \"037\", or \"1047\"");
        }
    }

    return VB_Open(path, openmode, block_size);
}

/**
 * VB_Close — flush pending output (if write mode), close the file, and
 * free all heap resources. Safe to call with NULL.
 */
void VB_Close(vb_handle_t *vb) {
    if (!vb || vb->fp == NULL) return;

    if (vb->mode == VB_MODE_WRITE) {
        if (vb_flush(vb) < 0) {
            fprintf(stderr,
                "VB_Close: final flush failed — data may be incomplete\n");
        }
    }

    fclose(vb->fp);
    vb->fp = NULL;

    if (vb->block_buf) {
        free(vb->block_buf);
        vb->block_buf = NULL;
    }

    free(vb);
}

/* -------------------------------------------------- */
/* Public API — record I/O                            */
/* -------------------------------------------------- */

/**
 * VB_Put — write one variable-length record to the VB file.
 *
 * Every logical record is prefixed with a 4-byte Record Descriptor Word
 * (RDW):
 *   bytes 0-1: total record length (data + 4), big-endian
 *   bytes 2-3: reserved (zero)
 *
 * If the current block buffer can't hold the new RDW + data, the block is
 * flushed to disk first, then the record goes into a fresh block.
 *
 * If a codeset translation table is set, the data is translated
 * byte-by-byte (ASCII -> EBCDIC) on the way into the buffer. The loop is
 * unrolled by 8 to cut down on loop-overhead and branch mispredictions —
 * worthwhile here because this runs once per byte of every record written.
 *
 * Returns 1 on success, -1 on flush error. Calls MVS_ABEND if a single
 * record is larger than the block buffer itself — that's a configuration
 * error (block_size set too small), not a transient I/O failure.
 */
int VB_Put(vb_handle_t *vb, const void *data, uint32_t len) {
    uint32_t rdw_len = len + 4;

    if (rdw_len > vb->block_size) {
        MVS_ABEND("S013", "Record exceeds maximum block size");
        return -1;   /* unreachable after ABEND */
    }

    if (vb->block_used + rdw_len > vb->block_size) {
        if (vb_flush(vb) < 0) return -1;
    }

    uint8_t *p = vb->block_buf + vb->block_used;

    /* Write the RDW header for this record */
    p[0] = (uint8_t)(rdw_len >> 8);
    p[1] = (uint8_t)(rdw_len & 0xFF);
    p[2] = 0;
    p[3] = 0;

    uint8_t       *dest = p + 4;
    const uint8_t *src  = (const uint8_t *)data;
    const uint8_t *tbl  = vb->trans_table;

    if (tbl) {
        uint32_t i = 0;
        if (len >= 8) {
            for (; i <= len - 8; i += 8) {
                dest[i]   = tbl[src[i]];   dest[i+1] = tbl[src[i+1]];
                dest[i+2] = tbl[src[i+2]]; dest[i+3] = tbl[src[i+3]];
                dest[i+4] = tbl[src[i+4]]; dest[i+5] = tbl[src[i+5]];
                dest[i+6] = tbl[src[i+6]]; dest[i+7] = tbl[src[i+7]];
            }
        }
        for (; i < len; i++) dest[i] = tbl[src[i]];   /* remaining tail bytes */
    } else {
        __builtin_memcpy(dest, src, len);
    }

    vb->block_used += rdw_len;
    return 1;
}

/**
 * VB_GetLocate — zero-copy record read ("Locate Mode").
 *
 * Returns a pointer straight into the internal block buffer rather than
 * copying the data out. This is the fastest way to process records when
 * no translation is needed — the caller reads the record in place.
 *
 * The returned pointer is valid only until the next VB call on this handle
 * (the next fill_block overwrites the buffer it points into).
 *
 * Returns 1 on success, 0 at EOF, -1 on RDW corruption (calls MVS_ABEND).
 */
int VB_GetLocate(vb_handle_t *vb, const char **ptr, uint32_t *len) {
    if (vb->block_pos + 4 > vb->block_used) {
        if (vb_fill_block(vb) <= 0) return 0;   /* EOF or read error */
    }

    uint8_t  *p       = vb->block_buf + vb->block_pos;
    uint32_t  rdw_len = ((uint32_t)p[0] << 8) | (uint32_t)p[1];

    /*
     * Sanity check: an RDW must be at least 4 (itself) and must not claim
     * more data than we actually have buffered. If this fails, the data
     * on disk is corrupt — there's no safe way to keep reading, so we
     * abend rather than risk silently misinterpreting garbage as records.
     */
    if (rdw_len < 4 || (vb->block_pos + rdw_len > vb->block_used)) {
        MVS_ABEND("S0C4", "Record Descriptor Word (RDW) corruption detected");
        vb->error = 1;
        return -1;   /* unreachable after ABEND */
    }

    *len = rdw_len - 4;
    *ptr = (const char *)(p + 4);

    vb->block_pos += rdw_len;
    return 1;
}

/**
 * VB_Get — read one record into a caller-supplied buffer.
 *
 * Calls VB_GetLocate internally, then copies (and optionally translates)
 * the record data into buf. If no translation table is set, the output is
 * also null-terminated for convenience when treating records as C strings.
 *
 * Returns 1 on success, 0 at EOF, -1 if the record is larger than max_len.
 */
int VB_Get(vb_handle_t *vb, void *buf, uint32_t max_len, uint32_t *out_len) {
    const char *src_ptr;
    uint32_t    rlen;

    if (VB_GetLocate(vb, &src_ptr, &rlen) <= 0) return 0;
    if (rlen > max_len) return -1;

    uint8_t       *dest = (uint8_t *)buf;
    const uint8_t *src  = (const uint8_t *)src_ptr;
    const uint8_t *tbl  = vb->trans_table;

    if (tbl) {
        uint32_t i = 0;
        if (rlen >= 8) {
            for (; i <= rlen - 8; i += 8) {
                dest[i]   = tbl[src[i]];   dest[i+1] = tbl[src[i+1]];
                dest[i+2] = tbl[src[i+2]]; dest[i+3] = tbl[src[i+3]];
                dest[i+4] = tbl[src[i+4]]; dest[i+5] = tbl[src[i+5]];
                dest[i+6] = tbl[src[i+6]]; dest[i+7] = tbl[src[i+7]];
            }
        }
        for (; i < rlen; i++) dest[i] = tbl[src[i]];
    } else {
        __builtin_memcpy(dest, src, rlen);
        dest[rlen] = '\0';   /* only safe/meaningful for raw (non-EBCDIC) data */
    }

    if (out_len) *out_len = rlen;
    return 1;
}

/**
 * VB_Skip — advance past the current record without copying its data.
 *
 * Used when scanning past records you don't need — no memcpy, no
 * translation, just pointer arithmetic on the already-buffered block.
 *
 * Returns 1 on success, 0 at EOF, -1 on RDW error.
 */
int VB_Skip(vb_handle_t *vb, uint32_t *skipped_len) {
    if (vb->block_pos + 4 > vb->block_used) {
        if (vb_fill_block(vb) <= 0) return 0;
    }

    uint8_t  *p    = vb->block_buf + vb->block_pos;
    uint32_t  rlen = ((uint32_t)p[0] << 8) | (uint32_t)p[1];

    if (rlen < 4 || vb->block_pos + rlen > vb->block_used) {
        vb->error = 1;
        return -1;
    }

    vb->block_pos += rlen;

    if (skipped_len) *skipped_len = rlen - 4;

    return 1;
}

/* -------------------------------------------------- */
/* Code page translation tables                       */
/* -------------------------------------------------- */

/*
 * ASCII_TO_EBCDIC_037
 *
 * Maps 7-bit ASCII (0x00-0x7F) to IBM Code Page 037 (EBCDIC). This is the
 * standard code page for North American z/OS systems.
 *
 * The upper 128 entries (0x80-0xFF) are zero by design — plain 7-bit ASCII
 * text should never contain bytes in that range, so there's nothing
 * meaningful to map them to.
 */
const uint8_t ASCII_TO_EBCDIC_037[256] = {
    /* 00-0F */ 0x00,0x01,0x02,0x03,0x37,0x2D,0x2E,0x2F,
                0x16,0x05,0x25,0x0B,0x0C,0x0D,0x0E,0x0F,
    /* 10-1F */ 0x10,0x11,0x12,0x13,0x3C,0x3D,0x32,0x26,
                0x18,0x19,0x3F,0x27,0x1C,0x1D,0x1E,0x1F,
    /* 20-2F */ 0x40,0x5A,0x7F,0x7B,0x5B,0x6C,0x50,0x7D,
                0x4D,0x5D,0x5C,0x4E,0x6B,0x60,0x4B,0x61,
    /* 30-3F */ 0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,
                0xF8,0xF9,0x7A,0x5E,0x4C,0x7E,0x6E,0x6F,
    /* 40-4F */ 0x7C,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,
                0xC8,0xC9,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,
    /* 50-5F */ 0xD7,0xD8,0xD9,0xE2,0xE3,0xE4,0xE5,0xE6,
                0xE7,0xE8,0xE9,0xBA,0xE0,0xBB,0xB0,0x6D,
    /* 60-6F */ 0x79,0x81,0x82,0x83,0x84,0x85,0x86,0x87,
                0x88,0x89,0x91,0x92,0x93,0x94,0x95,0x96,
    /* 70-7F */ 0x97,0x98,0x99,0xA2,0xA3,0xA4,0xA5,0xA6,
                0xA7,0xA8,0xA9,0xC0,0x4F,0xD0,0xA1,0x07,
    /* 80-FF: zero by design — not valid 7-bit ASCII input */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

/*
 * ASCII_TO_EBCDIC_1047
 *
 * Maps 7-bit ASCII to IBM Code Page 1047 (EBCDIC Open Systems variant),
 * used by z/OS UNIX System Services (OMVS). The main practical difference
 * from 037: bracket characters [ ] { } ^ ~ map differently.
 *
 * The upper 128 entries are padded with 0x20 (EBCDIC space) rather than
 * left at zero, for Borland-compiler compatibility.
 */
const uint8_t ASCII_TO_EBCDIC_1047[256] = {
    0x00,0x01,0x02,0x03,0x37,0x2D,0x2E,0x2F,0x16,0x05,0x15,0x0B,0x0C,0x0D,0x0E,0x0F,
    0x10,0x11,0x12,0x13,0x3C,0x3D,0x32,0x26,0x18,0x19,0x3F,0x27,0x1C,0x1D,0x1E,0x1F,
    0x40,0x5A,0x7F,0x7B,0x5B,0x6C,0x50,0x7D,0x4D,0x5D,0x5C,0x4E,0x6B,0x60,0x4B,0x61,
    0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0x7A,0x5E,0x4C,0x7E,0x6E,0x6F,
    0x7C,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,
    0xD7,0xD8,0xD9,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xAD,0xE0,0xBD,0x5F,0x6D,
    0x79,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x91,0x92,0x93,0x94,0x95,0x96,
    0x97,0x98,0x99,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xC0,0x4F,0xD0,0xA1,0x07,
    /* 0x80-0xFF: padded with EBCDIC space (0x20) for Borland array compatibility */
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x5F,0x20,0x20,0x20,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

/*
 * EBCDIC_037_TO_ASCII
 *
 * Inverse of ASCII_TO_EBCDIC_037. EBCDIC characters with no ASCII
 * equivalent map to 0x1A (ASCII SUB) — a visible placeholder rather than
 * silently dropping or corrupting the byte.
 */
const uint8_t EBCDIC_037_TO_ASCII[256] = {
    0x00,0x01,0x02,0x03,0x1A,0x09,0x1A,0x7F,0x1A,0x1A,0x1A,0x0B,0x0C,0x0D,0x0E,0x0F,
    0x10,0x11,0x12,0x13,0x1A,0x1A,0x08,0x1A,0x18,0x19,0x1A,0x1A,0x1C,0x1D,0x1E,0x1F,
    0x1A,0x1A,0x1A,0x1A,0x1A,0x0A,0x17,0x1B,0x1A,0x1A,0x1A,0x1A,0x1A,0x05,0x06,0x07,
    0x1A,0x1A,0x16,0x1A,0x1A,0x1A,0x1A,0x04,0x1A,0x1A,0x1A,0x1A,0x14,0x15,0x1A,0x1A,
    0x20,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x2E,0x3C,0x28,0x2B,0x7C,
    0x26,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x21,0x24,0x2A,0x29,0x3B,0xAC,
    0x2D,0x2F,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x2C,0x25,0x5F,0x3E,0x3F,
    0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x60,0x3A,0x23,0x40,0x27,0x3D,0x22,
    0x1A,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
    0x1A,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
    0x1A,0x7E,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
    0x5E,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x5B,0x5D,0x1A,0x1A,0x1A,0x1A,
    0x7B,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
    0x7D,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50,0x51,0x52,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
    0x5C,0x1A,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x1A,0x1A,0x1A,0x1A,0x1A,0xFF
};

/*
 * EBCDIC_1047_TO_ASCII
 *
 * Inverse of ASCII_TO_EBCDIC_1047. Used when reading VB files created on
 * z/OS UNIX System Services (OMVS). Unmapped characters produce 0x1A
 * (ASCII SUB) as a visible sentinel, same convention as the 037 table.
 */
const uint8_t EBCDIC_1047_TO_ASCII[256] = {
    0x00,0x01,0x02,0x03,0x1A,0x09,0x1A,0x7F,0x1A,0x1A,0x1A,0x0B,0x0C,0x0D,0x0E,0x0F,
    0x10,0x11,0x12,0x13,0x1A,0x0A,0x08,0x1A,0x18,0x19,0x1A,0x1A,0x1C,0x1D,0x1E,0x1F,
    0x1A,0x1A,0x1A,0x1A,0x1A,0x0A,0x17,0x1B,0x1A,0x1A,0x1A,0x1A,0x1A,0x05,0x06,0x07,
    0x1A,0x1A,0x16,0x1A,0x1A,0x1A,0x1A,0x04,0x1A,0x1A,0x1A,0x1A,0x14,0x15,0x1A,0x1A,
    0x20,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0xA2,0x2E,0x3C,0x28,0x2B,0x7C,
    0x26,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x21,0x24,0x2A,0x29,0x3B,0x5E,
    0x2D,0x2F,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0xA6,0x2C,0x25,0x5F,0x3E,0x3F,
    0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x60,0x3A,0x23,0x40,0x27,0x3D,0x22,
    0x1A,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
    0x1A,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
    0x1A,0x7E,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
    0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
    0x7B,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
    0x7D,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50,0x51,0x52,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
    0x5C,0x1A,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x1A,0x1A,0x1A,0x1A,0x1A,0xFF
};
