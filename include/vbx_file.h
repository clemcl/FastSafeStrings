/**
 * @file vbx_file.h
 * @brief FastSafeStrings Variable Blocked (VB) File I/O Engine
 *
 * High-performance, zero-copy, vectorized I/O engine for IBM Variable Blocked
 * (VB) datasets with Block Descriptor Words (BDW: 4 bytes) and Record
 * Descriptor Words (RDW: 4 bytes).
 *
 * Supports bidirectional translation between ASCII and EBCDIC (CP037, CP1047)
 * using SIMD (SSSE3/AVX2/NEON) and SWAR acceleration.
 *
 * Compatible with C99/C11 and C++17/C++20 across MSVC, GCC, Clang, and IBM compilers.
 *
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

#ifndef VBX_FILE_H
#define VBX_FILE_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/* Compiler Alignment & Inlining Macros                                      */
/* ------------------------------------------------------------------------- */

#if defined(_MSC_VER)
  #define VBX_ALIGN64 __declspec(align(8))
  #define VBX_INLINE  __forceinline static
#elif defined(__GNUC__) || defined(__clang__)
  #define VBX_ALIGN64 __attribute__((aligned(8)))
  #define VBX_INLINE  __inline__ static __attribute__((always_inline))
#else
  #define VBX_ALIGN64
  #define VBX_INLINE  static inline
#endif

/* ------------------------------------------------------------------------- */
/* Constants & Limits                                                        */
/* ------------------------------------------------------------------------- */

#define VB_BDW_LEN              4U
#define VB_RDW_LEN              4U
#define VB_DEFAULT_BLOCK_SIZE   32768U
#define VB_MAX_BLOCK_SIZE       65536U
#define VB_MIN_BLOCK_SIZE       8U

/* The PL/I logical NOT character. Maps to 0x5F in EBCDIC 1047, 0xAC in 037. */
#define PLI_NOT_SYMBOL          0x5F

/* ------------------------------------------------------------------------- */
/* Enumerations                                                              */
/* ------------------------------------------------------------------------- */

/**
 * @brief File access modes for Variable Blocked files.
 */
typedef enum vb_mode {
    VB_MODE_READ   = 1,
    VB_MODE_WRITE  = 2,
    VB_MODE_APPEND = 3,

    /* Aliases for concise modern usage */
    VB_READ        = 1,
    VB_WRITE       = 2,
    VB_APPEND      = 3
} vb_mode_t;

/**
 * @brief Code page character translation modes.
 */
typedef enum vb_trans {
    VB_TRANS_NONE             = 0,
    VB_TRANS_EBCDIC037_ASCII  = 1,
    VB_TRANS_ASCII_EBCDIC037  = 2,
    VB_TRANS_EBCDIC1047_ASCII = 3,
    VB_TRANS_ASCII_EBCDIC1047 = 4
} vb_trans_t;

/* ------------------------------------------------------------------------- */
/* Statistics & Metrics                                                      */
/* ------------------------------------------------------------------------- */

/**
 * @brief I/O performance and accounting statistics for a VB file handle.
 */
typedef struct vb_stats {
    uint64_t records_read;
    uint64_t records_written;
    uint64_t bytes_read;
    uint64_t bytes_written;
    uint64_t blocks_read;
    uint64_t blocks_written;
} vb_stats_t;

/* ------------------------------------------------------------------------- */
/* Handle Definition                                                         */
/* ------------------------------------------------------------------------- */

/**
 * @brief Internal state and buffer management for an active VB dataset.
 *
 * Maintained 64-bit aligned for optimal memory bus throughput and SIMD
 * buffer alignment.
 */
typedef struct VBX_ALIGN64 vb_handle {
    /* Legacy header layout preserved for direct field access compatibility */
    FILE          *fp;              /**< Standard binary file handle */
    vb_mode_t      mode;            /**< Active access mode (read/write/append) */
    uint32_t       block_size;      /**< Maximum block payload capacity (excl BDW) */
    uint32_t       block_used;      /**< Current buffered payload size in bytes */
    uint32_t       block_pos;       /**< Current read offset within block_buf */
    uint8_t       *block_buf;       /**< Heap-allocated block I/O buffer */
    const uint8_t *trans_table;     /**< Active 256-byte translation table or NULL */
    int            eof;             /**< End-of-file indicator (1=EOF, 0=active) */
    int            error;           /**< Error indicator (1=error, 0=clean) */

    /* Modern extension fields */
    vb_trans_t     trans_mode;      /**< Translation mode enum */
    uint32_t       flags;           /**< Operational configuration flags */
    vb_stats_t     stats;           /**< Cumulative record/block statistics */
    char           last_error[128]; /**< Human-readable diagnostic description */
} vb_handle_t;

