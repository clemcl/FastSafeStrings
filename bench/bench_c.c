#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#else
    #include <time.h>
#endif

#if __has_include("include/faststr.h")
    #include "include/faststr.h"
#elif __has_include("faststr.h")
    #include "faststr.h"
#else
    #include "../include/faststr.h"
#endif

/* ---------------------------------------------------------------------------
 * High-Resolution Timing & Sinks
 * --------------------------------------------------------------------------- */
#if defined(_WIN32) || defined(_WIN64)
static double get_time_ns(void) {
    static LARGE_INTEGER freq;
    static int initialized = 0;
    if (!initialized) {
        QueryPerformanceFrequency(&freq);
        initialized = 1;
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return ((double)counter.QuadPart * 1e9) / (double)freq.QuadPart;
}
#else
static double get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}
#endif

/* Volatile sink to prevent dead-code elimination */
static volatile uint64_t g_sink = 0;
static inline void sink_ptr(const void *p) {
    g_sink += (uintptr_t)p;
}
static inline void sink_val(uint64_t v) {
    g_sink += v;
}

/* ---------------------------------------------------------------------------
 * Benchmark Payload Helpers
 * --------------------------------------------------------------------------- */
static char g_buf_8[9];
static char g_buf_16[17];
static char g_buf_64[65];
static char g_buf_256[257];
static char g_buf_1024[1025];
static char g_buf_4096[4097];

static void init_payloads(void) {
    const char *pattern = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@";
    size_t patlen = strlen(pattern);
    for (size_t i = 0; i < 8; i++) g_buf_8[i] = pattern[i % patlen]; g_buf_8[8] = '\0';
    for (size_t i = 0; i < 16; i++) g_buf_16[i] = pattern[i % patlen]; g_buf_16[16] = '\0';
    for (size_t i = 0; i < 64; i++) g_buf_64[i] = pattern[i % patlen]; g_buf_64[64] = '\0';
    for (size_t i = 0; i < 256; i++) g_buf_256[i] = pattern[i % patlen]; g_buf_256[256] = '\0';
    for (size_t i = 0; i < 1024; i++) g_buf_1024[i] = pattern[i % patlen]; g_buf_1024[1024] = '\0';
    for (size_t i = 0; i < 4096; i++) g_buf_4096[i] = pattern[i % patlen]; g_buf_4096[4096] = '\0';
}

/* ---------------------------------------------------------------------------
 * Benchmark Suites
 * --------------------------------------------------------------------------- */

