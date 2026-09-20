/*
   FastSafeStrings Robust Variable Benchmark
*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "Faststr.h"

/* FIX: clock() on this system only ticks in ~1ms increments (visible in
   the earlier .lst as suspiciously round 0.008s/0.010s readings), which
   isn't fine-grained enough to resolve a genuinely O(1) LEN() call once
   its total loop time drops to a handful of milliseconds. Windows'
   QueryPerformanceCounter gives sub-microsecond resolution and works the
   same from GCC and Clang (MinGW / clang for Windows) with no extra
   linking -- it's part of kernel32, which every Win32 program already
   links against. */
#include <windows.h>

typedef LARGE_INTEGER hr_time_t;
static double hr_freq_inv = 0.0;

static void hr_init(void) {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    hr_freq_inv = 1.0 / (double)freq.QuadPart;
}

static hr_time_t hr_now(void) {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t;
}

volatile unsigned long bench_sink = 0;

static double elapsed(hr_time_t start, hr_time_t end) {
    return (double)(end.QuadPart - start.QuadPart) * hr_freq_inv;
}

#define NET_TIME(raw, base) (((raw) > (base)) ? ((raw) - (base)) : 0.000001)

/* FIX: without this, GCC/Clang can prove a per-iteration read like LEN()
   never changes across the loop and hoist it out entirely, timing
   ~nothing instead of ~iterations calls. This forces a fresh read/write
   each pass. MSVC has no equivalent here; add _ReadWriteBarrier() if
   you need to benchmark under MSVC. */
#if defined(__GNUC__) || defined(__clang__)
    #define COMPILER_BARRIER() __asm__ volatile("" ::: "memory")
#else
    #define COMPILER_BARRIER() /* no-op on this compiler */
#endif

/* Increased iterations slightly for larger sizes to avoid 0.000000s clock readings */
static int get_iterations(int size) {
    if (size <= 64)   return 10000000; 
    if (size <= 1024) return 5000000;  
    return 1000000;                     /* 1M iterations ensures Clang tracks time fields */
}

/* FIX: LEN() is O(1) -- its per-call cost doesn't grow with target_size,
   unlike strcpy/strcat/strcmp/strlen. get_iterations() deliberately
   *shrinks* the iteration count as size grows (to keep the O(n)
   tests from taking forever), but applying that same shrink to Length
   just makes its already-tiny total runtime harder to measure at
   exactly the sizes where the contrast matters most. Length gets its
   own fixed, high count instead. */
#define LENGTH_ITERATIONS 50000000

static void build_test_string(char *dst, int size, char fill) {
    for (int i = 0; i < size; i++) {
        dst[i] = fill;
    }
    dst[size] = '\0';
}

