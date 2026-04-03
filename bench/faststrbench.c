/*
   FastSafeStrings Benchmark
*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "faststr.h"

#define ITER 20000000

volatile unsigned long bench_sink = 0;
 
static double elapsed(clock_t start, clock_t end)
{
    return (double)(end-start) / CLOCKS_PER_SEC;
}

int main(void)
{
    printf("FastSafeStrings Benchmark\n\n");

#if defined(__64BIT__) || defined(__x86_64__) || defined(__ppc64__) || defined(_M_X64)
    #define IS_64BIT 1
#else
    #define IS_64BIT 0
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

    // Detect Microsoft Visual C++
#elif defined(_MSC_VER)
    printf("Compiled with MSVC (Visual C++) version %d\n", _MSC_VER);

    // Unknown compiler
#else
    printf("Unknown compiler\n");
#endif
 
    clock_t start,end;

    /* -------------------------------------------------- */
    /* strlen benchmark                                   */
    /* -------------------------------------------------- */

    char cstr[40] = "ClementClarke";

    DCL(fstr,40);
    SET(fstr,"ClementClarke");
    dumpvar(fstr);

  /*  char a[40];
    char b[40] = "ABCDEFGHIJK";     */

    char a[90] = "";
    char b[90] = "ABCDEFGHIJK";
    volatile char *va = a;
    volatile char *vb = b;

 //    dumpvar(b);
  
    printf("a=: %s\n",a);
    printf("b=: %s\n",b);
  
    start = clock();

    for(int i=0;i<ITER;i++)
    {
       bench_sink += strlen(fstr);
    }

    end = clock();

    printf("C strlen      : %f seconds\n",elapsed(start,end));


    start = clock();

    for(int i=0;i<ITER;i++)
    {
        volatile size_t len = LEN(fstr);
        (void)len;
    }

    end = clock();

    printf("Fast LEN      : %f seconds\n\n",elapsed(start,end));


    /* -------------------------------------------------- */
    /* strcpy benchmark                                   */
    /* -------------------------------------------------- */


    DCL(fa,40);
    DCL(fb,40);

    SET(fa,"ABCDEFxHIJKLMNOP");
    SET(fb,"ABCDEFGHIJK");

    printf("Address of a: %p\n", (void *)&a);
    printf("Address of b: %p\n", (void *)&b);
    memset(b,'b',7);
    printf("b=: %s\n",b);
    memset(a,'l',7);
    a[8] = 0;   
    b[8] = 0;   
    printf("a=: %s\n",a);
    a[8] = 0;   
    b[8] = 0;   
    printf("a=: %s\n",a);
    printf("b=: %s\n",b);
    dumpvar(fa);
    dumpvar(fb);
  
    strcpy(a,fa);
    strcpy(b,fb);
    printf("a=: %s\n",a);
    printf("b=: %s\n",b);
    printf("fa=: %s\n",fa);
    printf("fb=: %s\n",fb);
  
     
    start = clock();

    for(int i=0;i<ITER;i++)
    {
 //        strcpy(a,b);
         strcpy((char*)va,(char*)vb);
         bench_sink +=1; 
    }

    end = clock();

    printf("C strcpy      : %f seconds\n",elapsed(start,end));


    start = clock();

    for(int i=0;i<ITER;i++)
    {
         CPY(fa,fb);
         bench_sink +=1; 
    }

    end = clock();

    printf("Fast CPY      : %f seconds\n\n",elapsed(start,end));

    start = clock();

    for(int i=0;i<ITER;i++)
    {
       a[0]='\0';
       strcat(a,b);
       bench_sink +=1; 
    }


    /* -------------------------------------------------- */
    /* strcat benchmark                                   */
    /* -------------------------------------------------- */

    end = clock();

    printf("C strcat      : %f seconds\n",elapsed(start,end));


    start = clock();

    for(int i=0;i<ITER;i++)
    {
       dv_fa.cur_len = 0;
       fa[0] = '\0';
        CAT(fa,fb);
       bench_sink +=1; 
    }

    end = clock();

    printf("Fast CAT      : %f seconds\n\n",elapsed(start,end));

    /* -------------------------------------------------- */
    /* strcmp benchmark                                   */
    /* -------------------------------------------------- */

    start = clock();

    for(int i=0;i<ITER;i++)
    {
         strcmp(a,b);
         bench_sink +=1; 
    }

    end = clock();

    printf("C strcmp      : %f seconds\n",elapsed(start,end));


    start = clock();

    for(int i=0;i<ITER;i++)
    {
         CMP(fa,fb);
         bench_sink +=1; 
    }

    end = clock();

    printf("Fast CMP      : %f seconds\n\n",elapsed(start,end));


    /* -------------------------------------------------- */
    /* literal assignment                                 */
    /* -------------------------------------------------- */

    start = clock();

    for(int i=0;i<ITER;i++)
    {
         strcpy(a,"ABCDEF");
         cstr[0] = (char)(i & 7) + 'A';
         bench_sink += 1; 
    }

    end = clock();

    printf("C literal copy: %f seconds\n",elapsed(start,end));


    start = clock();

    for(int i=0;i<ITER;i++)
    {
         SET(fa,"ABCDEF");
         cstr[0] = (char)(i & 7) + 'A';
         bench_sink += 1; 
    }

    end = clock();

    printf("Fast SET      : %f seconds\n\n",elapsed(start,end));


    printf("Iterations: %d\n",ITER);
    printf("Sink value: %lu\n", bench_sink);

    return 0;
}

