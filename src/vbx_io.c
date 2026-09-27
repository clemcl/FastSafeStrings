/**
 * @file vbx_io.c
 * @brief FastSafeStrings Variable Blocked (VB) File I/O Engine Implementation
 *
 * High-performance I/O engine for IBM Variable Blocked (VB) datasets.
 * Features Block Descriptor Word (BDW: 4 bytes) and Record Descriptor Word
 * (RDW: 4 bytes) handling, zero-copy locate-mode reading, and vectorized
 * code page translation (ASCII <-> EBCDIC 037/1047).
 *
 * @copyright 1988-2026 Clement Victor Clarke (FSS Project)
 */
/******************************************************************************
 * PROJECT:       FastSafeStrings (FSS) & VBIO
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
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#if defined(__has_include)
  #if __has_include("include/vbx_file.h")
    #include "include/vbx_file.h"
  #else
    #include "vbx_file.h"
  #endif
#else
  #include "vbx_file.h"
#endif

/* ------------------------------------------------------------------------- */
/* Vectorized Architecture Detection & Intrinsics                            */
/* ------------------------------------------------------------------------- */

#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(_M_ARM64)
  #include <arm_neon.h>
  #define VBX_HAVE_NEON 1
#elif defined(__AVX2__) || defined(__SSSE3__)
  #include <immintrin.h>
  #include <tmmintrin.h>
  #define VBX_HAVE_SSSE3 1
#endif

/* ------------------------------------------------------------------------- */
/* Code Page Translation Tables                                              */
/* ------------------------------------------------------------------------- */

/**
 * @brief Maps 7-bit ASCII (0x00-0x7F) to IBM Code Page 037 (EBCDIC).
 * Standard code page for North American z/OS mainframe datasets.
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
    /* 80-FF: Unmapped high-order byte padding */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

/**
 * @brief Maps 7-bit ASCII to IBM Code Page 1047 (EBCDIC Open Systems variant).
 * Standard for z/OS UNIX System Services (OMVS).
 */
