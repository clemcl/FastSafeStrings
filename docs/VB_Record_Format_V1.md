# VB Record Format V1
## Length-Prefixed Record File Specification

Version 1.0 (2026 Edition)

---

## Overview

VB is a record-oriented file format using length-prefixed records.

Each record is stored as:

[length][data]

---

## Variants

- VB  : 16-bit length prefix
- VB4 : 32-bit length prefix

---

## Header (Recommended)

```c
struct vb_header {
    char     magic[4];     // "VBF1"
    uint32_t block_size;
    uint16_t lenfmt;       // 2 or 4
    uint16_t reserved;
};
```

---

## API

```c
vb_handle_t *VB_OpenWrite(const char *path,
                          uint32_t block_size,
                          vb_lenfmt_t lenfmt);

vb_handle_t *VB_OpenRead(const char *path);

int VB_Put(vb_handle_t *vb,
           const void  *data,
           uint32_t     len);

int VB_Get(vb_handle_t *vb,
           void        *buf,
           uint32_t     max_len,
           uint32_t    *out_len);

int VB_Close(vb_handle_t *vb);
```

---

## Design Principles

- Record-oriented
- Binary-safe
- Length-prefixed
- Blocking optional

---

## Integration with FastStrings

```c
VB_Put(handle, name, LEN(name));
```

VB is independent from the FastStrings memory layer.
