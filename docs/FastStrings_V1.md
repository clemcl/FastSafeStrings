# FastStrings V1
## Fixed-Capacity High-Performance Strings for C and C++

Version 1.0 (2026 Edition)

---

## Overview

FastStrings is a deterministic, fixed-capacity string layer for C and C++.

It eliminates repeated `strlen` scanning by maintaining explicit length metadata.
It preserves full compatibility with standard C APIs such as `printf`.

---

## Core Design

```c
typedef struct {
    uint32_t max_len;
    uint32_t cur_len;
} vb_meta_t;
```

The visible string remains a standard `char[]`.

---

## Declaration

```c
#define DCL(name, size) \
    char name[size]; \
    vb_meta_t dv_##name = { (size) - 1, 0 }
```

---

## Key Operations

- `SET(name, "literal")`
- `CPY(dest, src)`
- `CAT(dest, src)`
- `LEN(name)`
- `CMP(a, b)`

---

## Performance

FastStrings avoids redundant memory scanning.

Typical tight-loop benchmarks show significant speedup compared to `strlen + memcpy`.

Reducing unnecessary memory traversal improves CPU efficiency and can reduce energy usage in high-frequency workloads.

---

## Safety

- Fixed capacity
- Automatic truncation
- Always null-terminated
- No heap allocation (C version)

---

## Scope

FastStrings is a memory abstraction only.
File formats are handled separately by the VB Record layer.
