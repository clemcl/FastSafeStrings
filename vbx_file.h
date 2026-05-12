#ifndef VBX_FILE_H
#define VBX_FILE_H

#include <stdint.h>
#include <stdio.h>


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
 
typedef enum {
    VB_MODE_READ  = 1,
    VB_MODE_WRITE = 2
} vb_mode_t;

typedef struct vb_handle {
 //   FILE        *fp;
    int          fp;
    vb_mode_t    mode;
    uint32_t     block_size;
    uint32_t     block_used;
    uint32_t     block_pos;
    uint8_t     *block_buf;
    const uint8_t *trans_table; // Pointer to ASCII_TO_EBCDIC_037 or NULL
    int          eof;
    int          error;
} vb_handle_t;

/* vbx_file.h additions */
extern const uint8_t ASCII_TO_EBCDIC_037[256];
extern const uint8_t EBCDIC_037_TO_ASCII[256];
extern const uint8_t ASCII_TO_EBCDIC_1047[256];
extern const uint8_t EBCDIC_1047_TO_ASCII[256];

/* The PL/I NOT symbol often maps to 0x5F or 0xAC depending on the terminal */
#define PLI_NOT_SYMBOL 0x5F
/* API */
vb_handle_t *VB_Open(const char *path, const char *mode_str, uint32_t block_size) ;
vb_handle_t *VB_OpenRead(const char *path,  const char *translate);
vb_handle_t *VB_OpenWrite(const char *path, uint32_t block_size,  const char *translate);
int          VB_Put(vb_handle_t *vb, const void *data, uint32_t len);
int          VB_Get(vb_handle_t *vb, void *buf, uint32_t max_len, uint32_t *out_len);
int          VB_GetLocate(vb_handle_t *vb, const char **ptr, uint32_t *len);
void         VB_Close(vb_handle_t *vb);

#define MVS_ABEND(code, msg) do { \
    fprintf(stderr, "ABEND %s: %s at line %d\n", code, msg, __LINE__); \
    exit(1); \
} while(0)
 
#endif
