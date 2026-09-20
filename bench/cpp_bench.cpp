/*
   FastSafeStrings C++ Benchmark
   ------------------------------
   Tests std::string against fast_string<N> on the operations that
   actually matter for the "does C++ already solve this" question.

   std::string DOES track its own length internally (s.length() / s.size()
   are genuinely O(1) -- that part of the "C++ already knows the length"
   claim is true). What this file tests is the three places that claim
   stops being true in practice:

     1. Constructing/assigning a std::string from a const char* whose
        length is NOT known at compile time (the realistic case -- data
        that came from a file, a socket, another function's return value).
        The standard does not guarantee this avoids a scan, and whether
        a given compiler manages to fold it away for a literal is a
        quality-of-implementation detail, not a language guarantee.

     2. Interop with a C API taking const char* -- the callee has no way
        to see std::string's internal length field, so if it needs the
        length, it's finding it the old way regardless of what the
        std::string on the caller's side already knew.

     3. operator+= / operator+ with a const char* right-hand side, which
        has to determine that side's length before it can grow the
        buffer, for the same reason as (1).

   This follows the same methodology as Var_bench.c (compiler barriers
   to prevent hoisting invariant reads, no terminator-clobbering tricks,
   size-scaled iteration counts, high-resolution timer) -- see that
   file's comments and the README's "Benchmark Methodology" section for
   why each of those matters.

   NOTE: this assumes a fast_string<N> API matching the README's C++
   Quick Start (operator=, operator+=, .c_str(), .length(), constructible
   from const char*). Adjust the FSS_* calls below if your actual
   fast_string.hpp differs -- this file is a scaffold, not tested against
   your real header.

   REVISION: run_size_benchmark is now templated on capacity (CAP), and
   main() calls it once per size tier with a right-sized capacity (a
   few times the content size) instead of one large fixed capacity for
   every tier. The first version used fast_string<16385> even for the
   16-byte test -- if fast_string's internals do any work proportional
   to declared capacity rather than actual content length, that ~1000x
   oversized capacity would show up as a penalty specifically at small
   sizes. This version isolates that variable.
*/

#include <cstdio>
#include <cstring>
#include <string>
#include "fast_string.hpp"   /* adjust path as needed */

#if defined(_WIN32)
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
    static double hr_elapsed(hr_time_t start, hr_time_t end) {
        return (double)(end.QuadPart - start.QuadPart) * hr_freq_inv;
    }
#else
    #include <time.h>
    typedef struct timespec hr_time_t;
    static void hr_init(void) {}
    static hr_time_t hr_now(void) {
        hr_time_t t;
        clock_gettime(CLOCK_MONOTONIC, &t);
        return t;
    }
    static double hr_elapsed(hr_time_t start, hr_time_t end) {
        return (double)(end.tv_sec - start.tv_sec) +
               (double)(end.tv_nsec - start.tv_nsec) / 1e9;
    }
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define COMPILER_BARRIER() __asm__ volatile("" ::: "memory")
#else
    #define COMPILER_BARRIER() /* no-op on this compiler */
#endif

volatile unsigned long bench_sink = 0;
volatile const char *bench_ptr_sink = nullptr;  /* forces the compiler to
                                                     treat a pointer as
                                                     escaping, so it can't
                                                     prove a "runtime"
                                                     source is actually
                                                     compile-time-constant */

static int get_iterations(int size) {
    if (size <= 64)   return 5000000;
    if (size <= 1024) return 1000000;
    return 200000;
}

static void build_test_string(char *dst, int size, char fill) {
    for (int i = 0; i < size; i++) dst[i] = fill;
    dst[size] = '\0';
}

/* A trivial "C API" standing in for any real interop boundary --
   strlen() here represents whatever a third-party function would have
   to do to find the length of a const char* it receives, since it
   cannot see std::string's or fast_string's internal bookkeeping. */
static size_t c_api_takes_const_char_ptr(const char *s) {
    return strlen(s);
}

