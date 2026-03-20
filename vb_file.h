#ifndef VB_HANDLE_H
#define VB_HANDLE_H
 
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VB_FILE_MAGIC 0x31464256u  /* "VBF1" */
#define VB_FILE_VERSION 1  

typedef enum {
    VB_MODE_READ  = 1,
    VB_MODE_WRITE = 2
} vb_mode_t;

typedef enum {
    VB_LEN16 = 2,   /* classic VB */
    VB_LEN32 = 4    /* VB4 */
} vb_lenfmt_t;


typedef struct vb_file_header {
    uint32_t magic;        /* "VBF1" */
    uint16_t version;      /* = 1 */
    uint16_t header_size;  /* sizeof(vb_file_header) */

    uint32_t block_size;   /* 0 = unblocked, >0 = VBB */

    uint16_t lenfmt;       /* 2 = VB, 4 = VB4 */
    uint16_t flags;        /* reserved (must be 0 in V1) */

    uint32_t reserved1;    /* must be 0 */
    uint32_t reserved2;    /* must be 0 */
} vb_file_header_t;
 
/*
 * VB file handle (V1)
 * Uses ISO C stdio for maximum portability
 */
typedef struct vb_handle {
    /* I/O */
    FILE        *fp;
    vb_mode_t    mode;

    /* Record format */
    vb_lenfmt_t  lenfmt;

    /* Blocking */
    uint32_t     block_size;    /* 0 = unblocked */
    uint32_t     block_used;    /* bytes used (write) */
    uint32_t     block_pos;     /* cursor (read) */

    uint8_t     *block_buf;

    /* State */
    int          eof;
    int          error;
} vb_handle_t;

/* API */
vb_handle_t *VB_OpenWrite(
    const char  *path,
    uint32_t     block_size,
    vb_lenfmt_t  lenfmt
);

vb_handle_t *VB_OpenRead(
    const char *path
);

int VB_Put(
    vb_handle_t *vb,
    const void  *data,
    uint32_t     len
);

int VB_Get(
    vb_handle_t *vb,
    void        *buf,
    uint32_t     max_len,
    uint32_t    *out_len
);

int VB_Close(vb_handle_t *vb);

#ifdef __cplusplus
}
#endif

#endif /* VB_HANDLE_H */