static void bench_copy(int iterations) {
    printf("\n--- 1. STRING COPY BENCHMARK (%d iterations) ---\n", iterations);
    printf("%-10s | %-16s | %-16s | %-10s\n", "Size", "libc strcpy (ns)", "FSS CPY (ns)", "Speedup");
    printf("-----------|------------------|------------------|-----------\n");

    /* 8B */
    {
        DCL(fss_src, 16);
        DCL(fss_dst, 16);
        CPY_CSTR(fss_src, g_buf_8);
        char libc_dst[16];

        double t0 = get_time_ns();
        for (int i = 0; i < iterations; i++) {
            strcpy(libc_dst, g_buf_8);
            sink_ptr(libc_dst);
        }
        double t_libc = (get_time_ns() - t0) / iterations;

        t0 = get_time_ns();
        for (int i = 0; i < iterations; i++) {
            CPY(fss_dst, fss_src);
            sink_ptr(fss_dst);
        }
        double t_fss = (get_time_ns() - t0) / iterations;
        printf("%-10s | %16.2f | %16.2f | %9.2fx\n", "8 Bytes", t_libc, t_fss, t_libc / (t_fss > 0.0001 ? t_fss : 0.0001));
    }

    /* 16B */
    {
        DCL(fss_src, 32);
        DCL(fss_dst, 32);
        CPY_CSTR(fss_src, g_buf_16);
        char libc_dst[32];

        double t0 = get_time_ns();
        for (int i = 0; i < iterations; i++) {
            strcpy(libc_dst, g_buf_16);
            sink_ptr(libc_dst);
        }
        double t_libc = (get_time_ns() - t0) / iterations;

        t0 = get_time_ns();
        for (int i = 0; i < iterations; i++) {
            CPY(fss_dst, fss_src);
            sink_ptr(fss_dst);
        }
        double t_fss = (get_time_ns() - t0) / iterations;
        printf("%-10s | %16.2f | %16.2f | %9.2fx\n", "16 Bytes", t_libc, t_fss, t_libc / (t_fss > 0.0001 ? t_fss : 0.0001));
    }

    /* 64B */
    {
        DCL(fss_src, 128);
        DCL(fss_dst, 128);
        CPY_CSTR(fss_src, g_buf_64);
        char libc_dst[128];

        double t0 = get_time_ns();
        for (int i = 0; i < iterations; i++) {
            strcpy(libc_dst, g_buf_64);
            sink_ptr(libc_dst);
        }
        double t_libc = (get_time_ns() - t0) / iterations;

        t0 = get_time_ns();
        for (int i = 0; i < iterations; i++) {
            CPY(fss_dst, fss_src);
            sink_ptr(fss_dst);
        }
        double t_fss = (get_time_ns() - t0) / iterations;
        printf("%-10s | %16.2f | %16.2f | %9.2fx\n", "64 Bytes", t_libc, t_fss, t_libc / (t_fss > 0.0001 ? t_fss : 0.0001));
    }

    /* 256B */
    {
        DCL(fss_src, 512);
        DCL(fss_dst, 512);
        CPY_CSTR(fss_src, g_buf_256);
        char libc_dst[512];

        double t0 = get_time_ns();
        for (int i = 0; i < iterations; i++) {
            strcpy(libc_dst, g_buf_256);
            sink_ptr(libc_dst);
        }
        double t_libc = (get_time_ns() - t0) / iterations;

        t0 = get_time_ns();
        for (int i = 0; i < iterations; i++) {
            CPY(fss_dst, fss_src);
            sink_ptr(fss_dst);
        }
        double t_fss = (get_time_ns() - t0) / iterations;
        printf("%-10s | %16.2f | %16.2f | %9.2fx\n", "256 Bytes", t_libc, t_fss, t_libc / (t_fss > 0.0001 ? t_fss : 0.0001));
    }

    /* 1KB */
    {
        DCL(fss_src, 2048);
        DCL(fss_dst, 2048);
        CPY_CSTR(fss_src, g_buf_1024);
        char libc_dst[2048];

        double t0 = get_time_ns();
        for (int i = 0; i < iterations; i++) {
            strcpy(libc_dst, g_buf_1024);
            sink_ptr(libc_dst);
        }
        double t_libc = (get_time_ns() - t0) / iterations;

        t0 = get_time_ns();
        for (int i = 0; i < iterations; i++) {
            CPY(fss_dst, fss_src);
            sink_ptr(fss_dst);
        }
        double t_fss = (get_time_ns() - t0) / iterations;
        printf("%-10s | %16.2f | %16.2f | %9.2fx\n", "1 KB", t_libc, t_fss, t_libc / (t_fss > 0.0001 ? t_fss : 0.0001));
    }

    /* 4KB */
    {
        DCL(fss_src, 8192);
        DCL(fss_dst, 8192);
        CPY_CSTR(fss_src, g_buf_4096);
        char libc_dst[8192];

        double t0 = get_time_ns();
        for (int i = 0; i < iterations; i++) {
            strcpy(libc_dst, g_buf_4096);
            sink_ptr(libc_dst);
        }
        double t_libc = (get_time_ns() - t0) / iterations;

        t0 = get_time_ns();
        for (int i = 0; i < iterations; i++) {
            CPY(fss_dst, fss_src);
            sink_ptr(fss_dst);
        }
        double t_fss = (get_time_ns() - t0) / iterations;
        printf("%-10s | %16.2f | %16.2f | %9.2fx\n", "4 KB", t_libc, t_fss, t_libc / (t_fss > 0.0001 ? t_fss : 0.0001));
    }
}

