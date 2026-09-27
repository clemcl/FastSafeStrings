// json_bench.c
// Compile:  gcc -O3 -march=native json_bench.c -o json_bench
// Run:      ./json_bench

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

// Adjust this to your actual FSS header
#include "faststr.h"

// ---------- RDTSC (x86_64) ----------
static inline uint64_t rdtsc(void) {
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

// ---------- Test JSON payloads ----------
static const char json_small[] =
    "{\"user\":\"clem\",\"action\":\"update\",\"id\":12345}";

static const char json_medium[] =
    "{"
    "\"user\":\"clem\","
    "\"action\":\"update\","
    "\"timestamp\":\"2026-09-24T09:30:00Z\","
    "\"payload\":{\"id\":12345,\"status\":\"active\","
    "\"message\":\"FastSafeStrings JSON demo\"}"
    "}";

static const char json_large[] =
    "{"
    "\"user\":\"clem\",\"action\":\"bulk\",\"items\":["
    "{\"id\":1,\"msg\":\"one\"},"
    "{\"id\":2,\"msg\":\"two\"},"
    "{\"id\":3,\"msg\":\"three\"},"
    "{\"id\":4,\"msg\":\"four\"},"
    "{\"id\":5,\"msg\":\"five\"}"
    "],"
    "\"meta\":{\"source\":\"bench\",\"version\":\"1.0\"}"
    "}";

// ---------- Baseline C implementations ----------

char *baseline_find_key(const char *json, const char *key) {
    size_t key_len = strlen(key);
    const char *p = json;

    while (*p) {
        if (strncmp(p, key, key_len) == 0)
            return (char *)p;
        p++;
    }
    return NULL;
}

char *baseline_extract_value(const char *start) {
    while (*start && *start != ':') start++;
    if (*start == ':') start++;
    while (*start == ' ' || *start == '"') start++;
    return (char *)start;
}

int baseline_token_eq(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

int baseline_validate_utf8(const unsigned char *s) {
    while (*s) {
        if (*s < 0x80) { s++; continue; }
        if ((*s & 0xE0) == 0xC0) s += 2;
        else if ((*s & 0xF0) == 0xE0) s += 3;
        else if ((*s & 0xF8) == 0xF0) s += 4;
        else return 0;
    }
    return 1;
}

void baseline_parse_small(void) {
    baseline_find_key(json_small, "user");
    baseline_find_key(json_small, "action");
    baseline_validate_utf8((const unsigned char *)json_small);
}

void baseline_parse_medium(void) {
    baseline_find_key(json_medium, "user");
    baseline_find_key(json_medium, "action");
    baseline_find_key(json_medium, "timestamp");
    baseline_validate_utf8((const unsigned char *)json_medium);
}

void baseline_parse_large(void) {
    baseline_find_key(json_large, "user");
    baseline_find_key(json_large, "meta");
    baseline_validate_utf8((const unsigned char *)json_large);
}

// ---------- FastSafeStrings versions ----------
// NOTE: adjust vb_meta_t, DCL, LEN, CMP_BLK, etc. to match your actual API.

vb_meta_t meta_small;
vb_meta_t meta_medium;
vb_meta_t meta_large;

void init_fss_meta(void) {
    // Assuming vb_meta_t has fields: cur_len, max_len
    meta_small.cur_len  = (int)strlen(json_small);
    meta_small.max_len  = meta_small.cur_len;
    meta_medium.cur_len = (int)strlen(json_medium);
    meta_medium.max_len = meta_medium.cur_len;
    meta_large.cur_len  = (int)strlen(json_large);
    meta_large.max_len  = meta_large.cur_len;
}

// Example FSS key compare using CMP_BLK and stored lengths.
// Replace CMP_BLK with your actual block compare function.
char *fss_find_key(const char *json, vb_meta_t *json_meta,
                   const char *key, vb_meta_t *key_meta) {

    size_t key_len  = (size_t)key_meta->cur_len;
    size_t json_len = (size_t)json_meta->cur_len;

    for (size_t i = 0; i + key_len < json_len; i++) {
  //      if (CMP_BLK(json + i, key, key_len) == 0)
        if (memcmp(json + i, key, key_len) == 0)
            return (char *)(json + i);
    }
    return NULL;
}

// Simple meta for constant keys
vb_meta_t meta_user   = { 4, 4 };   // "user"
vb_meta_t meta_action = { 6, 6 };   // "action"
vb_meta_t meta_ts     = { 9, 9 };   // "timestamp"
vb_meta_t meta_meta   = { 4, 4 };   // "meta"

int fss_validate_utf8(const unsigned char *s, size_t len) {
    // Replace with your vectorized UTF-8 validator if available
    return baseline_validate_utf8(s); // placeholder
}

void fss_parse_small(void) {
    fss_find_key(json_small, &meta_small, "user", &meta_user);
    fss_find_key(json_small, &meta_small, "action", &meta_action);
    fss_validate_utf8((const unsigned char *)json_small, meta_small.cur_len);
}

void fss_parse_medium(void) {
    fss_find_key(json_medium, &meta_medium, "user", &meta_user);
    fss_find_key(json_medium, &meta_medium, "action", &meta_action);
    fss_find_key(json_medium, &meta_medium, "timestamp", &meta_ts);
    fss_validate_utf8((const unsigned char *)json_medium, meta_medium.cur_len);
}

void fss_parse_large(void) {
    fss_find_key(json_large, &meta_large, "user", &meta_user);
    fss_find_key(json_large, &meta_large, "meta", &meta_meta);
    fss_validate_utf8((const unsigned char *)json_large, meta_large.cur_len);
}

// ---------- Benchmark harness ----------

uint64_t bench_void(void (*fn)(void), int iters) {
    uint64_t start = rdtsc();
    for (int i = 0; i < iters; i++) {
        fn();
    }
    uint64_t end = rdtsc();
    return end - start;
}

int main(void) {
    int iters = 5 * 1000 * 1000;

    init_fss_meta();

    printf("JSON benchmark (iterations = %d)\n\n", iters);

    uint64_t base_small  = bench_void(baseline_parse_small, iters);
    uint64_t fss_small   = bench_void(fss_parse_small, iters);

    uint64_t base_medium = bench_void(baseline_parse_medium, iters);
    uint64_t fss_medium  = bench_void(fss_parse_medium, iters);

    uint64_t base_large  = bench_void(baseline_parse_large, iters);
    uint64_t fss_large   = bench_void(fss_parse_large, iters);

    printf("Small JSON:\n");
    printf("  Baseline cycles: %llu\n", (unsigned long long)base_small);
    printf("  FSS      cycles: %llu\n", (unsigned long long)fss_small);
    printf("  Speedup: %.2fx\n\n",
           (double)base_small / (double)fss_small);

    printf("Medium JSON:\n");
    printf("  Baseline cycles: %llu\n", (unsigned long long)base_medium);
    printf("  FSS      cycles: %llu\n", (unsigned long long)fss_medium);
    printf("  Speedup: %.2fx\n\n",
           (double)base_medium / (double)fss_medium);

    printf("Large JSON:\n");
    printf("  Baseline cycles: %llu\n", (unsigned long long)base_large);
    printf("  FSS      cycles: %llu\n", (unsigned long long)fss_large);
    printf("  Speedup: %.2fx\n\n",
           (double)base_large / (double)fss_large);

    printf("Tip: run under perf for counters, e.g.\n");
    printf("  perf stat -e cycles,instructions,branches,branch-misses,");
    printf("L1-dcache-loads,L1-dcache-load-misses ./json_bench\n");

    return 0;
}
