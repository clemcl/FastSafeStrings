
---

# ?? DESIGN.md

```markdown
# Design Philosophy

FastStrings is intentionally minimal.

## Core Structure (C)

```c
typedef struct {
    uint32_t max_len;
    uint32_t cur_len;
} vb_meta_t;
