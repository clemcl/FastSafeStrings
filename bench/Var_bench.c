/*
   FastSafeStrings Robust Variable Benchmark
*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "Faststr.h"

volatile unsigned long bench_sink = 0;

static double elapsed(clock_t start, clock_t end) {
    return (double)(end - start) / CLOCKS_PER_SEC;
}

/* Increased iterations slightly for larger sizes to avoid 0.000000s clock readings */
static int get_iterations(int size) {
    if (size <= 64)   return 10000000; 
    if (size <= 1024) return 5000000;  
    return 1000000;                     /* 1M iterations ensures Clang tracks time fields */
}

static void build_test_string(char *dst, int size, char fill) {
    for (int i = 0; i < size; i++) {
        dst[i] = fill;
    }
    dst[size] = '\0';
}

int printcompiler(void) {
#if defined(__GNUC__) && !defined(__clang__)
    printf("Compiled with GCC %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#elif defined(__clang__)
    printf("Compiled with Clang %d.%d.%d\n", __clang_major__, __clang_minor__, __clang_patchlevel__);
#elif defined(__BORLANDC__)
    printf("Compiler: Borland %0X\n", __BORLANDC__);
#elif defined(_MSC_VER)
    printf("Compiled with MSVC version %d\n", _MSC_VER);
#else
    printf("Unknown compiler\n");
#endif
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

    clock_t start, end;

    /* -------------------------------------------------- */
    /* 1. LENGTH EVALUATION                               */
    /* -------------------------------------------------- */
    build_test_string(c_src, target_size, 'A');
    dv_fss_src_buf.cur_len = target_size; /* Explicitly simulate dynamic fill length */

    start = clock();
    for (int i = 0; i < iterations; i++) {
        bench_sink += strlen(c_src);
    }
    end = clock();
    double c_len_time = elapsed(start, end);

    start = clock();
    for (int i = 0; i < iterations; i++) {
        bench_sink += LEN(fss_src_buf); 
    }
    end = clock();
    double fss_len_time = elapsed(start, end);

    printf("  Length -> C: %.6f s | FSS: %.6f s | Speedup: %.2fx\n", 
           c_len_time, fss_len_time, fss_len_time > 0 ? c_len_time / fss_len_time : 0);

    /* -------------------------------------------------- */
    /* 2. STRING COPYING                                  */
    /* -------------------------------------------------- */
    build_test_string(c_src, target_size, 'A');
    build_test_string(fss_src_buf, target_size, 'A');
    dv_fss_src_buf.cur_len = target_size;

    start = clock();
    for (int i = 0; i < iterations; i++) {
        strcpy(c_dst, c_src);
        c_src[0] = (char)(i & 1 ? 'A' : 'B'); /* Trick compiler into executing memory updates */
    }
    end = clock();
    double c_cpy_time = elapsed(start, end);

    start = clock();
    for (int i = 0; i < iterations; i++) {
        CPY(fss_dst_buf, fss_src_buf);
        fss_src_buf[0] = (char)(i & 1 ? 'A' : 'B');
    }
    end = clock();
    double fss_cpy_time = elapsed(start, end);

    printf("  Copy   -> C: %.6f s | FSS: %.6f s | Speedup: %.2fx\n", 
           c_cpy_time, fss_cpy_time, fss_cpy_time > 0 ? c_cpy_time / fss_cpy_time : 0);

    /* -------------------------------------------------- */
    /* 3. STRING CONCATENATION                            */
    /* -------------------------------------------------- */
    build_test_string(c_src, target_size, 'A');
    build_test_string(fss_src_buf, target_size, 'A');
    dv_fss_src_buf.cur_len = target_size;

    start = clock();
    for (int i = 0; i < iterations; i++) {
        c_dst[0] = '\0';
        strcat(c_dst, c_src);
        bench_sink += c_dst[0];
    }
    end = clock();
    double c_cat_time = elapsed(start, end);

    start = clock();
    for (int i = 0; i < iterations; i++) {
        dv_fss_dst_buf.cur_len = 0;
        fss_dst_buf[0] = '\0';
        CAT(fss_dst_buf, fss_src_buf);
        bench_sink += fss_dst_buf[0];
    }
    end = clock();
    double fss_cat_time = elapsed(start, end);

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

/* --- NEW UNPREDICTABLE COMPARE LOOPS --- */
    start = clock();
    for (int i = 0; i < iterations; i++) {
        /* Shift the pointer forward by 0 or 1 byte dynamically */
        /* This breaks Clang's ability to optimize the loop away! */
        int offset = (i & 1); 
        bench_sink += strcmp(c_src + offset, c_dst + offset);
    }
    end = clock();
    double c_cmp_time = elapsed(start, end);

    start = clock();
    for (int i = 0; i < iterations; i++) {
        /* Mutate the first byte dynamically, exactly like the CPY test */
        fss_src_buf[0] = (char)(i & 1 ? 'A' : 'B');
        bench_sink += CMP(fss_src_buf, fss_dst_buf);
    }
    end = clock();
    double fss_cmp_time = elapsed(start, end);
    /* Restore character */
    fss_src_buf[0] = 'A';
#if 0    
    start = clock();
    for (int i = 0; i < iterations; i++) {
        bench_sink += CMP(fss_src_buf, fss_dst_buf);
    }
    end = clock();
    double fss_cmp_time = elapsed(start, end);
#endif
    printf("  Compare-> C: %.6f s | FSS: %.6f s | Speedup: %.2fx\n\n", 
           c_cmp_time, fss_cmp_time, fss_cmp_time > 0 ? c_cmp_time / fss_cmp_time : 0);
}

int main(void) {
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
