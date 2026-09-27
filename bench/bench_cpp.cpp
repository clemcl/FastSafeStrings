#include <iostream>
#include <string>
#include <string_view>
#include <chrono>
#include <vector>
#include <iomanip>
#include <cstdint>

#if __has_include("include/fast_string.hpp")
    #include "include/fast_string.hpp"
#elif __has_include("fast_string.hpp")
    #include "fast_string.hpp"
#else
    #include "../include/fast_string.hpp"
#endif

/* ---------------------------------------------------------------------------
 * High-Resolution Timing & Optimization Inhibitors
 * --------------------------------------------------------------------------- */
template <typename T>
inline void escape_sink(const T& val) {
    static volatile const void* sink_p = nullptr;
    sink_p = static_cast<const void*>(&val);
    (void)sink_p;
}

static volatile uint64_t g_val_sink = 0;
inline void sink_value(uint64_t v) {
    g_val_sink += v;
}

class Timer {
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}
    void reset() { start_ = std::chrono::high_resolution_clock::now(); }
    double elapsed_ns() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::nano>(end - start_).count();
    }
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

/* ---------------------------------------------------------------------------
 * Benchmarks
 * --------------------------------------------------------------------------- */

static void bench_construction(int iterations) {
    std::cout << "\n--- 1. CONSTRUCTION BENCHMARK (" << iterations << " iterations) ---\n";
    std::cout << std::left << std::setw(15) << "Payload"
              << " | " << std::setw(18) << "std::string (ns)"
              << " | " << std::setw(20) << "fast_string (ns)"
              << " | " << std::setw(10) << "Speedup" << "\n";
    std::cout << "----------------|--------------------|----------------------|-----------\n";

    const char *short_str = "ShortLiteral";
    const char *long_str  = "A very long string that definitively exceeds typical SSO 15-byte buffers and triggers heap allocation.";

    // Short string (SSO vs fast_string<32>)
    {
        Timer t;
        for (int i = 0; i < iterations; i++) {
            std::string s(short_str);
            escape_sink(s);
        }
        double t_std = t.elapsed_ns() / iterations;

        t.reset();
        for (int i = 0; i < iterations; i++) {
            fast_string<32> s(short_str);
            escape_sink(s);
        }
        double t_fast = t.elapsed_ns() / iterations;

        std::cout << std::left << std::setw(15) << "12B (SSO)"
                  << " | " << std::right << std::setw(18) << std::fixed << std::setprecision(2) << t_std
                  << " | " << std::setw(20) << std::fixed << std::setprecision(2) << t_fast
                  << " | " << std::setw(9) << std::fixed << std::setprecision(2) << (t_std / (t_fast > 0.001 ? t_fast : 0.001)) << "x\n";
    }

    // Long string (Heap vs fast_string<256>)
    {
        Timer t;
        for (int i = 0; i < iterations; i++) {
            std::string s(long_str);
            escape_sink(s);
        }
        double t_std = t.elapsed_ns() / iterations;

        t.reset();
        for (int i = 0; i < iterations; i++) {
            fast_string<256> s(long_str);
            escape_sink(s);
        }
        double t_fast = t.elapsed_ns() / iterations;

        std::cout << std::left << std::setw(15) << "103B (Heap)"
                  << " | " << std::right << std::setw(18) << std::fixed << std::setprecision(2) << t_std
                  << " | " << std::setw(20) << std::fixed << std::setprecision(2) << t_fast
                  << " | " << std::setw(9) << std::fixed << std::setprecision(2) << (t_std / (t_fast > 0.001 ? t_fast : 0.001)) << "x\n";
    }
}

static void bench_copy(int iterations) {
    std::cout << "\n--- 2. COPY & ASSIGNMENT BENCHMARK (" << iterations << " iterations) ---\n";
    std::cout << std::left << std::setw(15) << "Payload"
              << " | " << std::setw(18) << "std::string (ns)"
              << " | " << std::setw(20) << "fast_string (ns)"
              << " | " << std::setw(10) << "Speedup" << "\n";
    std::cout << "----------------|--------------------|----------------------|-----------\n";

    const std::string std_src = "Payload for benchmarking copy performance across architectures.";
    const fast_string<128> fast_src("Payload for benchmarking copy performance across architectures.");

    std::string std_dst;
    fast_string<128> fast_dst;

    Timer t;
    for (int i = 0; i < iterations; i++) {
        std_dst = std_src;
        escape_sink(std_dst);
    }
    double t_std = t.elapsed_ns() / iterations;

    t.reset();
    for (int i = 0; i < iterations; i++) {
        fast_dst = fast_src;
        escape_sink(fast_dst);
    }
    double t_fast = t.elapsed_ns() / iterations;

    std::cout << std::left << std::setw(15) << "63 Bytes"
              << " | " << std::right << std::setw(18) << std::fixed << std::setprecision(2) << t_std
              << " | " << std::setw(20) << std::fixed << std::setprecision(2) << t_fast
              << " | " << std::setw(9) << std::fixed << std::setprecision(2) << (t_std / (t_fast > 0.001 ? t_fast : 0.001)) << "x\n";
}

