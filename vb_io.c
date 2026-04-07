#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "vb_file.h"
#include "faststr.h"

/* -------------------------------------------------- */
/* 1. Internal Helper Functions (Must be at the top)  */
/* -------------------------------------------------- */

static int vb_flush(vb_handle_t *vb) {
    if (vb->block_used == 0) return 0;
    fwrite(&vb->block_used, 4, 1, vb->fp);
    if (fwrite(vb->block_buf, 1, vb->block_used, vb->fp) != vb->block_used) {
        vb->error = 1;
        return -1;
    }
    vb->block_used = 0;
    return 0;
}

static int vb_fill_block(vb_handle_t *vb) {
    if (vb->mode != VB_MODE_READ || !vb->block_size) return 0;

    uint32_t next_block_len = 0;
    
    /* 1. Read the 4-byte prefix that tells us how big THIS block is */
    if (fread(&next_block_len, 4, 1, vb->fp) != 1) {
        return 0; /* Normal EOF */
    }

    /* 2. Safety check: ensure it doesn't exceed our allocated buffer */
    if (next_block_len > vb->block_size) return -1;

    /* 3. Read exactly that many bytes into the buffer */
    size_t n = fread(vb->block_buf, 1, next_block_len, vb->fp);
    if (n != next_block_len) return -1; /* Corrupt file or truncated block */

    vb->block_used = (uint32_t)n;
    vb->block_pos = 0;
    return 1;
}
 

/* -------------------------------------------------- */
/* Length field helpers (little-endian, defined)      */
/* -------------------------------------------------- */

static inline __attribute__((always_inline)) void
vb_write_len(uint8_t *p, uint32_t len, vb_lenfmt_t fmt)
{
    p[0] = (uint8_t)(len & 0xFF);
    p[1] = (uint8_t)((len >> 8) & 0xFF);

    if (fmt == VB_LEN32) {
        p[2] = (uint8_t)((len >> 16) & 0xFF);
        p[3] = (uint8_t)((len >> 24) & 0xFF);
    }
}

static inline __attribute__((always_inline)) uint32_t
vb_read_len(const uint8_t *p, vb_lenfmt_t fmt)
{
    uint32_t len = (uint32_t)p[0]
                 | ((uint32_t)p[1] << 8);

    if (fmt == VB_LEN32) {
        len |= ((uint32_t)p[2] << 16)
            |  ((uint32_t)p[3] << 24);
    }
    return len;
}

static inline void vb_write_lencvc(uint8_t *p, uint32_t len, vb_lenfmt_t fmt) {
    if (fmt == VB_LEN16) {
        *(uint16_t*)p = (uint16_t)len;
    } else {
        *(uint32_t*)p = len;
    }
}

static inline uint32_t vb_read_lencv(const uint8_t *p, vb_lenfmt_t fmt) {
    if (fmt == VB_LEN16) {
        return (uint32_t)(*(const uint16_t*)p);
    }
    return *(const uint32_t*)p;
}

/* -------------------------------------------------- */
/* 2. Public Optimized I/O Functions                  */
/* -------------------------------------------------- */

int VB_Put(vb_handle_t *vb, const void *data, uint32_t len) {
    uint32_t lsz = (vb->lenfmt == VB_LEN16) ? 2 : 4;
    if (vb->block_size == 0) {
        uint8_t lenbuf[4];
        vb_write_len(lenbuf, len, vb->lenfmt);
        if (fwrite(lenbuf, lsz, 1, vb->fp) != 1) return -1;
        if (fwrite(data, 1, len, vb->fp) != len) return -1;
        return 1;
    }
    if (vb->block_used + lsz + len > vb->block_size) {
        if (vb_flush(vb) < 0) return -1;
    }
    vb_write_len(vb->block_buf + vb->block_used, len, vb->lenfmt);
    vb->block_used += lsz;
    __builtin_memcpy(vb->block_buf + vb->block_used, data, len);
    vb->block_used += len;
    return 1;
}

int VB_Get(vb_handle_t *vb, void *buf, uint32_t max_len, uint32_t *out_len) {
    uint32_t lsz = (vb->lenfmt == VB_LEN16) ? 2 : 4;
    if (vb->block_pos + lsz > vb->block_used) {
        if (vb_fill_block(vb) <= 0) return 0;
    }
    uint32_t len = vb_read_len(vb->block_buf + vb->block_pos, vb->lenfmt);
    if (len > max_len) return -1;
    const void *src_ptr = vb->block_buf + vb->block_pos + lsz;
    __builtin_memcpy(buf, src_ptr, len);
    vb->block_pos += (lsz + len);
    *out_len = len;
    return 1;
}

/* VB_Skip: Move past the current record without copying data */
int VB_Skip(vb_handle_t *vb, uint32_t *skipped_len) {
    uint32_t lsz = (vb->lenfmt == VB_LEN16) ? 2 : 4;

    /* 1. Ensure the current block has enough data for at least the length header */
    if (vb->block_pos + lsz > vb->block_used) {
        if (vb_fill_block(vb) <= 0) return 0; /* EOF or Error */
    }

    /* 2. Read the record length (effectively the RDW) */
    uint32_t rlen = vb_read_len(vb->block_buf + vb->block_pos, vb->lenfmt);

    /* 3. Safety check: ensure the record data does not overrun the block buffer */
    if (vb->block_pos + lsz + rlen > vb->block_used) {
        vb->error = 1;
        return -1; /* Corrupt record or block boundary error */
    }

    /* 4. Advance the pointer—no memcpy required, maximizing performance */
    vb->block_pos += (lsz + rlen);
    
    if (skipped_len) {
        *skipped_len = rlen;
    }

    return 1; /* Success */
}
 