/* FIX: the previous version hardcoded fast_string<16385> (and <16450> for
   the append test) for EVERY size tier, including the 16-byte test. If
   fast_string's internals do any work proportional to the DECLARED
   capacity rather than strictly the actual content length, that's a
   ~1000x-oversized capacity penalizing fast_string exactly where the
   earlier results showed it losing (small sizes) and becoming
   irrelevant exactly where it started winning (large sizes) -- a
   textbook confound shape. Templating on capacity lets each size tier
   use a realistically-sized capacity (a few times the content size,
   the way you'd actually declare one in real code) instead of one
   wildly oversized capacity for all six tiers. */
template <int CAP>
void run_size_benchmark(int target_size) {
    int iterations = get_iterations(target_size);
    printf("=== BENCHMARKING SIZE: %d bytes (%d iterations) ===\n",
           target_size, iterations);

    char raw_buf[16385];
    build_test_string(raw_buf, target_size, 'A');

    hr_time_t start, end;

    /* -------------------------------------------------- */
    /* 1. CONSTRUCT/ASSIGN FROM A RUNTIME const char*      */
    /*    (source length not known at compile time --      */
    /*    bench_ptr_sink forces the compiler to treat it   */
    /*    as opaque rather than a literal it can fold)      */
    /* -------------------------------------------------- */
    bench_ptr_sink = raw_buf;

    start = hr_now();
    for (int i = 0; i < iterations; i++) {
        COMPILER_BARRIER();
        std::string s((const char *)bench_ptr_sink);
        bench_sink += s.size();
        raw_buf[0] = (char)(i & 1 ? 'A' : 'B');
    }
    end = hr_now();
    double cpp_construct_time = hr_elapsed(start, end);

    start = hr_now();
    for (int i = 0; i < iterations; i++) {
        COMPILER_BARRIER();
        fast_string<CAP> f((const char *)bench_ptr_sink);
        bench_sink += f.length();
        raw_buf[0] = (char)(i & 1 ? 'A' : 'B');
    }
    end = hr_now();
    double fss_construct_time = hr_elapsed(start, end);

    printf("  Construct from runtime const char* -> std::string: %.6f s | "
           "fast_string: %.6f s | Speedup: %.2fx\n",
           cpp_construct_time, fss_construct_time,
           fss_construct_time > 0 ? cpp_construct_time / fss_construct_time : 0);

    /* -------------------------------------------------- */
    /* 2. C API INTEROP: pass an already-built string      */
    /*    into a function taking const char*.               */
    /*    The point here: even though both s and f already  */
    /*    know their own length internally, the callee      */
    /*    doesn't have access to that -- it's finding the   */
    /*    length itself, same as it would for a raw buffer. */
    /* -------------------------------------------------- */
    std::string cpp_str(raw_buf);
    /* FIX: passing raw_buf directly (as a named array) here selected
       fast_string's array-reference "literal" constructor template
       (fast_string(const char (&)[M])) instead of the intended runtime
       fast_string(const char*) constructor -- M gets deduced as
       sizeof(raw_buf), NOT target_size, and that overload does a
       fixed-size memcpy with no strlen() at all. Casting forces the
       correct runtime overload, matching what Test 1 already does via
       bench_ptr_sink. */
    fast_string<CAP> fss_str((const char *)raw_buf);

    start = hr_now();
    for (int i = 0; i < iterations; i++) {
        COMPILER_BARRIER();
        bench_sink += c_api_takes_const_char_ptr(cpp_str.c_str());
    }
    end = hr_now();
    double cpp_interop_time = hr_elapsed(start, end);

    start = hr_now();
    for (int i = 0; i < iterations; i++) {
        COMPILER_BARRIER();
        bench_sink += c_api_takes_const_char_ptr(fss_str.c_str());
    }
    end = hr_now();
    double fss_interop_time = hr_elapsed(start, end);

    printf("  C-API interop (both sides call the same C function) -> "
           "std::string: %.6f s | fast_string: %.6f s | "
           "(expected ~same -- the cost lives in the callee, not the "
           "caller's string type)\n",
           cpp_interop_time, fss_interop_time);

    /* -------------------------------------------------- */
    /* 3. operator+= WITH A const char* RIGHT-HAND SIDE     */
    /* -------------------------------------------------- */
    char suffix_buf[64];
    build_test_string(suffix_buf, 16, 'X');
    bench_ptr_sink = suffix_buf;

    /* FIX: unlike tests 1 and 2, this test never proved to the compiler
       that raw_buf/suffix_buf's content actually varies across
       iterations -- nothing wrote to them after the initial setup.
       Combined with target_size now being a compile-time constant
       (via the explicit template calls below) and CAP being small
       enough for the optimizer to trace all the way through, this let
       the compiler fold large parts of the loop to a constant despite
       COMPILER_BARRIER() -- the barrier stops caching a previously-read
       VALUE across it, but doesn't stop the compiler independently
       proving via whole-program analysis that nothing ever writes this
       memory at all, which is a different (and here, sufficient) way
       to justify folding. Perturbing a byte of each buffer after use,
       the same way test 1 already does with raw_buf, closes that gap
       by making the "never written" premise false. */
    start = hr_now();
    for (int i = 0; i < iterations; i++) {
        COMPILER_BARRIER();
        std::string s(raw_buf);
        s += (const char *)bench_ptr_sink;
        bench_sink += s.size();
        raw_buf[0] = (char)(i & 1 ? 'A' : 'B');
        suffix_buf[0] = (char)(i & 1 ? 'X' : 'Y');
    }
    end = hr_now();
    double cpp_append_time = hr_elapsed(start, end);

    start = hr_now();
    for (int i = 0; i < iterations; i++) {
        COMPILER_BARRIER();
        /* FIX: same overload trap as the interop test above -- raw_buf
           passed bare here previously hit the array-literal constructor
           (fixed-size memcpy, no strlen, length determined by CAP/array
           size rather than target_size), not the runtime constructor
           this test intends to measure. */
        fast_string<CAP> f((const char *)raw_buf);
        f += (const char *)bench_ptr_sink;
        bench_sink += f.length();
        raw_buf[0] = (char)(i & 1 ? 'A' : 'B');
        suffix_buf[0] = (char)(i & 1 ? 'X' : 'Y');
    }
    end = hr_now();
    double fss_append_time = hr_elapsed(start, end);

    printf("  operator+= with const char* RHS -> std::string: %.6f s | "
           "fast_string: %.6f s | Speedup: %.2fx\n\n",
           cpp_append_time, fss_append_time,
           fss_append_time > 0 ? cpp_append_time / fss_append_time : 0);
}

int main(void) {
    hr_init();
    printf("FastSafeStrings C++ Benchmark\n");
    printf("==============================\n\n");
    printf("Testing the specific claim: 'std::string already knows the "
           "length, so this isn't a problem in C++.'\n"
           "True for .size()/.length() on an existing std::string. The "
           "three tests below check the cases where it stops being true:\n"
           "construction from a runtime source, interop with C APIs, and "
           "concatenation with a const char*.\n\n");

    /* FIX: capacity must be a compile-time constant per fast_string<N>
       instantiation, so each tier gets its own explicit call instead of
       a runtime loop. Capacities below are ~2-4x the content size --
       realistic headroom, the way you'd actually size a buffer in real
       code -- rather than one capacity oversized by ~1000x for the
       small tiers. */
    run_size_benchmark<64>(16);
    run_size_benchmark<128>(64);
    run_size_benchmark<512>(256);
    run_size_benchmark<2048>(1024);
    run_size_benchmark<8192>(4096);
    run_size_benchmark<32768>(16384);

    printf("Sink verification value: %lu\n", bench_sink);
    return 0;
}