static void bench_append(int iterations) {
    std::cout << "\n--- 3. APPEND & CONCATENATION BENCHMARK (" << iterations << " iterations) ---\n";
    std::cout << std::left << std::setw(15) << "Operation"
              << " | " << std::setw(18) << "std::string (ns)"
              << " | " << std::setw(20) << "fast_string (ns)"
              << " | " << std::setw(10) << "Speedup" << "\n";
    std::cout << "----------------|--------------------|----------------------|-----------\n";

    const char* chunk = "ABCDEFGH";

    Timer t;
    for (int i = 0; i < iterations; i++) {
        std::string s;
        for (int k = 0; k < 8; k++) {
            s.append(chunk);
        }
        escape_sink(s);
    }
    double t_std = t.elapsed_ns() / iterations;

    t.reset();
    for (int i = 0; i < iterations; i++) {
        fast_string<128> s;
        for (int k = 0; k < 8; k++) {
            s.append(chunk);
        }
        escape_sink(s);
    }
    double t_fast = t.elapsed_ns() / iterations;

    std::cout << std::left << std::setw(15) << "8x8B Appends"
              << " | " << std::right << std::setw(18) << std::fixed << std::setprecision(2) << t_std
              << " | " << std::setw(20) << std::fixed << std::setprecision(2) << t_fast
              << " | " << std::setw(9) << std::fixed << std::setprecision(2) << (t_std / (t_fast > 0.001 ? t_fast : 0.001)) << "x\n";
}

static void bench_comparison(int iterations) {
    std::cout << "\n--- 4. STRING EQUALITY COMPARISON BENCHMARK (" << iterations << " iterations) ---\n";
    std::cout << std::left << std::setw(15) << "Payload"
              << " | " << std::setw(18) << "std::string (ns)"
              << " | " << std::setw(20) << "fast_string (ns)"
              << " | " << std::setw(10) << "Speedup" << "\n";
    std::cout << "----------------|--------------------|----------------------|-----------\n";

    const std::string s1 = "Sample comparison string for testing benchmark execution speed.";
    const std::string s2 = "Sample comparison string for testing benchmark execution speed.";
    const fast_string<128> f1("Sample comparison string for testing benchmark execution speed.");
    const fast_string<128> f2("Sample comparison string for testing benchmark execution speed.");

    Timer t;
    for (int i = 0; i < iterations; i++) {
        bool eq = (s1 == s2);
        sink_value(eq ? 1 : 0);
    }
    double t_std = t.elapsed_ns() / iterations;

    t.reset();
    for (int i = 0; i < iterations; i++) {
        bool eq = (f1 == f2);
        sink_value(eq ? 1 : 0);
    }
    double t_fast = t.elapsed_ns() / iterations;

    std::cout << std::left << std::setw(15) << "63B Match"
              << " | " << std::right << std::setw(18) << std::fixed << std::setprecision(2) << t_std
              << " | " << std::setw(20) << std::fixed << std::setprecision(2) << t_fast
              << " | " << std::setw(9) << std::fixed << std::setprecision(2) << (t_std / (t_fast > 0.001 ? t_fast : 0.001)) << "x\n";
}

static void bench_find_substr(int iterations) {
    std::cout << "\n--- 5. FIND & SUBSTR BENCHMARK (" << iterations << " iterations) ---\n";
    std::cout << std::left << std::setw(15) << "Operation"
              << " | " << std::setw(18) << "std::string (ns)"
              << " | " << std::setw(20) << "fast_string (ns)"
              << " | " << std::setw(10) << "Speedup" << "\n";
    std::cout << "----------------|--------------------|----------------------|-----------\n";

    const std::string s = "The quick brown fox jumps over the lazy dog in modern high speed C++ code.";
    const fast_string<128> f("The quick brown fox jumps over the lazy dog in modern high speed C++ code.");

    // Find
    {
        Timer t;
        for (int i = 0; i < iterations; i++) {
            auto pos = s.find("lazy dog");
            sink_value(pos);
        }
        double t_std = t.elapsed_ns() / iterations;

        t.reset();
        for (int i = 0; i < iterations; i++) {
            auto pos = f.find("lazy dog");
            sink_value(pos);
        }
        double t_fast = t.elapsed_ns() / iterations;

        std::cout << std::left << std::setw(15) << "find(\"lazy dog\")"
                  << " | " << std::right << std::setw(18) << std::fixed << std::setprecision(2) << t_std
                  << " | " << std::setw(20) << std::fixed << std::setprecision(2) << t_fast
                  << " | " << std::setw(9) << std::fixed << std::setprecision(2) << (t_std / (t_fast > 0.001 ? t_fast : 0.001)) << "x\n";
    }

    // Substr
    {
        Timer t;
        for (int i = 0; i < iterations; i++) {
            auto sub = s.substr(10, 20);
            escape_sink(sub);
        }
        double t_std = t.elapsed_ns() / iterations;

        t.reset();
        for (int i = 0; i < iterations; i++) {
            auto sub = f.substr(10, 20);
            escape_sink(sub);
        }
        double t_fast = t.elapsed_ns() / iterations;

        std::cout << std::left << std::setw(15) << "substr(10, 20)"
                  << " | " << std::right << std::setw(18) << std::fixed << std::setprecision(2) << t_std
                  << " | " << std::setw(20) << std::fixed << std::setprecision(2) << t_fast
                  << " | " << std::setw(9) << std::fixed << std::setprecision(2) << (t_std / (t_fast > 0.001 ? t_fast : 0.001)) << "x\n";
    }
}

int main() {
    std::cout << "===============================================================================\n";
    std::cout << "  FastSafeStrings (FSS) C++ fast_string<N> Microbenchmark Suite\n";
    std::cout << "===============================================================================\n";

    const int iters = 1000000;

    bench_construction(iters);
    bench_copy(iters);
    bench_append(iters);
    bench_comparison(iters);
    bench_find_substr(iters);

    std::cout << "\nBenchmark complete. (sink checksum: " << g_val_sink << ")\n";
    return 0;
}
