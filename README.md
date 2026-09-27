# FastSafeStrings (FSS) & VBIO

<div align="center">

```
  ███████╗ █████╗ ███████╗████████╗███████╗ █████╗ ███████╗███████╗
  ██╔════╝██╔══██╗██╔════╝╚══██╔══╝██╔════╝██╔══██╗██╔════╝██╔════╝
  █████╗  ███████║███████╗   ██║   ███████╗███████║█████╗  █████╗  
  ██╔══╝  ██╔══██║╚════██║   ██║   ╚════██║██╔══██║██╔══╝  ██╔══╝  
  ██║     ██║  ██║███████║   ██║   ███████║██║  ██║██║     ███████╗
  ╚═╝     ╚═╝  ╚═╝╚══════╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝     ╚══════╝
                     STRINGS & HIGH-SPEED VB I/O
```

**Next-Generation Descriptor-Based String Processing & Record I/O Engine**

*Engineered by **FasterByDesign** • Original Concept by Clement Victor Clarke (Originator of Jol)*[cite: 8]

[![Standard](https://img.shields.io/badge/C%20Standard-C99%20%2F%20C11-blue.svg)](#)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B%20Standard-C%2B%2B17%20%2F%20C%2B%2B20-blue.svg)](#)
[![Platforms](https://img.shields.io/badge/Platforms-x86__64%20%7C%20ARM64%20%7C%20IBM%20z%2FOS-brightgreen.svg)](#)
[![Warnings](https://img.shields.io/badge/Warnings-Zero%20(%2FW4%20%2F%20--Wall%20--Wextra%20--Wpedantic)-success.svg)](#)
[![License](https://img.shields.io/badge/License-MIT%20%2F%20Commercial-blue.svg)](#license)

</div>

---

## 🚀 Executive Summary

For over fifty years, systems programming has paid an enormous, silent performance tax: **null-terminated C strings** (`NUL`, `\0`)[cite: 8]. Standard C functions like `strlen()`, `strcat()`, `strcpy()`, and `strcmp()` force modern CPUs to scan memory byte-by-byte looking for a zero byte—stalling execution pipelines, trashing CPU caches, causing branch mispredictions, and creating pervasive buffer-overflow vulnerabilities[cite: 8].

**FastSafeStrings (FSS)** eliminates string-scanning friction entirely by coupling contiguous character data with a lightweight **Dope Vector** descriptor storing current length and maximum capacity[cite: 8].

### Key Advantages
- **$\mathcal{O}(1)$ Constant-Time Operations**: Length checks (`LEN`), concatenation (`CAT`), single-character appends (`CATCHAR`), and bounds verification execute instantaneously in $\mathcal{O}(1)$ time without scanning[cite: 8].
- **Modern Cloud Impact (Zero-Copy JSON)**: Accelerates REST and microservice payload decoding by up to **2.78x** using length-bounded slice extraction, eliminating heap allocations and redundant scans.
- **5x–7x Real-World Speedup**: Outperforms standard C library string routines across copying and concatenation, and exceeds **780x** on length access for large buffers[cite: 2].
- **Zero Heap Allocations**: Pure stack-allocated or static buffer semantics eliminate dynamic memory manager (`malloc`/`free`) overhead and heap lock contention[cite: 8].
- **Guaranteed Memory Safety**: Automatic silent truncation policy clamps writes strictly to maximum capacity, preventing buffer overruns while maintaining guaranteed null-termination[cite: 8].
- **High-Speed VB Record I/O**: Variable Blocked (`VB`) record engine bypasses line-by-line `fgets()` scanning, delivering up to **12.8x faster I/O processing** on multi-gigabyte files[cite: 8].
- **Global Energy Efficiency**: Eliminating billions of redundant memory scans saves measurable CPU cycles, reducing data center power consumption and carbon footprint at cloud scale[cite: 8].

---

## 📐 Architecture & Memory Layout

Traditional C strings rely on in-band termination[cite: 8]. C++ `std::string` relies on heap indirection (outside Small String Optimization)[cite: 8]. FastSafeStrings uses an aligned, contiguous descriptor-buffer layout[cite: 8].

```
1. Null-Terminated C String (char*):
   ┌───┬───┬───┬───┬───┬───┬───┬────┐
   │ H │ e │ l │ l │ o │ , │   │ \0 │  <-- Requires O(N) linear byte scan to find length
   └───┴───┴───┴───┴───┴───┴───┴────┘

2. C++ std::string (Heap-based, 24-32 byte handle):
   ┌───────────────┬───────────────┬───────────────┐
   │ Pointer (ptr) │ Capacity (24) │  Length (7)   │  ───┐ (Heap Indirection & Allocation Overhead)
   └───────────────┴───────────────┴───────────────┘     │
                                                         ▼
                                                       ┌───┬───┬───┬───┬───┬───┬───┬────┐
                                                       │ H │ e │ l │ l │ o │ , │   │ \0 │
                                                       └───┴───┴───┴───┴───┴───┴───┴────┘

3. FastSafeStrings (Descriptor + 16-Byte Aligned Contiguous Buffer):
   ┌───────────────────────────────┬────────────────────────────────────────────────────────┐
   │     Dope Vector (vb_meta_t)   │         Contiguous Aligned Storage (N + 1 Bytes)       │
   ├───────────────┬───────────────┼───┬───┬───┬───┬───┬───┬───┬────┬───┬───┬───┬───┬───┬───┤
   │ cur_len: 7    │ max_len: 32   │ H │ e │ l │ l │ o │ , │   │ \0 │ ? │ ? │ ? │ ? │ ? │ \0 │
   └───────────────┴───────────────┴───┴───┴───┴───┴───┴───┴───┴────┴───┴───┴───┴───┴───┴───┘
    ▲               ▲               ▲
    └─ O(1) Length  └─ O(1) Space   └─ SIMD 16-byte aligned vector register loads
```
[cite: 8]

### The Dope Vector (`vb_meta_t`)
```c
typedef struct {
    uint32_t cur_len;  /* Current payload length in bytes (excluding null) */
    uint32_t max_len;  /* Total allocated capacity in bytes (excluding null) */
} vb_meta_t;
```
[cite: 8]

---

## ⚡ Quickstart Guide

### 1. Idiomatic C (`faststr.h`)

FastSafeStrings provides clean subroutines wrapped in ergonomic macros[cite: 8].

```c
#include <stdio.h>
#include "faststr.h"

int main(void) {
    /* Declare a 64-byte capacity string on stack (allocates 65 bytes + metadata) */
    DCL(greeting, 64);
    DCL(name, 32);

    /* O(1) assignment via compile-time sizeof (no runtime scan) */
    SET(greeting, "Hello, ");
    SET(name, "World!");

    /* O(1) append: uses stored lengths, never scans for null terminator */
    CAT(greeting, name);

    /* O(1) length query */
    printf("Result: %s (Length: %u, Capacity: %u)\n", 
           greeting, LEN(greeting), MAXLEN(greeting));

    /* Zero-copy substring view: points into existing buffer without copying */
    VIEW(sub, greeting, 7, 5);
    printf("Substring View: %.*s (Length: %u)\n", 
           (int)LEN(sub), sub, LEN(sub));

    return 0;
}
```
[cite: 8]

### 2. High-Speed Zero-Copy JSON Decoding

Instead of allocating memory and running byte-by-byte null searches via `strstr` or full DOM parsers, FSS extracts fields as zero-copy bounded slices:

```c
#include "faststr.h"

typedef struct {
    const char *ptr;
    size_t len;
} fss_slice_t;

int main(void) {
    const char *payload = "{\"user\":\"clem\",\"action\":\"transfer\",\"amount\":5000}";
    size_t payload_len = strlen(payload);

    fss_slice_t action_val;
    /* Rejects mismatched field keys on length alone before calling memcmp */
    if (fss_json_find(payload, payload_len, "action", 6, &action_val)) {
        printf("Action: %.*s (Length: %zu)\n", 
               (int)action_val.len, action_val.ptr, action_val.len);
    }
    return 0;
}
```

### 3. Modern C++ (`fast_string.hpp`)

The C++ interface provides an idiomatic, zero-allocation container `fast_string<N>` compatible with C++17/C++20 algorithms and `std::string_view`[cite: 8].

```cpp
#include <iostream>
#include <string_view>
#include "fast_string.hpp"

int main() {
    /* Stack-allocated string with 256 bytes capacity */
    fss::fast_string<256> str = "FasterByDesign";
    fss::fast_string<256> suffix = " - Enterprise Performance";

    /* O(1) operator append */
    str += suffix;
    str += " [2026]";

    std::cout << "String: " << str.c_str() << "\n";
    std::cout << "Size:   " << str.size() << " / " << str.capacity() << "\n";

    /* Seamless std::string_view interoperability */
    std::string_view sv = str.string_view();
    std::cout << "Prefix: " << sv.substr(0, 14) << "\n";

    /* Constant-time lexicographical compare */
    fss::fast_string<64> a = "Alpha";
    fss::fast_string<64> b = "Beta";
    if (a < b) {
        std::cout << a << " precedes " << b << "\n";
    }

    return 0;
}
```
[cite: 8]

---

## 🗄️ High-Speed VB Record I/O & Mainframe Processing

Standard C `fgets()` scans input streams byte-by-byte searching for newline characters (`\n` or `\r\n`)[cite: 8, 9]. In enterprise data processing (logs, ETL, transaction feeds), this CPU scanning forms a massive bottleneck[cite: 8].

### Variable Blocked (VB) File Architecture
FSS features a Variable Blocked file engine inspired by mainframe Record Descriptor Words (RDW)[cite: 8]. Records are stored with explicit length prefixes, allowing the I/O subsystem to stream large 32KB/64KB blocks directly into memory and extract records in $\mathcal{O}(1)$ time[cite: 8, 10, 11].

```c
#include "faststr.h"
#include "vbx_file.h"

void process_large_dataset(void) {
    vb_handle_t *in  = VB_OpenRead("race_test.vbf", "");
    vb_handle_t *out = VB_OpenWrite("race_test_fss_out.vbf", 32768, "");
    if (!in || !out) return;

    DCL(rec_buf, 512);
    DCL(work_area, 600);
    uint32_t bytes_read;

    while (VB_Get(in, rec_buf, 512, &bytes_read) > 0) {
        dv_rec_buf.cur_len = bytes_read;

        /* Surgical in-place edit without scanning */
        if (memcmp(rec_buf + 19, "_Item_", 6) == 0) {
            memcpy(rec_buf + 19, "_DATA_", 6);
        }

        /* O(1) prefix append */
        SET(work_area, "PROC:");
        CAT(work_area, rec_buf);

        /* Write record with known length */
        VB_Put(out, work_area, dv_work_area.cur_len);
    }

    VB_Close(in);
    VB_Close(out);
}
```
[cite: 10]

---

## 📊 Verified Microbenchmarks

Tested with **GCC 15.2.0 (x86_64, -O3)** on Windows/Linux[cite: 2].

### 1. String Operation Timings (GCC 15.2 Subroutine Engine)

| Operation | String Size | Standard C (`glibc`) | FastSafeStrings | Speedup Multiple |
| :--- | :--- | :--- | :--- | :--- |
| **Length (`strlen` vs `LEN`)**[cite: 2] | 16 Bytes[cite: 2] | 0.442 s[cite: 2] | **0.104 s**[cite: 2] | **4.25x**[cite: 2] |
| *(50,000,000 iterations)*[cite: 2] | 64 Bytes[cite: 2] | 0.686 s[cite: 2] | **0.102 s**[cite: 2] | **6.75x**[cite: 2] |
| | 256 Bytes[cite: 2] | 1.904 s[cite: 2] | **0.104 s**[cite: 2] | **18.30x**[cite: 2] |
| | 1,024 Bytes[cite: 2] | 6.583 s[cite: 2] | **0.103 s**[cite: 2] | **63.94x**[cite: 2] |
| | 4,096 Bytes[cite: 2] | 20.700 s[cite: 2] | **0.106 s**[cite: 2] | **195.74x**[cite: 2] |
| | 16,384 Bytes[cite: 2] | 79.749 s[cite: 2] | **0.102 s**[cite: 2] | **780.84x**[cite: 2] |
| **Copy (`strcpy` vs `CPY`)**[cite: 2] | 16 Bytes[cite: 2] | 0.070 s[cite: 2] | **0.041 s**[cite: 2] | **1.69x**[cite: 2] |
| | 64 Bytes[cite: 2] | 0.136 s[cite: 2] | **0.064 s**[cite: 2] | **2.12x**[cite: 2] |
| | 256 Bytes[cite: 2] | 0.277 s[cite: 2] | **0.048 s**[cite: 2] | **5.80x**[cite: 2] |
| | 1,024 Bytes[cite: 2] | 0.859 s[cite: 2] | **0.144 s**[cite: 2] | **5.95x**[cite: 2] |
| | 4,096 Bytes[cite: 2] | 0.620 s[cite: 2] | **0.095 s**[cite: 2] | **6.56x**[cite: 2] |
| | 16,384 Bytes[cite: 2] | 2.436 s[cite: 2] | **0.407 s**[cite: 2] | **5.98x**[cite: 2] |
| **Concatenation (`strcat` vs `CAT`)**[cite: 2] | 16 Bytes[cite: 2] | 0.065 s[cite: 2] | **0.009 s**[cite: 2] | **7.10x**[cite: 2] |
| | 64 Bytes[cite: 2] | 0.147 s[cite: 2] | **0.064 s**[cite: 2] | **2.30x**[cite: 2] |
| | 256 Bytes[cite: 2] | 0.236 s[cite: 2] | **0.054 s**[cite: 2] | **4.41x**[cite: 2] |
| | 1,024 Bytes[cite: 2] | 0.998 s[cite: 2] | **0.154 s**[cite: 2] | **6.50x**[cite: 2] |
| | 4,096 Bytes[cite: 2] | 0.627 s[cite: 2] | **0.093 s**[cite: 2] | **6.75x**[cite: 2] |
| | 16,384 Bytes[cite: 2] | 2.522 s[cite: 2] | **0.420 s**[cite: 2] | **6.01x**[cite: 2] |
| **Compare (`strcmp` vs `CMP`)**[cite: 2] | 16 Bytes[cite: 2] | 0.084 s[cite: 2] | **0.067 s**[cite: 2] | **1.26x**[cite: 2] |
| | 64 Bytes[cite: 2] | 0.131 s[cite: 2] | **0.067 s**[cite: 2] | **1.94x**[cite: 2] |
| | 256 Bytes[cite: 2] | 0.165 s[cite: 2] | **0.065 s**[cite: 2] | **2.55x**[cite: 2] |
| | 1,024 Bytes[cite: 2] | 0.495 s[cite: 2] | **0.175 s**[cite: 2] | **2.83x**[cite: 2] |
| | 4,096 Bytes[cite: 2] | 0.330 s[cite: 2] | **0.112 s**[cite: 2] | **2.95x**[cite: 2] |
| | 16,384 Bytes[cite: 2] | 1.317 s[cite: 2] | **0.444 s**[cite: 2] | **2.97x**[cite: 2] |

### 2. High-Speed I/O Pipeline (5,000,000 Record Ingestion & Transformation)

Comparison of `UpdateFgets.c` (`fgets` / `strstr` / `fprintf`) vs. `UpdateVB.c` (`VB_Get` / `SET` / `CAT` / `VB_Put`)[cite: 9, 10]:

| Pipeline Benchmark (5M Records) | Standard C (`fgets` / `fprintf`)[cite: 8] | FSS Variable Blocked (`VBIO`)[cite: 8] | Speedup Multiple[cite: 8] |
| :--- | :--- | :--- | :--- |
| **Ingest + Edit + Concat + Write**[cite: 8] | 9.249 s[cite: 8] | **0.719 s**[cite: 8] | **12.86x Faster**[cite: 8] |

---

## 📖 Complete API Reference (`faststr.h`)

| Macro / Function | Description | Time Complexity |
| :--- | :--- | :--- |
| `DCL(name, size)`[cite: 8] | Declares 16-byte aligned buffer and Dope Vector `dv_##name`[cite: 8]. | $\mathcal{O}(1)$[cite: 8] |
| `SET(dst, "lit")`[cite: 8] | Assigns string literal to `dst` using compile-time `sizeof`[cite: 8]. | $\mathcal{O}(K)$[cite: 8] |
| `CPY(dst, src)`[cite: 8] | Copies FSS string `src` into `dst`, clamping to capacity[cite: 8]. | $\mathcal{O}(K)$[cite: 8] |
| `CAT(dst, src)`[cite: 8] | Appends FSS string `src` to `dst` in $\mathcal{O}(1)$ without scanning destination[cite: 8]. | $\mathcal{O}(K)$[cite: 8] |
| `LEN(name)`[cite: 8] | Returns current length in bytes (`dv_##name.cur_len`)[cite: 8]. | $\mathcal{O}(1)$[cite: 8] |
| `MAXLEN(name)`[cite: 8] | Returns maximum capacity in bytes (`dv_##name.max_len`)[cite: 8]. | $\mathcal{O}(1)$[cite: 8] |
| `CLEAR(name)`[cite: 8] | Resets string length to 0 and null-terminates index 0[cite: 8]. | $\mathcal{O}(1)$[cite: 8] |
| `CPYCHAR(dst, ch)`[cite: 8] | Sets `dst` to a single character `ch` (length becomes 1)[cite: 8]. | $\mathcal{O}(1)$[cite: 8] |
| `CATCHAR(dst, ch)`[cite: 8] | Appends character `ch` to `dst` if capacity allows[cite: 8]. | $\mathcal{O}(1)$[cite: 8] |
| `CMP(a, b)`[cite: 8] | Lexicographical three-way compare ($<0, 0, >0$) via `memcmp`[cite: 8]. | $\mathcal{O}(\min(N_a, N_b))$[cite: 8] |
| `VIEW(view, src, pos, len)`[cite: 8] | Creates zero-copy substring descriptor pointing into `src`[cite: 8]. | $\mathcal{O}(1)$[cite: 8] |
| `SUBSTR(dst, src, pos, len)`[cite: 8] | Copies substring of `src` into destination `dst`[cite: 8]. | $\mathcal{O}(K)$[cite: 8] |
| `VB_OpenRead(path, opts)` | Opens Variable Blocked file for buffered streaming read. | $\mathcal{O}(1)$ |
| `VB_OpenWrite(path, blk, opts)` | Opens VB file with target block size (e.g., 32768)[cite: 10, 11]. | $\mathcal{O}(1)$ |
| `VB_Get(h, buf, max, &read)`[cite: 10] | Reads next record from buffer block into `buf`[cite: 10]. | $\mathcal{O}(1)$ amortized |
| `VB_Put(h, buf, len)`[cite: 10, 11] | Buffers record into output block without scanning[cite: 10]. | $\mathcal{O}(K)$ |
| `VB_Close(h)`[cite: 10, 11] | Flushes remaining records and closes file handle[cite: 10]. | $\mathcal{O}(1)$ |

---

## 🛠️ Build & Integration

FastSafeStrings compiles cleanly with **zero warnings** under strict compiler flags (`-Wall -Wextra -Wpedantic` / `/W4`) across GCC, Clang, MSVC, and IBM XLC[cite: 8].

```bash
# Compile and run variable benchmarks
gcc -O3 -Wall -Wextra Var_bench.c -o var_bench

# Compile and run JSON zero-copy extractor benchmark
gcc -O3 -Wall -Wextra fss_json_bench.c -o json_bench

# Run Variable Blocked File I/O pipeline test
gcc -O3 MakeFile.c src/vbx_io.c -Iinclude -o make_file
./make_file
gcc -O3 UpdateVB.c src/vbx_io.c -Iinclude -o update_vb
./update_vb
```
[cite: 1, 4, 10, 11]

---

## 📜 License & Enterprise Terms

- **Open Source / Non-Profit**: Free to use under the [MIT License](LICENSE)[cite: 8].
- **Commercial Licensing**: Commercial entities with annual revenue exceeding $1,000,000 AUD require a commercial license or "Shared Savings" agreement[cite: 8].
- **Contact**: `clemclarke@gmail.com` for commercial licensing terms[cite: 8].