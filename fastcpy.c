#pragma  -P
#pragma  inline
#pragma intrinsic(strcpy)
#pragma intrinsic(memcpy)
//#pragma intrinsic 
/*
   FastSafeStrings Benchmark
*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include "faststr.h"

#include <immintrin.h>
#include <stddef.h>

#include <immintrin.h>
#include <stddef.h>

__attribute__((target("avx2")))
int avx2_memcmp(const void *a, const void *b, size_t n)
{
    const unsigned char *pa = (const unsigned char *)a;
    const unsigned char *pb = (const unsigned char *)b;

    while (n >= 32)
    {
        __m256i va = _mm256_loadu_si256((const __m256i*)pa);
        __m256i vb = _mm256_loadu_si256((const __m256i*)pb);

        __m256i cmp = _mm256_cmpeq_epi8(va, vb);
        int mask = _mm256_movemask_epi8(cmp);

        if (mask != -1)
            break;

        pa += 32;
        pb += 32;
        n -= 32;
    }

    for (size_t i = 0; i < n; i++)
        if (pa[i] != pb[i])
            return (int)pa[i] - (int)pb[i];

    return 0;
}
 
 
#define ITER 50000000
#define STRINGLEN 400

volatile unsigned long bench_sink = 0;
 
static double elapsed(clock_t start, clock_t end)
{
    return (double)(end-start) / CLOCKS_PER_SEC;
}


// Force inline behavior across different compilers
#if defined(__BORLANDC__)
#define __builtin_memcpy   memcpy
#define FORCE_INLINE __inline
#elif defined(_MSC_VER)
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE inline __attribute__((always_inline))
#endif
#pragma inline
  

 
int main(void)
{
    printf("FastSafeStrings Benchmark\n\n");

#if defined(__64BIT__) || defined(__x86_64__) || defined(__ppc64__) || defined(_M_X64)
    #define IS_64BIT 1
    printf ("Compiler is 64 bit.\n");
#else
    #define IS_64BIT 0
    printf ("Compiler is probably 32 bit.\n");
#endif

/* Compiler Identification */
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

    clock_t start,end;

    /* -------------------------------------------------- */
    /* strlen benchmark                                   */
    /* -------------------------------------------------- */

    char cstr[40] = "ClementClarke";

    DCL(fstr,STRINGLEN);
    DCL(fa,STRINGLEN);
    DCL(fb,STRINGLEN);
    DCL(fc,STRINGLEN);

    printf("Address of fa: %p\n", (void *)&fa);
    printf("Address of fb: %p\n", (void *)&fb);

    memset(fstr,'A',STRINGLEN);
    fstr[STRINGLEN-1]='\0';
    dv_fstr.cur_len = STRINGLEN - 1;  // <-- Update the descriptor length!
     
    CPY_CSTR(fa,fstr);  // Copy string and set length
    CPY_CSTR(fb,fstr);   
    FSS_DEBUG(fstr);



    /* -------------------------------------------------- */
    /* strcpy benchmark                                   */
    /* -------------------------------------------------- */

    FSS_DEBUG(fa);       // display the strings
    FSS_DEBUG(fb);

    start = clock();

    for(int i=0;i<ITER;i++)
    {
         strcpy(fa,fb);
         fb[STRINGLEN-2]=(char) i & 7 ;
         bench_sink +=1;
    }

    end = clock();

    printf("C strcpy       : %f seconds\n",elapsed(start,end));
    start = clock();


    CPY(fb,fa);  //,LEN(fb));   // Copy fb to fa


    for(int i=0;i<ITER;i++)
    {
  //     memcpy(fa, fb, LEN(fb));   // .3
         fb[STRINGLEN-2]=(char) i & 7 ;

         CPY(fa,fb);


    //    *(uint64_t*)(fa + i) = *(const uint64_t*)(fb + i);
         bench_sink +=1;
    }

    end = clock();

    printf("Fast memcpy CPY: %f seconds\n\n",elapsed(start,end));
    FSS_DEBUG(fa);
    FSS_DEBUG(fb);


    /* -------------------------------------------------- */
    /* strcmp benchmark                                   */
    /* -------------------------------------------------- */


 
    int result = strcmp(fa, fb);
    
    start = clock();

    for(int i=0;i<ITER;i++)
    {
              bench_sink += strcmp(fa,fb);
         fb[STRINGLEN-2- (i &7)]=(char) i & 7 ;
 //        bench_sink +=1; 
    }

    end = clock();

    printf("C strcmp       : %f seconds\n",elapsed(start,end));


    start = clock();

    for(int i=0;i<ITER;i++)
    {
             bench_sink += __builtin_memcmp(fa,fb,400); // CMP(fa,fb);
 //            bench_sink += avx2_memcmp(fa,fb,400); // CMP(fa,fb);
         fb[STRINGLEN-2 -(i &7)]=(char) i & 7 ;
 //       bench_sink +=1; 
    }

    end = clock();

    printf("Fast CMP memcmp: %f seconds\n\n",elapsed(start,end));
    result = memcmp(fa, fb,LEN(fb));


    printf("Iterations:  %d\n",ITER);
    printf("Sink value: %lu\n", bench_sink);

    return 0;
}