/* VB_GetLocate: QSAM-style GET LOCATE with EOF safety */
int VB_GetLocate(vb_handle_t *vb, const char **ptr, uint32_t *len) {
    uint32_t lsz = (vb->lenfmt == VB_LEN16) ? 2 : 4;

    /* Check if current block is exhausted */
    if (vb->block_pos + lsz > vb->block_used) {
        int fill_rc = vb_fill_block(vb);
        if (fill_rc <= 0) return 0; /* EOF or Error: Stop the loop */
    }

    /* Read the RDW from the freshly filled (or current) block */
    uint32_t rlen = vb_read_len(vb->block_buf + vb->block_pos, vb->lenfmt);

    /* Hand back the address directly into the block buffer */
    *ptr = (const char *)(vb->block_buf + vb->block_pos + lsz);
    *len = rlen;

    /* Advance the pointer for the next call */
    vb->block_pos += (lsz + rlen);
    return 1; /* Record found */
}
int VB_GetView(vb_handle_t *vb, vb_view_t *view) {
    uint32_t lsz = (vb->lenfmt == VB_LEN16) ? 2 : 4;
    if (vb->block_pos + lsz > vb->block_used) {
        if (vb_fill_block(vb) <= 0) return 0;
    }
    uint32_t len = vb_read_len(&vb->block_buf[vb->block_pos], vb->lenfmt);
    view->data = (const char *)(vb->block_buf + vb->block_pos + lsz);
    view->len = len;
    vb->block_pos += (lsz + len);
    return 1;
}

 
/* -------------------------------------------------- */
/* Close                                              */
/* -------------------------------------------------- */

int
VB_Close(vb_handle_t *vb)
{
    if (!vb)
        return 0;

    if (vb->mode == VB_MODE_WRITE && vb->block_size)
        vb_flush(vb);

    fclose(vb->fp);
    free(vb->block_buf);
    free(vb);
    return 0;
}

/*static*/ vb_handle_t *vb_alloc(void)
{
    vb_handle_t *vb = (vb_handle_t *) calloc(1, sizeof(*vb));
    return vb;
}

vb_handle_t *VB_OpenWrite(const char *path, uint32_t block_size, vb_lenfmt_t lenfmt)
{
    vb_handle_t *vb = vb_alloc();
    if (!vb) return NULL;

    vb->fp = fopen(path, "wb");
    if (!vb->fp) {
        free(vb);
        return NULL;
    }

    vb->mode = VB_MODE_WRITE;
    vb->lenfmt = lenfmt;
    vb->block_size = block_size;

    vb_file_header_t hdr = {
        .magic = VB_FILE_MAGIC,
        .version = 1,
        .header_size = sizeof(hdr),
        .block_size = block_size,
        .lenfmt = lenfmt,
        .flags = 0,
        .reserved1 = 0,
        .reserved2 = 0
    };

    fwrite(&hdr, sizeof hdr, 1, vb->fp);

    if (block_size) {
        vb->block_buf =  ( uint8_t * ) malloc(block_size);
        vb->block_used = 0;
    }

    return vb;
}

vb_handle_t *VB_OpenRead(const char *path)
{
    vb_file_header_t hdr;
    vb_handle_t *vb = vb_alloc();
    if (!vb) return NULL;

    vb->fp = fopen(path, "rb");
    if (!vb->fp) {
        free(vb);
        return NULL;
    }

    /* Read the global file header */
    if (fread(&hdr, sizeof hdr, 1, vb->fp) != 1) {
        fprintf(stderr, "Error: Could not read file header.\n");
        fclose(vb->fp);
        free(vb);
        return NULL;
    }

    if (hdr.magic != VB_FILE_MAGIC) {
        fclose(vb->fp);
        free(vb);
        return NULL;
    }

    vb->mode = VB_MODE_READ;
    vb->lenfmt = (vb_lenfmt_t)hdr.lenfmt;
    vb->block_size = hdr.block_size;

    if (vb->block_size) {
        vb->block_buf = (uint8_t *)malloc(vb->block_size);
        vb->block_used = 0;
        vb->block_pos = 0;
        /* Note: We don't read the first block here. 
           VB_Get will call vb_fill_block when it sees block_pos == block_used */
    }

    return vb;
}
 
vb_handle_t *VB_OpenReadx(const char *path)
{
    vb_file_header_t hdr;
    vb_handle_t *vb = vb_alloc();
    if (!vb) return NULL;

    vb->fp = fopen(path, "rb");
    if (!vb->fp) {
        free(vb);
        return NULL;
    }

    fread(&hdr, sizeof hdr, 1, vb->fp);

    if (hdr.magic != VB_FILE_MAGIC || hdr.version != 1) {
        fprintf(stderr, "Error: Not a valid VBF1 file.\n");
        fclose(vb->fp);
        free(vb);
        return NULL;
    }

    vb->mode = VB_MODE_READ;
    vb->lenfmt = (vb_lenfmt_t)hdr.lenfmt;
    vb->block_size = hdr.block_size;

    if (fread(&vb->block_used, 4, 1, vb->fp) != 1) {
       fprintf(stderr, "Error: Could not read initial block size.\n");
       fclose(vb->fp);
       free(vb->block_buf);
       free(vb);
       return NULL;
    }


    if (vb->block_size) {
        vb->block_buf = ( uint8_t * ) malloc(vb->block_size);
        vb->block_used = fread(vb->block_buf, 1, vb->block_used, vb->fp);
        vb->block_pos = 0;
    }

    return vb;
}