static void bench_concat(int iterations) {
    printf("\n--- 2. STRING CONCATENATION BENCHMARK (%d iterations) ---\n", iterations);
    printf("%-10s | %-16s | %-16s | %-10s\n", "Size", "libc strcat (ns)", "FSS CAT (ns)", "Speedup");
    printf("-----------|------------------|------------------|-----------\n");

    /* 64B append */
    {
        DCL(fss_dst, 1024);
        DCL(fss_src, 128);
        CPY_CSTR(fss_src, g_buf_64);
        char libc_dst[1024];

        double t0 = get_time_ns();
        for (int i = 0; i < iterations; i++) {
            libc_dst[0] = '\0';
            strcat(libc_dst, g_buf_64);
            strcat(libc_dst, g_buf_64);
            sink_ptr(libc_dst);
        }
        double t_libc = (get_time_ns() - t0) / iterations;

        t0 = get_time_ns();
        for (int i = 0; i < iterations; i++) {
            CLEAR(fss_dst);
            CAT(fss_dst, fss_src);
            CAT(fss_dst, fss_src);
            sink_ptr(fss_dst);
        }
        double t_fss = (get_time_ns() - t0) / iterations;
        printf("%-10s | %16.2f | %16.2f | %9.2fx\n", "64 Bytes", t_libc, t_fss, t_libc / (t_fss > 0.0001 ? t_fss : 0.0001));
    }

    /* 256B append */
    {
        DCL(fss_dst, 2048);
        DCL(fss_src, 512);
        CPY_CSTR(fss_src, g_buf_256);
        char libc_dst[2048];

        double t0 = get_time_ns();
        for (int i = 0; i < iterations; i++) {
            libc_dst[0] = '\0';
            strcat(libc_dst, g_buf_256);
            strcat(libc_dst, g_buf_256);
            sink_ptr(libc_dst);
        }
        double t_libc = (get_time_ns() - t0) / iterations;

        t0 = get_time_ns();
        for (int i = 0; i < iterations; i++) {
            CLEAR(fss_dst);
            CAT(fss_dst, fss_src);
            CAT(fss_dst, fss_src);
            sink_ptr(fss_dst);
        }
        double t_fss = (get_time_ns() - t0) / iterations;
        printf("%-10s | %16.2f | %16.2f | %9.2fx\n", "256 Bytes", t_libc, t_fss, t_libc / (t_fss > 0.0001 ? t_fss : 0.0001));
    }
}

static void bench_length(int iterations) {
    printf("\n--- 3. STRING LENGTH CALCULATION BENCHMARK (%d iterations) ---\n", iterations);
    printf("%-10s | %-16s | %-16s | %-10s\n", "Size", "libc strlen (ns)", "FSS LEN (ns)", "Speedup");
    printf("-----------|------------------|------------------|-----------\n");

    /* 256B */
    {
        DCL(fss_s, 512);
        CPY_CSTR(fss_s, g_buf_256);

        double t0 = get_time_ns();
        for (int i = 0; i < iterations; i++) {
            size_t l = strlen(g_buf_256);
            sink_val(l);
        }
        double t_libc = (get_time_ns() - t0) / iterations;

        t0 = get_time_ns();
        for (int i = 0; i < iterations; i++) {
            uint32_t l = LEN(fss_s);
            sink_val(l);
        }
        double t_fss = (get_time_ns() - t0) / iterations;
        printf("%-10s | %16.2f | %16.2f | %9.2fx\n", "256 Bytes", t_libc, t_fss, t_libc / (t_fss > 0.0001 ? t_fss : 0.0001));
    }

    /* 4KB */
    {
        DCL(fss_s, 8192);
        CPY_CSTR(fss_s, g_buf_4096);

        double t0 = get_time_ns();
        for (int i = 0; i < iterations; i++) {
            size_t l = strlen(g_buf_4096);
            sink_val(l);
        }
        double t_libc = (get_time_ns() - t0) / iterations;

        t0 = get_time_ns();
        for (int i = 0; i < iterations; i++) {
            uint32_t l = LEN(fss_s);
            sink_val(l);
        }
        double t_fss = (get_time_ns() - t0) / iterations;
        printf("%-10s | %16.2f | %16.2f | %9.2fx\n", "4 KB", t_libc, t_fss, t_libc / (t_fss > 0.0001 ? t_fss : 0.0001));
    }
}

