#ifndef VBX_FILE_H
#define VBX_FILE_H

/******************************************************************************
 * PROJECT:      FastSafeStrings (FSS) & VBIO
 *
 * AUTHOR:       Clement Victor Clarke (Warracknabeal, Australia)
 * COPYRIGHT:    Copyright (c) 1988-2026 Clement Victor Clarke (Originator of Jol)
 * All Rights Reserved.
 *
 * LICENSE TERMS:
 *   1. INDIVIDUAL/NON-PROFIT: Use is free under the MIT License.
 *   2. COMMERCIAL: Use by entities with annual revenue > $1M AUD requires
 *      a paid Commercial License.
 *
 * MISSION:      To reduce global energy consumption through computational
 *               efficiency.
 *
 * CONTACT:      clemclarke@gmail.com for commercial terms and
 *               "Shared Savings" agreements.
 ******************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>   /* for exit() used in MVS_ABEND */

typedef enum {
    VB_MODE_READ  = 1,
    VB_MODE_WRITE = 2
} vb_mode_t;

/*
 * vb_handle_t — internal state for an open VB file.
 *
 * fp         : standard stdio FILE* handle, opened via fopen() in binary
 *              mode ("rb"/"wb"). All reads/writes go through fread()/
 *              fwrite() — plain stdio, no platform-specific headers needed.
 * mode       : VB_MODE_READ or VB_MODE_WRITE
 * block_size : maximum block payload size (bytes, excluding the 4-byte BDW)
 * block_used : bytes currently occupied in block_buf (write side), or bytes
 *              available to read from in block_buf (read side)
 * block_pos  : current read position within block_buf (read side only)
 * block_buf  : heap-allocated block I/O buffer, sized to block_size
 * trans_table: pointer to a 256-byte codepage translation table (see
 *              ASCII_TO_EBCDIC_037 etc. below), or NULL for no translation
 * eof        : set to 1 at end of file
 * error      : set to 1 on I/O error
 */
typedef struct vb_handle {
    FILE          *fp;
    vb_mode_t      mode;
    uint32_t       block_size;
    uint32_t       block_used;
    uint32_t       block_pos;
    uint8_t       *block_buf;
    const uint8_t *trans_table;
    int            eof;
    int            error;
} vb_handle_t;

/* -------------------------------------------------- */
/* Code page translation tables (defined in vbx_io.c) */
/* -------------------------------------------------- */

/*
 * Four translation tables, covering the two EBCDIC code pages this library
 * supports:
 *   ASCII_TO_EBCDIC_037   — IBM Code Page 037  (North America, most common)
 *   EBCDIC_037_TO_ASCII   — inverse of above
 *   ASCII_TO_EBCDIC_1047  — IBM Code Page 1047 (Open Systems / Unix on z/OS)
 *   EBCDIC_1047_TO_ASCII  — inverse of above
 */
extern const uint8_t ASCII_TO_EBCDIC_037[256];
extern const uint8_t EBCDIC_037_TO_ASCII[256];
extern const uint8_t ASCII_TO_EBCDIC_1047[256];
extern const uint8_t EBCDIC_1047_TO_ASCII[256];

/* The PL/I logical NOT character. Maps to 0x5F in EBCDIC 1047, 0xAC in 037. */
#define PLI_NOT_SYMBOL 0x5F

/* -------------------------------------------------- */
/* API                                                */
/* -------------------------------------------------- */

/*
 * VB_Open — low-level open. Prefer VB_OpenRead / VB_OpenWrite below.
 *   mode_str: "r" or "w", optionally with ",codeset=037" or ",codeset=1047"
 */
vb_handle_t *VB_Open(const char *path, const char *mode_str,
                     uint32_t block_size);

/* VB_OpenRead  — open for reading. translate: "" (none), "037", or "1047" */
vb_handle_t *VB_OpenRead(const char *path, const char *translate);

/* VB_OpenWrite — open for writing. translate: "" (none), "037", or "1047" */
vb_handle_t *VB_OpenWrite(const char *path, uint32_t block_size,
                           const char *translate);

/* VB_Put — write one record. Returns 1 on success, -1 on error. */
int VB_Put(vb_handle_t *vb, const void *data, uint32_t len);

/*
 * VB_Get — read one record into caller's buffer.
 * Returns 1 on success, 0 at EOF, -1 if record exceeds max_len or on error.
 */
int VB_Get(vb_handle_t *vb, void *buf, uint32_t max_len, uint32_t *out_len);

/*
 * VB_GetLocate — zero-copy read. Returns a pointer directly into the
 * internal block buffer (valid until the next VB call on this handle).
 */
int VB_GetLocate(vb_handle_t *vb, const char **ptr, uint32_t *len);

/* VB_Skip — advance past the current record without copying its data. */
int VB_Skip(vb_handle_t *vb, uint32_t *skipped_len);

/* VB_Close — flush (if writing), close the file, free all resources. */
void VB_Close(vb_handle_t *vb);

/* -------------------------------------------------- */
/* Error / abort macro                                */
/* -------------------------------------------------- */

/*
 * MVS_ABEND — print a structured error message and terminate.
 * Named after the IBM z/OS ABEND (abnormal end) convention. Used for
 * unrecoverable conditions (RDW corruption, record too large for the
 * block buffer) where continuing would risk silently corrupting data.
 */
#define MVS_ABEND(code, msg) do { \
    fprintf(stderr, "ABEND %s: %s at line %d\n", code, msg, __LINE__); \
    exit(1); \
} while(0)

#endif /* VBX_FILE_H */
