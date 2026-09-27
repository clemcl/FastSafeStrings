#ifndef FSS_JSON_H
#define FSS_JSON_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Copyright Clement Victor Clarke 2026 */
/*
   Zero-copy descriptor pointing directly into the input JSON buffer.
   No heap allocations, no string copies, no null-terminator modifications.
*/
typedef struct {
    const char *ptr;
    size_t len;
} fss_slice_t;

/*
   Searches for a top-level or nested key in a JSON payload.
   
   - json: Pointer to JSON text
   - json_len: Length of the JSON buffer
   - key: Key name to find (excluding surrounding quotes)
   - key_len: Length of key name
   - out_val: Populated with pointer and length of the value on success
   
   Returns 1 if found, 0 if not found or malformed.
*/
int fss_json_find(const char *json, size_t json_len,
                  const char *key, size_t key_len,
                  fss_slice_t *out_val);

/*
   Helper to compare a slice against a C-string literal.
   Rejects immediately on length mismatch before scanning memory.
*/
static inline int fss_slice_equals(const fss_slice_t *slice, const char *lit, size_t lit_len) {
    if (slice->len != lit_len) return 0;
    return memcmp(slice->ptr, lit, lit_len) == 0;
}

#ifdef __cplusplus
}
#endif

#endif /* FSS_JSON_H */