static void bench_compare(int iterations) {
    printf("\n--- 4. STRING COMPARISON BENCHMARK (%d iterations) ---\n", iterations);
    printf("%-10s | %-16s | %-16s | %-10s\n", "Size", "libc strcmp (ns)", "FSS CMP (ns)", "Speedup");
    printf("-----------|------------------|------------------|-----------\n");

    /* 64B Match */
    {
        
        DCL(fss_a, 128);
        DCL(fss_b, 128);
        CPY_CSTR(fss_a, g_buf_64);
        CPY_CSTR(fss_b, g_buf_64);
         
       /* Give libc two distinct memory addresses */
        char libc_a[512];
        char libc_b[512];
         
        strcpy(libc_a, g_buf_64);
        strcpy(libc_b, g_buf_64);

        double t0 = get_time_ns();
        for (int i = 0; i < iterations; i++) {
             int r = strcmp(libc_a, libc_b); // Distinct addresses: compiler cannot fold to 0
             sink_val((uint32_t)r);
        }

        double t_libc = (get_time_ns() - t0) / iterations;

        t0 = get_time_ns();
        for (int i = 0; i < iterations; i++) {
            int r = CMP(fss_a, fss_b);
            sink_val((uint32_t)r);
        }
        double t_fss = (get_time_ns() - t0) / iterations;
        printf("%-10s | %16.2f | %16.2f | %9.2fx\n", "64B Match", t_libc, t_fss, t_libc / (t_fss > 0.0001 ? t_fss : 0.0001));
    }

    /* 256B Match */
    {
        DCL(fss_a, 512);
        DCL(fss_b, 512);
        CPY_CSTR(fss_a, g_buf_256);
        CPY_CSTR(fss_b, g_buf_256);
         
       /* Give libc two distinct memory addresses */
        char libc_a[512];
        char libc_b[512];
         

        strcpy(libc_a, g_buf_256);
        strcpy(libc_b, g_buf_256);   

        double t0 = get_time_ns();
        for (int i = 0; i < iterations; i++) {
             int r = strcmp(libc_a, libc_b); // Distinct addresses: compiler cannot fold to 0
             sink_val((uint32_t)r);
        }

        double t_libc = (get_time_ns() - t0) / iterations;

        t0 = get_time_ns();
        for (int i = 0; i < iterations; i++) {
            int r = CMP(fss_a, fss_b);
            sink_val((uint32_t)r);
        }
        double t_fss = (get_time_ns() - t0) / iterations;
        printf("%-10s | %16.2f | %16.2f | %9.2fx\n", "256B Match", t_libc, t_fss, t_libc / (t_fss > 0.0001 ? t_fss : 0.0001));
    }
}

int main(void) {
    printf("===============================================================================\n");
    printf("  FastSafeStrings (FSS) C Microbenchmark Suite\n");
    printf("===============================================================================\n");

    init_payloads();
    const int iters = 1000000;

    bench_copy(iters);
    bench_concat(iters);
    bench_length(iters);
    bench_compare(iters);

    printf("\nBenchmark complete. (sink checksum: %llu)\n", (unsigned long long)g_sink);
    return 0;
}
