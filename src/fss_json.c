#include "fss_json.h"
#include <string.h>

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
 
int fss_json_find(const char *json, size_t json_len,
                  const char *key, size_t key_len,
                  fss_slice_t *out_val) {
    if (!json || !key || !out_val || json_len < key_len + 3) {
        return 0;
    }

    const char *p = json;
    const char *end = json + json_len;

    while (p < end) {
        /* Advance to next quote */
        if (*p++ != '"') continue;

        const char *k_start = p;
        while (p < end && *p != '"') p++;
        if (p >= end) return 0;

        size_t found_klen = (size_t)(p - k_start);
        p++; /* Skip closing quote */

        /* Fast-reject on key length before reading memory */
        int match = (found_klen == key_len) && (memcmp(k_start, key, key_len) == 0);

        /* Skip trailing whitespace and colon */
        while (p < end && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++;
        if (p < end && *p == ':') p++;
        while (p < end && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++;

        if (p >= end) return 0;

        if (match) {
            if (*p == '"') {
                /* Quoted string value */
                p++;
                out_val->ptr = p;
                const char *v_start = p;
                while (p < end && *p != '"') p++;
                out_val->len = (size_t)(p - v_start);
            } else {
                /* Scalar value (numeric, boolean, null) */
                out_val->ptr = p;
                const char *v_start = p;
                while (p < end && *p != ',' && *p != '}' && *p != ' ' && 
                       *p != '\n' && *p != '\r' && *p != '\t') {
                    p++;
                }
                out_val->len = (size_t)(p - v_start);
            }
            return 1;
        }
    }

    return 0;
}
