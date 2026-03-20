#include <string.h>
#include <stdlib.h>

#include "vb_file.h"

/* -------------------------------------------------- */
/* Length field helpers (little-endian, defined)      */
/* -------------------------------------------------- */

static inline void
vb_write_len(uint8_t *p, uint32_t len, vb_lenfmt_t fmt)
{
    p[0] = (uint8_t)(len & 0xFF);
    p[1] = (uint8_t)((len >> 8) & 0xFF);

    if (fmt == VB_LEN32) {
        p[2] = (uint8_t)((len >> 16) & 0xFF);
        p[3] = (uint8_t)((len >> 24) & 0xFF);
    }
}

static inline uint32_t
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

/* -------------------------------------------------- */
/* Block flush (write side)                            */
/* -------------------------------------------------- */

static int
vb_flush(vb_handle_t *vb)
{
    if (vb->block_used == 0)
        return 0;

    fwrite(&vb->block_used, 4, 1, vb->fp);   /* Output Blocksize */  

    if (fwrite(vb->block_buf, 1, vb->block_used, vb->fp) != vb->block_used) {
        vb->error = 1;
        return -1;
    }

    vb->block_used = 0;
    return 0;
}

/* -------------------------------------------------- */
/* Write a record                                     */
/* -------------------------------------------------- */

int
VB_Put(vb_handle_t *vb, const void *data, uint32_t len)
{
    uint8_t lenbuf[4];
    uint32_t lsz = (vb->lenfmt == VB_LEN16) ? 2 : 4;

    vb_write_len(lenbuf, len, vb->lenfmt);

    /* Unblocked */
    if (vb->block_size == 0) {
        if (fwrite(lenbuf, lsz, 1, vb->fp) != 1) return -1;
        if (fwrite(data, 1, len, vb->fp) != len) return -1;
        return 1;
    }

    /* Blocked */
    if (lsz + len > vb->block_size)
        return -1;  /* record too large */

    if (vb->block_used + lsz + len > vb->block_size) {
        if (vb_flush(vb) < 0)
            return -1;
    }

    memcpy(vb->block_buf + vb->block_used, lenbuf, lsz);
    vb->block_used += lsz;

    memcpy(vb->block_buf + vb->block_used, data, len);
    vb->block_used += len;

    return 1;
}

/* -------------------------------------------------- */
/* Read a record                                      */
/* -------------------------------------------------- */

int
VB_Get(vb_handle_t *vb, void *buf, uint32_t max_len, uint32_t *out_len)
{
    uint8_t lenbuf[4];
    uint32_t lsz = (vb->lenfmt == VB_LEN16) ? 2 : 4;

    /* Unblocked */
    if (vb->block_size == 0) {
        if (fread(lenbuf, lsz, 1, vb->fp) != 1)
            return 0;  /* EOF */

        uint32_t len = vb_read_len(lenbuf, vb->lenfmt);
        if (len > max_len)
            return -1;

        if (fread(buf, 1, len, vb->fp) != len)
            return -1;

        *out_len = len;
        return 1;
    }

    /* Blocked */
    if (vb->block_pos + lsz > vb->block_used) {
        fread(&vb->block_used, 4, 1, vb->fp);   /* Input Blocksize */  
        vb->block_used = fread(vb->block_buf, 1,
                               vb->block_used, vb->fp);
        vb->block_pos = 0;
        if (vb->block_used == 0)
            return 0;
    }

    memcpy(lenbuf, vb->block_buf + vb->block_pos, lsz);
    vb->block_pos += lsz;

    uint32_t len = vb_read_len(lenbuf, vb->lenfmt);
    if (len > max_len)
        return -1;

    memcpy(buf, vb->block_buf + vb->block_pos, len);
    vb->block_pos += len;

    *out_len = len;
    return 1;
}

static int vb_fill_block(vb_handle_t *vb)
{
    if (vb->mode != VB_MODE_READ || !vb->block_size) return 0;

    size_t n = fread(vb->block_buf, 1, vb->block_size, vb->fp);
    if (n <= 0) {
        vb->block_used = 0;
        vb->block_pos = 0;
        return 0; // EOF or Error
    }

    vb->block_used = (uint32_t)n;
    vb->block_pos = 0;
    return 1;
}
 
int VB_Skip(vb_handle_t *vb, uint32_t *skipped_len)
{
    // 1. Ensure we have a block in memory
    if (vb->block_pos >= vb->block_used) {
        if (vb_fill_block(vb) <= 0) return 0;
    }

    // 2. Peek at the length
    uint32_t len = vb_read_len(&vb->block_buf[vb->block_pos], vb->lenfmt);
    uint32_t total_rec_size = (uint32_t)vb->lenfmt + len;

    // 3. Jump the pointer
    if (vb->block_pos + total_rec_size > vb->block_used) {
        // Record spans blocks or is corrupted
        return -1; 
    }

    vb->block_pos += total_rec_size;
    if (skipped_len) *skipped_len = len;
    
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

    fread(&vb->block_used, 4, 1, vb->fp);   /* Input Blocksize */

    if (vb->block_size) {
        vb->block_buf = ( uint8_t * ) malloc(vb->block_size);
        vb->block_used = fread(vb->block_buf, 1, vb->block_used, vb->fp);
        vb->block_pos = 0;
    }

    return vb;
}
