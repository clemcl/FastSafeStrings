/*
   FastSafeStrings Benchmark
*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "Faststr.h"   /* FIX: match exact filename case for Linux/ext4 */

#define ITER 20000000

/* FIX: moved out of main() — preprocessor directives are not block-scoped */
#if defined(__64BIT__) || defined(__x86_64__) || defined(__ppc64__) || defined(_M_X64)
    #define IS_64BIT 1
#else
    #define IS_64BIT 0
#endif

/* Compiler identification */
#if defined(__clang__)
    #define COMPILER "Clang"
#elif defined(__GNUC__)
    #define COMPILER "GCC"
#elif defined(__IBMC__) || defined(__xlC__)
    #define COMPILER "IBM xlc"
#elif defined(_MSC_VER)
    #define COMPILER "MSVC"
#else
    #define COMPILER "Unknown"
#endif

volatile unsigned long bench_sink = 0;

static double elapsed(clock_t start, clock_t end)
{
    return (double)(end - start) / CLOCKS_PER_SEC;
}

int main(void)
{
    printf("FastSafeStrings Benchmark\n\n");

#if defined(__GNUC__) && !defined(__clang__)
    printf("Compiled with GCC %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);

#elif defined(__clang__)
    printf("Compiled with Clang %d.%d.%d\n", __clang_major__, __clang_minor__, __clang_patchlevel__);

#elif defined(__BORLANDC__)
    printf("Compiler: Borland %0X\n", __BORLANDC__);

#elif defined(_MSC_VER)
    printf("Compiled with MSVC (Visual C++) version %d\n", _MSC_VER);

#elif defined(__IBMC__) || defined(__xlC__)
    /* FIX: __IBM__ does not exist; use __IBMC__ for the C compiler version */
    printf("Compiled with IBM XLC %d\n", __IBMC__);

#else
    printf("Unknown compiler\n");
#endif

    /* -------------------------------------------------- */
    /* strlen benchmark                                   */
    /* -------------------------------------------------- */

    char cstr[40] = "ClementClarke";

    DCL(fstr, 40);
    CPY_CSTR(fstr, "ClementClarke");
    FSS_DEBUG(fstr);

    char a[90] = "";
    char b[90] = "ABCDEFGHIJK";
    char *va = a;
    char *vb = b;

    printf("a=: %s\n", a);
    printf("b=: %s\n", b);

    clock_t start = clock();

    for (int i = 0; i < ITER; i++)
    {
        bench_sink += strlen(fstr);
    }

    clock_t end = clock();

    printf("C strlen      : %f seconds\n", elapsed(start, end));


    start = clock();

    for (int i = 0; i < ITER; i++)
    {
        volatile size_t len = LEN(fstr);
        (void)len;
    }

    end = clock();

    printf("Fast LEN      : %f seconds\n\n", elapsed(start, end));


    /* -------------------------------------------------- */
    /* strcpy benchmark                                   */
    /* -------------------------------------------------- */

    DCL(fa, 40);
    DCL(fb, 40);

    CPY_CSTR(fa, "ABCDEFxHIJKLMNOP");
    CPY_CSTR(fb, "ABCDEFGHIJK");

    memset(b, 'b', 7);
    memset(a, 'l', 7);
    /* FIX: duplicate a[8]/b[8] assignments removed — set each once */
    a[8] = 0;
    b[8] = 0; 

    CPY_CSTR(fa, va);
    CPY_CSTR(fb, vb);

    FSS_DEBUG(fa);
    FSS_DEBUG(fb);

    start = clock();

    for (int i = 0; i < ITER; i++)
    {
        strcpy((char *) va, (char *) vb);
        bench_sink += 1;
    }

    end = clock();

    printf("C strcpy      : %f seconds\n", elapsed(start, end));


    start = clock();

    for (int i = 0; i < ITER; i++)
    {
        CPY(fa, fb);
        bench_sink += 1;
    }

    end = clock();

    printf("Fast CPY      : %f seconds\n\n", elapsed(start, end));


    /* -------------------------------------------------- */
    /* strcat benchmark                                   */
    /* -------------------------------------------------- */

    /* FIX: section header was placed after the loop — moved to correct position */
    start = clock();

    for (int i = 0; i < ITER; i++)
    {
        a[0] = '\0';
        strcat(a, b);
        bench_sink += 1;
    }

    end = clock();

    printf("C strcat      : %f seconds\n", elapsed(start, end));


    start = clock();

    for (int i = 0; i < ITER; i++)
    {
        dv_fa.cur_len = 0;
        fa[0] = '\0';
        CAT(fa, fb);
        bench_sink += 1;
    }

    end = clock();

    printf("Fast CAT      : %f seconds\n\n", elapsed(start, end));

    /* -------------------------------------------------- */
    /* strcmp benchmark                                   */
    /* -------------------------------------------------- */

    start = clock();

    for (int i = 0; i < ITER; i++)
    {
        bench_sink += (unsigned long)(unsigned int)strcmp(a, b);
    }

    end = clock();

    printf("C strcmp      : %f seconds\n", elapsed(start, end));


    start = clock();

    for (int i = 0; i < ITER; i++)
    {
        bench_sink += (unsigned long)(unsigned int)CMP(fa, fb);
    }

    end = clock();

    printf("Fast CMP      : %f seconds\n\n", elapsed(start, end));


    /* -------------------------------------------------- */
    /* literal assignment                                 */
    /* -------------------------------------------------- */

    start = clock();

    for (int i = 0; i < ITER; i++)
    {
        strcpy(a, "ABCDEF");
        cstr[0] = (char)(i & 7) + 'A';
        bench_sink += 1;
    }

    end = clock();

    printf("C literal copy: %f seconds\n", elapsed(start, end));


    start = clock();

    for (int i = 0; i < ITER; i++)
    {
        CPY_CSTR(fa, "ABCDEF");
        cstr[0] = (char)(i & 7) + 'A';
        bench_sink += 1;
    }

    end = clock();

    printf("Fast SET      : %f seconds\n\n", elapsed(start, end));


    printf("Iterations: %d\n", ITER);
    printf("Sink value: %lu\n", bench_sink);

    return 0;
}