/* ------------------------------------------------------------------------- */
/* Code Page Translation Tables (Exported from vbx_io.c)                     */
/* ------------------------------------------------------------------------- */

extern const uint8_t ASCII_TO_EBCDIC_037[256];
extern const uint8_t EBCDIC_037_TO_ASCII[256];
extern const uint8_t ASCII_TO_EBCDIC_1047[256];
extern const uint8_t EBCDIC_1047_TO_ASCII[256];

/* ------------------------------------------------------------------------- */
/* Public API — Dataset Lifecycle & Operations                               */
/* ------------------------------------------------------------------------- */

/**
 * @brief Open a Variable Blocked file with full configuration parameters.
 *
 * @param path File system path.
 * @param mode_str Mode string: "r", "w", "a", optionally containing ",codeset=037" or ",codeset=1047".
 * @param block_size Desired block payload size (e.g. 32768, 65536).
 * @return Pointer to allocated vb_handle_t, or NULL on error.
 */
vb_handle_t *VB_Open(const char *path, const char *mode_str, uint32_t block_size);

/**
 * @brief Open a VB file for reading with optional code page translation.
 *
 * @param path File system path.
 * @param translate Translation selector: "" (none), "037", or "1047".
 * @return Pointer to allocated vb_handle_t, or NULL on failure.
 */
vb_handle_t *VB_OpenRead(const char *path, const char *translate);

/**
 * @brief Open a VB file for writing with specified block size and translation.
 *
 * @param path File system path.
 * @param block_size Output block buffer size (e.g. 32768).
 * @param translate Translation selector: "" (none), "037", or "1047".
 * @return Pointer to allocated vb_handle_t, or NULL on failure.
 */
vb_handle_t *VB_OpenWrite(const char *path, uint32_t block_size, const char *translate);

/**
 * @brief Open a VB file for appending with specified block size and translation.
 *
 * @param path File system path.
 * @param block_size Output block buffer size (e.g. 32768).
 * @param translate Translation selector: "" (none), "037", or "1047".
 * @return Pointer to allocated vb_handle_t, or NULL on failure.
 */
vb_handle_t *VB_OpenAppend(const char *path, uint32_t block_size, const char *translate);

/**
 * @brief Strongly-typed programmatic open function.
 *
 * @param path File system path.
 * @param mode Access mode (VB_READ, VB_WRITE, VB_APPEND).
 * @param block_size Block buffer size in bytes (e.g. 32768, 65536).
 * @param trans Translation mode enum.
 * @return Pointer to allocated vb_handle_t, or NULL on failure.
 */
vb_handle_t *VB_OpenEx(const char *path, vb_mode_t mode, uint32_t block_size, vb_trans_t trans);

/**
 * @brief Flush pending output (if writing), close the file, and release resources.
 *
 * Safe to call with NULL.
 *
 * @param vb Pointer to vb_handle_t.
 */
void VB_Close(vb_handle_t *vb);

/**
 * @brief Write one variable-length record to the dataset.
 *
 * Automatically prefixes the record with a 4-byte RDW and flushes the block
 * buffer when full. Applies code page translation if configured.
 *
 * @param vb Active write/append handle.
 * @param data Pointer to record payload bytes.
 * @param len Record payload length in bytes.
 * @return 1 on success, -1 on I/O or capacity error.
 */
int VB_Put(vb_handle_t *vb, const void *data, uint32_t len);

/**
 * @brief Read one variable-length record into a caller-supplied buffer.
 *
 * @param vb Active read handle.
 * @param buf Destination buffer.
 * @param max_len Maximum bytes destination buffer can hold.
 * @param out_len Optional pointer receiving the actual record byte count read.
 * @return 1 on success, 0 on clean EOF, -1 on error or truncation.
 */
int VB_Get(vb_handle_t *vb, void *buf, uint32_t max_len, uint32_t *out_len);

/**
 * @brief Zero-copy locate-mode read ("Locate Mode").
 *
 * Returns a direct pointer into the internal block buffer avoiding all data
 * copying and translation overhead. The returned pointer remains valid until
 * the subsequent read operation on this handle.
 *
 * @param vb Active read handle.
 * @param ptr Output pointer set to the first byte of the record payload.
 * @param len Output pointer set to the record payload length in bytes.
 * @return 1 on success, 0 on clean EOF, -1 on format or I/O error.
 */