int printcompiler(void) {

/* FIX: moved out of main() — preprocessor directives are not block-scoped */
#if defined(__64BIT__) || defined(__x86_64__) || defined(__ppc64__) || defined(_M_X64)
    #define IS_64BIT 1
#else
    #define IS_64BIT 0
#endif


/* Compiler identification */
#if defined(__GNUC__) && !defined(__clang__)
    printf("Compiled with GCC %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);

    // Detect Clang
#elif defined(__clang__)
    printf("Compiled with Clang %d.%d.%d\n", __clang_major__, __clang_minor__, __clang_patchlevel__);

#elif defined(__BORLANDC__)
    printf("Compiled with Borland %0X\n", __BORLANDC__);

    // Detect Microsoft Visual C++
#elif defined(_MSC_VER)
    printf("Compiled with MSVC (Visual C++) version %d\n", _MSC_VER);

    // Detect IBM C z/OS
#elif defined(__IBMC__) || defined(__xlC__)
    printf("Compiled with IBM XLC %d\n", __IBM__);

    // Unknown compiler
#else
    printf("Unknown compiler\n");
#endif

    printf("File: %s\n", __FILE__);
    printf("Date: %s\n", __DATE__);

#if defined(__64BIT__) || defined(__x86_64__) || defined(__ppc64__) || defined(_M_X64)
    #define IS_64BIT 1
    printf ("Compiler is 64 bit.\n");
#else
    #define IS_64BIT 0
    printf ("Compiler is probably 32 bit.\n");
#endif
    printf ("\n");
 
    return 0;
}

void run_size_benchmark(int target_size) {
    int iterations = get_iterations(target_size);
    printf("=== BENCHMARKING SIZE: %d bytes (%d iterations) ===\n", target_size, iterations);

    /* Allocate maximum array sizing safely */
    char c_src[16385];
    char c_dst[16385];
    DCL(fss_src_buf, 16385);
    DCL(fss_dst_buf, 16385);

    hr_time_t start, end;
    double baseline_overhead;

    /* -------------------------------------------------- */
    /* 0. BASELINE OVERHEAD CALIBRATION                   */
    /* -------------------------------------------------- */
    build_test_string(c_src, target_size, 'A');

    start = hr_now();
    for (int i = 0; i < iterations; i++) {
        c_src[0] = (char)(i & 1 ? 'A' : 'B');
        COMPILER_BARRIER();
        bench_sink += c_src[0];
    }
    end = hr_now();
    baseline_overhead = elapsed(start, end);

    printf("  Loop Overhead Base Time (Subtracted from Timings)     : %.6f s\n", 
           baseline_overhead);

    /* -------------------------------------------------- */
    /* 1. LENGTH EVALUATION                               */
    /* -------------------------------------------------- */
    build_test_string(c_src, target_size, 'A');
    dv_fss_src_buf.cur_len = target_size; /* Explicitly simulate dynamic fill length */

    start = hr_now();
    for (int i = 0; i < LENGTH_ITERATIONS; i++) {
        COMPILER_BARRIER();
        bench_sink += strlen(c_src);
        /* FIX: was c_src[target_size], which is exactly where build_test_string
           put the null terminator -- that clobbered it on iteration 0, and
           every strlen() after that was reading UB past the buffer. Touch
           byte 0 instead; it doesn't affect string length or validity. */
        c_src[0] = (char)(i & 1 ? 'A' : 'B');
    }
    end = hr_now();
 /*   double c_len_time = NET_TIME(elapsed(start, end), baseline_overhead);  */

/* For Length: use plain raw elapsed time */
    double c_len_time   = elapsed(start, end);
    
    start = hr_now();
    for (int i = 0; i < LENGTH_ITERATIONS; i++) {
        /* FIX: added COMPILER_BARRIER() -- nothing in this loop body changes
           dv_fss_src_buf.cur_len, so without a barrier the optimizer can
           (and did) hoist the LEN() read out of the loop entirely and
           report ~0s regardless of true per-call cost. */
        COMPILER_BARRIER();
        bench_sink += LEN(fss_src_buf); 
        fss_src_buf[0] = (char)(i & 1 ? 'A' : 'B');
    }
    end = hr_now();
 //   double fss_len_time = NET_TIME(elapsed(start, end), baseline_overhead);
    double fss_len_time = elapsed(start, end);

    printf("  Length -> C: %.6f s | FSS: %.6f s | Speedup: %.2fx  (%d iters)\n", 
           c_len_time, fss_len_time, fss_len_time > 0 ? c_len_time / fss_len_time : 0,
           LENGTH_ITERATIONS);

    /* -------------------------------------------------- */
    /* 2. STRING COPYING                                  */
    /* -------------------------------------------------- */
    build_test_string(c_src, target_size, 'A');
    build_test_string(fss_src_buf, target_size, 'A');
    dv_fss_src_buf.cur_len = target_size;

    start = hr_now();
    for (int i = 0; i < iterations; i++) {
        COMPILER_BARRIER();
        strcpy(c_dst, c_src);
        /* FIX: was c_src[target_size] -- same terminator-clobbering bug as
           the Length test above. After iteration 0, strcpy() was copying
           an unterminated (UB) source, likely running well past
           target_size bytes. Byte 0 is safe to touch instead. */
        c_src[0] = (char)(i & 1 ? 'A' : 'B');
    }
    end = hr_now();
    double c_cpy_time = NET_TIME(elapsed(start, end), baseline_overhead);

    start = hr_now();
    for (int i = 0; i < iterations; i++) {
        COMPILER_BARRIER();
  //      CPY_REP_MOVSB_BCC(fss_dst_buf, fss_src_buf);
  //     CPY_REP_MOVSB(fss_dst_buf, fss_src_buf);
        CPY(fss_dst_buf, fss_src_buf);
        fss_src_buf[0] = (char)(i & 1 ? 'A' : 'B');
    }
    end = hr_now();
    double fss_cpy_time = NET_TIME(elapsed(start, end), baseline_overhead);

    printf("  Copy   -> C: %.6f s | FSS: %.6f s | Speedup: %.2fx\n", 
           c_cpy_time, fss_cpy_time, fss_cpy_time > 0 ? c_cpy_time / fss_cpy_time : 0);

    /* -------------------------------------------------- */
    /* 3. STRING CONCATENATION                            */
    /* -------------------------------------------------- */
    build_test_string(c_src, target_size, 'A');
    build_test_string(fss_src_buf, target_size, 'A');
    dv_fss_src_buf.cur_len = target_size;

    /* This test was already sound -- no terminator clobbering, no
       asymmetric early exit. Barriers added below only for consistency
       with the other three tests; they shouldn't change the numbers. */
    start = hr_now();
    for (int i = 0; i < iterations; i++) {
        COMPILER_BARRIER();
        c_dst[0] = '\0';
        strcat(c_dst, c_src);
        bench_sink += c_dst[0];
    }
    end = hr_now();
    double c_cat_time = NET_TIME(elapsed(start, end), baseline_overhead);

    start = hr_now();
    for (int i = 0; i < iterations; i++) {
        COMPILER_BARRIER();
        dv_fss_dst_buf.cur_len = 0;
        fss_dst_buf[0] = '\0';
        CAT(fss_dst_buf, fss_src_buf);
        bench_sink += fss_dst_buf[0];
    }
    end = hr_now();
    double fss_cat_time = NET_TIME(elapsed(start, end), baseline_overhead);

    printf("  Concat -> C: %.6f s | FSS: %.6f s | Speedup: %.2fx\n", 
           c_cat_time, fss_cat_time, fss_cat_time > 0 ? c_cat_time / fss_cat_time : 0);

    /* -------------------------------------------------- */
    /* 4. STRING COMPARISON (True Character Evaluation)   */
    /* -------------------------------------------------- */
    build_test_string(c_src, target_size, 'A');
    build_test_string(c_dst, target_size, 'A');
    c_dst[target_size - 1] = 'Z'; /* Force worst-case scan traversal */

    build_test_string(fss_src_buf, target_size, 'A');
    build_test_string(fss_dst_buf, target_size, 'A');
    fss_dst_buf[target_size - 1] = 'Z';
    dv_fss_src_buf.cur_len = target_size;
    dv_fss_dst_buf.cur_len = target_size;

    /* FIX: the FSS loop below toggled fss_src_buf[0] between 'A'/'B' every
       iteration. c_dst/fss_dst_buf both start with 'A' at byte 0, so that
       toggle made CMP() short-circuit at byte 0 on ~half the iterations,
       while strcmp() below had no equivalent shortcut and scanned to the
       real mismatch ('Z') near the end every time -- comparing "sometimes
       1 byte" against "always ~target_size bytes" isn't a fair test of
       comparison throughput. Dropped the offset trick and gave the C loop
       the identical toggle so both sides get the same early-exit chance. */
    start = hr_now();
    for (int i = 0; i < iterations; i++) {
        COMPILER_BARRIER();
        c_src[0] = (char)(i & 1 ? 'A' : 'B');
        bench_sink += strcmp(c_src, c_dst);
    }
    end = hr_now();
    double c_cmp_time = NET_TIME(elapsed(start, end), baseline_overhead);
    c_src[0] = 'A';

    start = hr_now();
    for (int i = 0; i < iterations; i++) {
        COMPILER_BARRIER();
        fss_src_buf[0] = (char)(i & 1 ? 'A' : 'B');
        bench_sink += CMP(fss_src_buf, fss_dst_buf);
    }
    end = hr_now();
    double fss_cmp_time = NET_TIME(elapsed(start, end), baseline_overhead);
    fss_src_buf[0] = 'A';

    printf("  Compare-> C: %.6f s | FSS: %.6f s | Speedup: %.2fx\n\n", 
           c_cmp_time, fss_cmp_time, fss_cmp_time > 0 ? c_cmp_time / fss_cmp_time : 0);
}

int main(void) {
    hr_init();
    printf("FastSafeStrings Variable Character Benchmark (Fixed)\n");
    printf("===================================================\n\n");
    printcompiler();
     
    int sizes[] = {16, 64, 256, 1024, 4096, 16384};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int i = 0; i < num_sizes; i++) {
        run_size_benchmark(sizes[i]);
    }

    printf("Sink verification value: %lu\n", bench_sink);
    return 0;
}