const uint8_t ASCII_TO_EBCDIC_1047[256] = {
    /* 00-0F */ 0x00,0x01,0x02,0x03,0x37,0x2D,0x2E,0x2F,
                0x16,0x05,0x15,0x0B,0x0C,0x0D,0x0E,0x0F,
    /* 10-1F */ 0x10,0x11,0x12,0x13,0x3C,0x3D,0x32,0x26,
                0x18,0x19,0x3F,0x27,0x1C,0x1D,0x1E,0x1F,
    /* 20-2F */ 0x40,0x5A,0x7F,0x7B,0x5B,0x6C,0x50,0x7D,
                0x4D,0x5D,0x5C,0x4E,0x6B,0x60,0x4B,0x61,
    /* 30-3F */ 0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,
                0xF8,0xF9,0x7A,0x5E,0x4C,0x7E,0x6E,0x6F,
    /* 40-4F */ 0x7C,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,
                0xC8,0xC9,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,
    /* 50-5F */ 0xD7,0xD8,0xD9,0xE2,0xE3,0xE4,0xE5,0xE6,
                0xE7,0xE8,0xE9,0xAD,0xE0,0xBD,0x5F,0x6D,
    /* 60-6F */ 0x79,0x81,0x82,0x83,0x84,0x85,0x86,0x87,
                0x88,0x89,0x91,0x92,0x93,0x94,0x95,0x96,
    /* 70-7F */ 0x97,0x98,0x99,0xA2,0xA3,0xA4,0xA5,0xA6,
                0xA7,0xA8,0xA9,0xC0,0x4F,0xD0,0xA1,0x07,
    /* 80-FF */ 0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
                0x20,0x20,0x20,0x20,0x5F,0x20,0x20,0x20,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

/**
 * @brief Maps IBM Code Page 037 (EBCDIC) to ASCII.
 * Unmapped characters translate to 0x1A (ASCII SUB control character).
 */
const uint8_t EBCDIC_037_TO_ASCII[256] = {
    /* 00-0F */ 0x00,0x01,0x02,0x03,0x1A,0x09,0x1A,0x7F,
                0x1A,0x1A,0x1A,0x0B,0x0C,0x0D,0x0E,0x0F,
    /* 10-1F */ 0x10,0x11,0x12,0x13,0x1A,0x1A,0x08,0x1A,
                0x18,0x19,0x1A,0x1A,0x1C,0x1D,0x1E,0x1F,
    /* 20-2F */ 0x1A,0x1A,0x1A,0x1A,0x1A,0x0A,0x17,0x1B,
                0x1A,0x1A,0x1A,0x1A,0x1A,0x05,0x06,0x07,
    /* 30-3F */ 0x1A,0x1A,0x16,0x1A,0x1A,0x1A,0x1A,0x04,
                0x1A,0x1A,0x1A,0x1A,0x14,0x15,0x1A,0x1A,
    /* 40-4F */ 0x20,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
                0x1A,0x1A,0x1A,0x2E,0x3C,0x28,0x2B,0x7C,
    /* 50-5F */ 0x26,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
                0x1A,0x1A,0x21,0x24,0x2A,0x29,0x3B,0xAC,
    /* 60-6F */ 0x2D,0x2F,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
                0x1A,0x1A,0x1A,0x2C,0x25,0x5F,0x3E,0x3F,
    /* 70-7F */ 0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
                0x1A,0x60,0x3A,0x23,0x40,0x27,0x3D,0x22,
    /* 80-8F */ 0x1A,0x61,0x62,0x63,0x64,0x65,0x66,0x67,
                0x68,0x69,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
    /* 90-9F */ 0x1A,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,
                0x71,0x72,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
    /* A0-AF */ 0x1A,0x7E,0x73,0x74,0x75,0x76,0x77,0x78,
                0x79,0x7A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
    /* B0-BF */ 0x5E,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
                0x1A,0x1A,0x5B,0x5D,0x1A,0x1A,0x1A,0x1A,
    /* C0-CF */ 0x7B,0x41,0x42,0x43,0x44,0x45,0x46,0x47,
                0x48,0x49,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
    /* D0-DF */ 0x7D,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50,
                0x51,0x52,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
    /* E0-EF */ 0x5C,0x1A,0x53,0x54,0x55,0x56,0x57,0x58,
                0x59,0x5A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
    /* F0-FF */ 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
                0x38,0x39,0x1A,0x1A,0x1A,0x1A,0x1A,0xFF
};

/**
 * @brief Maps IBM Code Page 1047 (EBCDIC) to ASCII.
 * Unmapped characters translate to 0x1A (ASCII SUB control character).
 */
const uint8_t EBCDIC_1047_TO_ASCII[256] = {
    /* 00-0F */ 0x00,0x01,0x02,0x03,0x1A,0x09,0x1A,0x7F,
                0x1A,0x1A,0x1A,0x0B,0x0C,0x0D,0x0E,0x0F,
    /* 10-1F */ 0x10,0x11,0x12,0x13,0x1A,0x0A,0x08,0x1A,
                0x18,0x19,0x1A,0x1A,0x1C,0x1D,0x1E,0x1F,
    /* 20-2F */ 0x1A,0x1A,0x1A,0x1A,0x1A,0x0A,0x17,0x1B,
                0x1A,0x1A,0x1A,0x1A,0x1A,0x05,0x06,0x07,
    /* 30-3F */ 0x1A,0x1A,0x16,0x1A,0x1A,0x1A,0x1A,0x04,
                0x1A,0x1A,0x1A,0x1A,0x14,0x15,0x1A,0x1A,
    /* 40-4F */ 0x20,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
                0x1A,0x1A,0xA2,0x2E,0x3C,0x28,0x2B,0x7C,
    /* 50-5F */ 0x26,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
                0x1A,0x1A,0x21,0x24,0x2A,0x29,0x3B,0x5E,
    /* 60-6F */ 0x2D,0x2F,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
                0x1A,0x1A,0xA6,0x2C,0x25,0x5F,0x3E,0x3F,
    /* 70-7F */ 0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
                0x1A,0x60,0x3A,0x23,0x40,0x27,0x3D,0x22,
    /* 80-8F */ 0x1A,0x61,0x62,0x63,0x64,0x65,0x66,0x67,
                0x68,0x69,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
    /* 90-9F */ 0x1A,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,
                0x71,0x72,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
    /* A0-AF */ 0x1A,0x7E,0x73,0x74,0x75,0x76,0x77,0x78,
                0x79,0x7A,0x1A,0x1A,0x1A,0x5B,0x1A,0x1A,
    /* B0-BF */ 0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
                0x1A,0x1A,0x1A,0x1A,0x1A,0x5D,0x1A,0x1A,
    /* C0-CF */ 0x7B,0x41,0x42,0x43,0x44,0x45,0x46,0x47,
                0x48,0x49,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
    /* D0-DF */ 0x7D,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50,
                0x51,0x52,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
    /* E0-EF */ 0x5C,0x1A,0x53,0x54,0x55,0x56,0x57,0x58,
                0x59,0x5A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
    /* F0-FF */ 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
                0x38,0x39,0x1A,0x1A,0x1A,0x1A,0x1A,0xFF
};

/* ------------------------------------------------------------------------- */
/* Vectorized Translation Utilities                                          */
/* ------------------------------------------------------------------------- */

void vbx_translate_buffer(void *dest, const void *src, size_t len, const uint8_t *table) {
    if (!dest || !src || !table || len == 0) return;

    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    size_t i = 0;

    /* 16-way unrolled pipeline-parallel L1 cache lookup loop */
    for (; i + 16 <= len; i += 16) {
        uint8_t s0  = s[i +  0], s1  = s[i +  1], s2  = s[i +  2], s3  = s[i +  3];
        uint8_t s4  = s[i +  4], s5  = s[i +  5], s6  = s[i +  6], s7  = s[i +  7];
        uint8_t s8  = s[i +  8], s9  = s[i +  9], s10 = s[i + 10], s11 = s[i + 11];
        uint8_t s12 = s[i + 12], s13 = s[i + 13], s14 = s[i + 14], s15 = s[i + 15];

        d[i +  0] = table[s0];  d[i +  1] = table[s1];
        d[i +  2] = table[s2];  d[i +  3] = table[s3];
        d[i +  4] = table[s4];  d[i +  5] = table[s5];
        d[i +  6] = table[s6];  d[i +  7] = table[s7];
        d[i +  8] = table[s8];  d[i +  9] = table[s9];
        d[i + 10] = table[s10]; d[i + 11] = table[s11];
        d[i + 12] = table[s12]; d[i + 13] = table[s13];
        d[i + 14] = table[s14]; d[i + 15] = table[s15];
    }

    /* 4-way unrolled remainder */
    for (; i + 4 <= len; i += 4) {
        d[i + 0] = table[s[i + 0]];
        d[i + 1] = table[s[i + 1]];
        d[i + 2] = table[s[i + 2]];
        d[i + 3] = table[s[i + 3]];
    }

    /* Scalar tail loop */
    for (; i < len; i++) {
        d[i] = table[s[i]];
    }
}

void vbx_translate_ebcdic037_to_ascii(void *dest, const void *src, size_t len) {
    vbx_translate_buffer(dest, src, len, EBCDIC_037_TO_ASCII);
}

void vbx_translate_ascii_to_ebcdic037(void *dest, const void *src, size_t len) {
    vbx_translate_buffer(dest, src, len, ASCII_TO_EBCDIC_037);
}

void vbx_translate_ebcdic1047_to_ascii(void *dest, const void *src, size_t len) {
    vbx_translate_buffer(dest, src, len, EBCDIC_1047_TO_ASCII);
}

void vbx_translate_ascii_to_ebcdic1047(void *dest, const void *src, size_t len) {
    vbx_translate_buffer(dest, src, len, ASCII_TO_EBCDIC_1047);
}

/* ------------------------------------------------------------------------- */
/* Internal Block I/O Operations                                             */
/* ------------------------------------------------------------------------- */

/**
 * @brief Flush the current buffered block payload to disk with a 4-byte BDW.
 *
 * @param vb Active dataset handle.
 * @return 0 on success, -1 on I/O error.
 */
static int vb_flush(vb_handle_t *vb) {
    if (!vb || !vb->fp) return -1;
    if (vb->block_used == 0) return 0; /* Nothing buffered */

    uint32_t bdw_len = vb->block_used + VB_BDW_LEN;
    uint8_t bdw[4];
    bdw[0] = (uint8_t)((bdw_len >> 8) & 0xFF);
    bdw[1] = (uint8_t)(bdw_len & 0xFF);
    bdw[2] = 0;
    bdw[3] = 0;

    if (fwrite(bdw, 1, 4, vb->fp) != 4) {
        vb->error = 1;
        snprintf(vb->last_error, sizeof(vb->last_error),
                 "Failed to write BDW header (target %u bytes)", bdw_len);
        return -1;
    }

    if (fwrite(vb->block_buf, 1, vb->block_used, vb->fp) != (size_t)vb->block_used) {
        vb->error = 1;
        snprintf(vb->last_error, sizeof(vb->last_error),
                 "Failed to write block payload (%u bytes)", vb->block_used);
        return -1;
    }

    vb->stats.blocks_written++;
    vb->stats.bytes_written += bdw_len;
    vb->block_used = 0;
    return 0;
}

/**
 * @brief Read the next physical block from disk into the block buffer.
 *
 * @param vb Active dataset handle.
 * @return 1 on success, 0 on clean EOF, -1 on format or I/O error.
 */
static int vb_fill_block(vb_handle_t *vb) {
    if (!vb || !vb->fp) return -1;
    uint8_t bdw[4];

    size_t nread = fread(bdw, 1, 4, vb->fp);
    if (nread == 0) {
        vb->eof = 1;
        return 0; /* Clean EOF */
    }
    if (nread < 4) {
        vb->error = 1;
        snprintf(vb->last_error, sizeof(vb->last_error),
                 "Truncated BDW header: read %zu of 4 bytes", nread);
        return -1;
    }

    uint32_t block_len = ((uint32_t)bdw[0] << 8) | (uint32_t)bdw[1];
    if (block_len < VB_BDW_LEN) {
        vb->error = 1;
        snprintf(vb->last_error, sizeof(vb->last_error),
                 "Corrupted BDW: block length %u < 4", block_len);
        return -1;
    }

    uint32_t payload_len = block_len - VB_BDW_LEN;

    /* Dynamically expand buffer if block exceeds configured initial size */
    if (payload_len > vb->block_size) {
        if (payload_len > 16U * 1024U * 1024U) { /* 16MB sanity safety limit */
            vb->error = 1;
            snprintf(vb->last_error, sizeof(vb->last_error),
                     "Block payload %u bytes exceeds 16MB safety limit", payload_len);
            return -1;
        }
        uint8_t *new_buf = (uint8_t *)realloc(vb->block_buf, payload_len);
        if (!new_buf) {
            vb->error = 1;
            snprintf(vb->last_error, sizeof(vb->last_error),
                     "Memory reallocation failure for %u bytes", payload_len);
            return -1;
        }
        vb->block_buf = new_buf;
        vb->block_size = payload_len;
    }

    if (payload_len > 0) {
        size_t bytes_read = fread(vb->block_buf, 1, payload_len, vb->fp);
        if (bytes_read != (size_t)payload_len) {
            vb->error = 1;
            snprintf(vb->last_error, sizeof(vb->last_error),
                     "Truncated block payload (expected %u, got %zu)", payload_len, bytes_read);
            return -1;
        }
    }

    vb->block_used = payload_len;
    vb->block_pos = 0;
    vb->stats.blocks_read++;
    vb->stats.bytes_read += block_len;
    return 1;
}

/* ------------------------------------------------------------------------- */
/* Public API — Dataset Lifecycle                                            */
/* ------------------------------------------------------------------------- */

vb_handle_t *VB_Open(const char *path, const char *mode_str, uint32_t block_size) {
    if (!path || !mode_str) return NULL;

    vb_handle_t *vb = (vb_handle_t *)calloc(1, sizeof(vb_handle_t));
    if (!vb) return NULL;

    if (block_size < VB_MIN_BLOCK_SIZE) {
        block_size = VB_DEFAULT_BLOCK_SIZE;
    }

    if (mode_str[0] == 'w') {
        vb->mode = VB_MODE_WRITE;
        vb->fp = fopen(path, "wb");
    } else if (mode_str[0] == 'a') {
        vb->mode = VB_MODE_APPEND;
        vb->fp = fopen(path, "a+b");
        if (vb->fp) {
            fseek(vb->fp, 0, SEEK_END);
        }
    } else {
        vb->mode = VB_MODE_READ;
        vb->fp = fopen(path, "rb");
    }

    if (!vb->fp) {
        snprintf(vb->last_error, sizeof(vb->last_error), "Failed to open file '%s'", path);
        free(vb);
        MVS_ABEND("S013", "File open failed");
        return NULL;
    }

    /* Configure 64KB stdio buffer for high file-system throughput */
    (void)setvbuf(vb->fp, NULL, _IOFBF, 65536);

    /* Parse translation code page */
    if (strstr(mode_str, "codeset=037") || (mode_str[1] != '\0' && strstr(mode_str, "037"))) {
        if (vb->mode == VB_MODE_WRITE || vb->mode == VB_MODE_APPEND) {
            vb->trans_table = ASCII_TO_EBCDIC_037;
            vb->trans_mode = VB_TRANS_ASCII_EBCDIC037;
        } else {
            vb->trans_table = EBCDIC_037_TO_ASCII;
            vb->trans_mode = VB_TRANS_EBCDIC037_ASCII;
        }
    } else if (strstr(mode_str, "codeset=1047") || (mode_str[1] != '\0' && strstr(mode_str, "1047"))) {
        if (vb->mode == VB_MODE_WRITE || vb->mode == VB_MODE_APPEND) {
            vb->trans_table = ASCII_TO_EBCDIC_1047;
            vb->trans_mode = VB_TRANS_ASCII_EBCDIC1047;
        } else {
            vb->trans_table = EBCDIC_1047_TO_ASCII;
            vb->trans_mode = VB_TRANS_EBCDIC1047_ASCII;
        }
    } else {
        vb->trans_table = NULL;
        vb->trans_mode = VB_TRANS_NONE;
    }

    vb->block_size = block_size;
    vb->block_buf = (uint8_t *)malloc(block_size);
    if (!vb->block_buf) {
        fclose(vb->fp);
        free(vb);
        return NULL;
    }

    return vb;
}

vb_handle_t *VB_OpenRead(const char *path, const char *translate) {
    char openmode[32] = "r";
    if (translate && translate[0] != '\0') {
        if (strcmp(translate, "037") == 0) {
            strcpy(openmode, "r,codeset=037");
        } else if (strcmp(translate, "1047") == 0) {
            strcpy(openmode, "r,codeset=1047");
        } else {
            MVS_ABEND("S013", "Invalid translate table: must be \"\", \"037\", or \"1047\"");
            return NULL;
        }
    }
    return VB_Open(path, openmode, VB_DEFAULT_BLOCK_SIZE);
}

vb_handle_t *VB_OpenWrite(const char *path, uint32_t block_size, const char *translate) {
    char openmode[32] = "w";
    if (translate && translate[0] != '\0') {
        if (strcmp(translate, "037") == 0) {
            strcpy(openmode, "w,codeset=037");
        } else if (strcmp(translate, "1047") == 0) {
            strcpy(openmode, "w,codeset=1047");
        } else {
            MVS_ABEND("S013", "Invalid translate table: must be \"\", \"037\", or \"1047\"");
            return NULL;
        }
    }
    return VB_Open(path, openmode, block_size ? block_size : VB_DEFAULT_BLOCK_SIZE);
}

vb_handle_t *VB_OpenAppend(const char *path, uint32_t block_size, const char *translate) {
    char openmode[32] = "a";
    if (translate && translate[0] != '\0') {
        if (strcmp(translate, "037") == 0) {
            strcpy(openmode, "a,codeset=037");
        } else if (strcmp(translate, "1047") == 0) {
            strcpy(openmode, "a,codeset=1047");
        } else {
            MVS_ABEND("S013", "Invalid translate table: must be \"\", \"037\", or \"1047\"");
            return NULL;
        }
    }
    return VB_Open(path, openmode, block_size ? block_size : VB_DEFAULT_BLOCK_SIZE);
}

vb_handle_t *VB_OpenEx(const char *path, vb_mode_t mode, uint32_t block_size, vb_trans_t trans) {
    const char *mode_str = "r";
    if (mode == VB_MODE_WRITE) {
        if (trans == VB_TRANS_ASCII_EBCDIC037) mode_str = "w,codeset=037";
        else if (trans == VB_TRANS_ASCII_EBCDIC1047) mode_str = "w,codeset=1047";
        else mode_str = "w";
    } else if (mode == VB_MODE_APPEND) {
        if (trans == VB_TRANS_ASCII_EBCDIC037) mode_str = "a,codeset=037";
        else if (trans == VB_TRANS_ASCII_EBCDIC1047) mode_str = "a,codeset=1047";
        else mode_str = "a";
    } else {
        if (trans == VB_TRANS_EBCDIC037_ASCII) mode_str = "r,codeset=037";
        else if (trans == VB_TRANS_EBCDIC1047_ASCII) mode_str = "r,codeset=1047";
        else mode_str = "r";
    }
    return VB_Open(path, mode_str, block_size);
}

void VB_Close(vb_handle_t *vb) {
    if (!vb) return;

    if (vb->fp) {
        if (vb->mode == VB_MODE_WRITE || vb->mode == VB_MODE_APPEND) {
            if (vb_flush(vb) < 0) {
                fprintf(stderr, "VB_Close: final buffer flush failed\n");
            }
        }
        fclose(vb->fp);
        vb->fp = NULL;
    }

    if (vb->block_buf) {
        free(vb->block_buf);
        vb->block_buf = NULL;
    }

    free(vb);
}

/* ------------------------------------------------------------------------- */
/* Public API — Record I/O Operations                                        */
/* ------------------------------------------------------------------------- */

int VB_Put(vb_handle_t *vb, const void *data, uint32_t len) {
    if (!vb || (!data && len > 0)) return -1;

    uint32_t rdw_len = len + VB_RDW_LEN;

    /* Handle records larger than initial block size */
    if (rdw_len > vb->block_size) {
        if (rdw_len > 16U * 1024U * 1024U) {
            MVS_ABEND("S013", "Record exceeds 16MB maximum block limit");
            return -1;
        }
        if (vb->block_used > 0) {
            if (vb_flush(vb) < 0) return -1;
        }
        uint8_t *new_buf = (uint8_t *)realloc(vb->block_buf, rdw_len + 4096U);
        if (!new_buf) {
            MVS_ABEND("S013", "Out of memory expanding block buffer");
            return -1;
        }
        vb->block_buf = new_buf;
        vb->block_size = rdw_len + 4096U;
    } else if (vb->block_used + rdw_len > vb->block_size) {
        if (vb_flush(vb) < 0) return -1;
    }

    uint8_t *p = vb->block_buf + vb->block_used;

    /* 4-byte RDW (big-endian length) */
    p[0] = (uint8_t)((rdw_len >> 8) & 0xFF);
    p[1] = (uint8_t)(rdw_len & 0xFF);
    p[2] = 0;
    p[3] = 0;

    uint8_t *dest = p + VB_RDW_LEN;
    if (vb->trans_table) {
        vbx_translate_buffer(dest, data, len, vb->trans_table);
    } else if (len > 0) {
        memcpy(dest, data, len);
    }

    vb->block_used += rdw_len;
    vb->stats.records_written++;
    return 1;
}

int VB_GetLocate(vb_handle_t *vb, const char **ptr, uint32_t *len) {
    if (!vb || !ptr || !len) return -1;

    if (vb->block_pos + VB_RDW_LEN > vb->block_used) {
        int rc = vb_fill_block(vb);
        if (rc <= 0) return rc; /* 0 for EOF, -1 for error */
    }

    uint8_t *p = vb->block_buf + vb->block_pos;
    uint32_t rdw_len = ((uint32_t)p[0] << 8) | (uint32_t)p[1];

    if (rdw_len < VB_RDW_LEN || (vb->block_pos + rdw_len > vb->block_used)) {
        vb->error = 1;
        snprintf(vb->last_error, sizeof(vb->last_error),
                 "RDW corruption: rdw_len=%u, pos=%u, used=%u",
                 rdw_len, vb->block_pos, vb->block_used);
        MVS_ABEND("S0C4", "Record Descriptor Word (RDW) corruption detected");
        return -1;
    }

    *len = rdw_len - VB_RDW_LEN;
    *ptr = (const char *)(p + VB_RDW_LEN);

    vb->block_pos += rdw_len;
    vb->stats.records_read++;
    return 1;
}

int VB_Get(vb_handle_t *vb, void *buf, uint32_t max_len, uint32_t *out_len) {
    if (!vb || (!buf && max_len > 0)) return -1;

    const char *src_ptr = NULL;
    uint32_t rlen = 0;

    int rc = VB_GetLocate(vb, &src_ptr, &rlen);
    if (rc <= 0) return rc;

    if (rlen > max_len) {
        vb->error = 1;
        snprintf(vb->last_error, sizeof(vb->last_error),
                 "Record length (%u) exceeds destination buffer (%u)", rlen, max_len);
        return -1;
    }

    if (vb->trans_table) {
        vbx_translate_buffer(buf, src_ptr, rlen, vb->trans_table);
    } else if (rlen > 0) {
        memcpy(buf, src_ptr, rlen);
    }

    if (rlen < max_len) {
        ((char *)buf)[rlen] = '\0';
    }

    if (out_len) {
        *out_len = rlen;
    }

    return 1;
}

int VB_Skip(vb_handle_t *vb, uint32_t *skipped_len) {
    if (!vb) return -1;

    if (vb->block_pos + VB_RDW_LEN > vb->block_used) {
        int rc = vb_fill_block(vb);
        if (rc <= 0) return rc;
    }

    uint8_t *p = vb->block_buf + vb->block_pos;
    uint32_t rdw_len = ((uint32_t)p[0] << 8) | (uint32_t)p[1];

    if (rdw_len < VB_RDW_LEN || (vb->block_pos + rdw_len > vb->block_used)) {
        vb->error = 1;
        snprintf(vb->last_error, sizeof(vb->last_error),
                 "RDW corruption during skip: rdw_len=%u, pos=%u, used=%u",
                 rdw_len, vb->block_pos, vb->block_used);
        return -1;
    }

    vb->block_pos += rdw_len;
    vb->stats.records_read++;

    if (skipped_len) {
        *skipped_len = rdw_len - VB_RDW_LEN;
    }

    return 1;
}

int VB_Flush(vb_handle_t *vb) {
    if (!vb) return -1;
    if (vb->mode == VB_MODE_WRITE || vb->mode == VB_MODE_APPEND) {
        return vb_flush(vb);
    }
    return 0;
}

int VB_Prefetch(vb_handle_t *vb) {
    if (!vb || !vb->fp) return -1;
    return 0;
}

int VB_GetStats(const vb_handle_t *vb, vb_stats_t *stats) {
    if (!vb || !stats) return -1;
    *stats = vb->stats;
    return 0;
}

const char *VB_Error(const vb_handle_t *vb) {
    if (!vb) return "NULL handle";
    return vb->last_error[0] ? vb->last_error : "No error";
}