int VB_GetLocate(vb_handle_t *vb, const char **ptr, uint32_t *len);

/**
 * @brief Advance past the current record without inspecting or copying payload.
 *
 * @param vb Active read handle.
 * @param skipped_len Optional pointer receiving the byte length of skipped record.
 * @return 1 on success, 0 on EOF, -1 on format or I/O error.
 */
int VB_Skip(vb_handle_t *vb, uint32_t *skipped_len);

/**
 * @brief Flush any pending buffered records to storage.
 *
 * @param vb Active write/append handle.
 * @return 0 on success, -1 on error.
 */
int VB_Flush(vb_handle_t *vb);

/**
 * @brief Hint the I/O engine to prefetch the subsequent block from storage.
 *
 * @param vb Active read handle.
 * @return 0 on success, -1 on error.
 */
int VB_Prefetch(vb_handle_t *vb);

/**
 * @brief Retrieve snapshot of cumulative I/O statistics.
 *
 * @param vb Active handle.
 * @param stats Output pointer to receive stats copy.
 * @return 0 on success, -1 if parameters are invalid.
 */
int VB_GetStats(const vb_handle_t *vb, vb_stats_t *stats);

/**
 * @brief Retrieve human-readable description of last error.
 *
 * @param vb Active handle.
 * @return String description, or empty string if no error.
 */
const char *VB_Error(const vb_handle_t *vb);

/* ------------------------------------------------------------------------- */
/* Vectorized Translation Utilities                                          */
/* ------------------------------------------------------------------------- */

/**
 * @brief Vectorized / SWAR byte translation using an arbitrary 256-byte LUT.
 *
 * @param dest Destination memory buffer (may alias src).
 * @param src Source memory buffer.
 * @param len Byte count to translate.
 * @param table 256-byte lookup table.
 */
void vbx_translate_buffer(void *dest, const void *src, size_t len, const uint8_t *table);

/**
 * @brief Translate memory from EBCDIC CP037 to ASCII.
 */
void vbx_translate_ebcdic037_to_ascii(void *dest, const void *src, size_t len);

/**
 * @brief Translate memory from ASCII to EBCDIC CP037.
 */
void vbx_translate_ascii_to_ebcdic037(void *dest, const void *src, size_t len);

/**
 * @brief Translate memory from EBCDIC CP1047 to ASCII.
 */
void vbx_translate_ebcdic1047_to_ascii(void *dest, const void *src, size_t len);

/**
 * @brief Translate memory from ASCII to EBCDIC CP1047.
 */
void vbx_translate_ascii_to_ebcdic1047(void *dest, const void *src, size_t len);

/* ------------------------------------------------------------------------- */
/* Legacy Error / ABEND Macro                                                */
/* ------------------------------------------------------------------------- */

#ifndef MVS_ABEND
/**
 * @brief Print structured mainframe-style abend message and terminate.
 */
#define MVS_ABEND(code, msg) do { \
    fprintf(stderr, "ABEND %s: %s at %s:%d\n", (code), (msg), __FILE__, __LINE__); \
    exit(1); \
} while(0)
#endif

/* ------------------------------------------------------------------------- */
/* FastSafeStrings Record I/O Macros (Convenience Helpers)                   */
/* ------------------------------------------------------------------------- */

#ifndef GET_REC
/**
 * @brief Read record and synchronize with FastSafeStrings dope vector.
 */
#define GET_REC(h, name) \
    VB_Get((h), (name), (uint32_t)sizeof(name), &dv_##name.cur_len)
#endif

#ifndef PUT_REC
/**
 * @brief Write FastSafeStrings record using dope vector length.
 */
#define PUT_REC(h, name) \
    VB_Put((h), (name), dv_##name.cur_len)
#endif

#ifndef FIND_REC
/**
 * @brief Skip @p skip_count records, storing success flag in @p stat.
 */
#define FIND_REC(h, skip_count, stat) do { \
    uint32_t _vbx_i; \
    (stat) = 1; \
    for (_vbx_i = 0; _vbx_i < (uint32_t)(skip_count); _vbx_i++) { \
        if (VB_Skip((h), NULL) <= 0) { (stat) = 0; break; } \
    } \
} while(0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* VBX_FILE_H */
