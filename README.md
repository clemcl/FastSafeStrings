# FastSafeStrings

**Descriptor-based strings for C and C++ — eliminating the null-terminator scanning bottleneck.**

FastSafeStrings (FSS) replaces C's null-terminated string model with length-aware "Dope Vector" strings. Every operation knows the length of its data upfront, so nothing ever scans memory searching for `\0`. The result is faster processing, built-in bounds safety, and a direct path to CISA 2026 memory-safety compliance for existing C codebases.

The library has roots going back to 1989, and has been used on platforms ranging from IBM z/OS mainframes to Windows and Linux.

---

## At a Glance

- **Speed**: Up to **10× faster** on typical record-processing workloads (Linux/Clang), up to **18× faster** under GCC
- **Safety**: Bounds-checked by design — buffer overruns like Heartbleed are architecturally prevented
- **Portability**: Tested on Windows (GCC/Clang), Linux (GCC/Clang), and IBM z/OS (z16)
- **Incremental adoption**: Drop in alongside existing C code — no full rewrite required
- **C++ support**: Clean `fast_string<N>` template wrapper included

---

## The Problem with Null-Terminated Strings

Standard C string handling forces the CPU to scan the same memory repeatedly:

```
fgets()     → scans for '\n' to find end of input
strlen()    → scans again to find length
strcat()    → scans destination to find where to append
fprintf(%s) → scans again to find end for output
```

On every record in a loop, that is four passes over the same data. For millions of records it adds up fast.

FastSafeStrings stores the current length alongside the buffer (a "Dope Vector"). Every subsequent operation uses that length directly:

```
VB_Get()  → reads known length from record header
SET/CAT   → uses stored length, no scanning
VB_Put()  → writes known length, no scanning
```

---

## Benchmarks

### Real-World Workload: 5,000,000 Record Transformation

Both programs perform identical work: read a record, match and replace a 6-byte pattern at a fixed offset, prepend `"PROC:"`, write the result. The baseline uses competent, idiomatic C — `fgets`, `strcpy`, `strcat`, `fprintf` — not deliberately naïve code.

| Platform / Compiler       | Standard C  | FastSafeStrings | Speedup |
| :------------------------ | :---------- | :-------------- | :------ |
| Linux (Clang 21.1.8)      | 4.907s      | 0.511s          | **9.6×** |
| Linux (GCC 15.2.0)        | 9.079s      | 0.500s          | **18.2×** |
| Windows (GCC 15.2.0)      | 11.351s     | 0.845s          | **13.4×** |
| IBM z16 / z/OS            | 0.548s      | 0.139s          | **3.9×** |

The z/OS gap is smaller because IBM's `SRST` hardware instruction accelerates null-terminator searches directly in silicon. Even so, eliminating the search entirely is still nearly 4× faster.

**Why the baseline is a fair comparison**: the standard C version already uses `memcmp` at a fixed offset for the search (not `strstr`). The remaining gap comes entirely from the structural cost of `strcpy`/`strcat`/`fprintf` scanning for `\0` on every record — exactly what FSS eliminates.

### Microbenchmarks (GCC -O3, x86_64)

| Operation    | Standard C | FastSafeStrings | Speedup  |
| :----------- | :--------- | :-------------- | :------- |
| strlen       | 0.176s     | 0.007s          | ~25×     |
| strcpy       | 0.137s     | 0.044s          | ~3×      |
| strcat       | 0.216s     | 0.043s          | ~5×      |
| strcmp       | 0.049s     | 0.049s          | same     |
| literal copy | 0.040s     | 0.041s          | same     |

The `strlen` figure (25×) reflects long strings where scanning takes proportionally more time. For typical short strings the gap is smaller — but in record-processing loops the cumulative effect of repeated scans per record is what drives the real-world numbers above.

### Reproducing the Benchmarks

```bash
# Build both benchmarks (Linux/GCC example)
gcc -O3 -o bench_fgets UpdateFgets.c
gcc -O3 -o bench_vb    UpdateVB.c vb_io.c

# Generate test data (5,000,000 records)
# See /bench/gen_test_data.c

./bench_fgets
./bench_vb
```

Run each at least three times and use the median. Ensure both input files (`race_test38.txt` and `race_test.vbf`) are generated from the same source data. File cache warm-up is identical for both since the OS will have cached the data after the first pass.

---

## Quick Start (C)

```c
#include "faststr.h"

DCL(a, 40);   // Declare: 40-byte capacity, length tracked automatically
DCL(b, 40);

SET(a, "Hello ");   // Assign literal — O(1), no scanning
SET(b, "World");

CAT(a, b);          // Append — O(1), uses stored lengths

printf("%s\n", a);  // FSS strings are null-terminated for printf compatibility
```

---

## Quick Start (C++)

```cpp
#include <iostream>
#include "fast_string.hpp"

int main() {
    fast_string<256> name;
    fast_string<256> surname("Smith");

    name = "John";
    name += " ";
    name += surname;

    std::cout << name.c_str() << "\n";
    std::cout << "Length: " << name.length() << "\n";  // O(1) — no strlen
}
```

---

## Variable Blocked (VB) File I/O

FastSafeStrings includes a VB file library modelled on IBM mainframe Variable Blocked records. Each record is prefixed with its length, so reading requires no delimiter scanning.

```
Standard text file:  H e l l o \n W o r l d \n   (scan for \n to find each record)
VB file:            [5] H e l l o [5] W o r l d   (length prefix, no scanning needed)
```

VB files can be transferred to IBM z/OS and used directly as native VB datasets.

### VB API

```c
// Open
vb_handle_t *in  = VB_OpenRead("input.vbf", "");
vb_handle_t *out = VB_OpenWrite("output.vbf", 32768, "");

// Read
uint32_t len;
while (VB_Get(in, rec_buf, 512, &len) > 0) {
    dv_rec_buf.cur_len = len;   // Sync the Dope Vector
    // ... process record ...
}

// Write
VB_Put(out, work_area, dv_work_area.cur_len);

// Close
VB_Close(in);
VB_Close(out);
```

**Note**: Converting existing `.txt` files to `.vbf` is a one-time cost. A conversion utility is provided in `/tools`.

---

## Full C Macro API

| Macro                    | Purpose                            | Complexity |
| :----------------------- | :--------------------------------- | :--------- |
| `DCL(name, size)`        | Declare a string with max capacity | —          |
| `SET(dst, "literal")`    | Assign a string literal            | O(1)       |
| `CPY(dst, src)`          | Copy one FSS string to another     | O(1)       |
| `CAT(dst, src)`          | Append FSS string                  | O(1)       |
| `CATLIT(dst, "literal")` | Append a string literal            | O(1)       |
| `CATCHAR(dst, 'c')`      | Append a single character          | O(1)       |
| `CMP(a, b)`              | Compare two FSS strings            | O(n)       |
| `CMPLIT(a, "literal")`   | Compare FSS string to literal      | O(n)       |
| `LEN(x)`                 | Get current length                 | O(1)       |
| `CLEAR(x)`               | Set string to empty                | O(1)       |

All operations are bounds-checked. Writes that would exceed capacity are safely truncated — no buffer overrun.

---

## Safety and CISA 2026

Buffer overruns in null-terminated C strings are the root cause of vulnerabilities like Heartbleed. They occur because functions like `strcpy` and `strcat` have no inherent knowledge of destination capacity — they write until they hit `\0` or until they overwrite something they shouldn't.

FSS strings carry their capacity in the Dope Vector. Every write checks `cur_len + write_len ≤ max_len` before proceeding. There is no way to overflow an FSS string through the API.

This provides a pragmatic path to CISA 2026 "Secure by Design" compliance for legacy C codebases that cannot be rewritten in a memory-safe language.

---

## Installation

### Package Layout

The package is organised into three directories:

```
/source       — the library itself: faststr.h, vbx_file.h, vbx_io.c
/utilities    — VBIMPORT, VBEXPORT, VBINFO (standalone command-line tools)
/bench        — benchmark programs: UpdateFgets.c, UpdateVB.c, test data generators
```

`/source` is the only directory required to use FastSafeStrings in your own program. `/utilities` and `/bench` are optional and can be built independently.

### Method 1 — Local Include (recommended for most projects)

Copy the three files from `/source` into your project directory, and reference them with a relative path:

```c
#include "faststr.h"
```

Compile `vbx_io.c` alongside your program:

```bash
# Linux / macOS / z/OS
gcc myprog.c vbx_io.c -o myprog

# Windows (MinGW)
gcc myprog.c vbx_io.c -o myprog.exe

# Separate compilation
gcc -c vbx_io.c
gcc myprog.c vbx_io.o -o myprog
```

This is the simplest approach. It avoids any system-wide changes and makes it easy to bundle a specific tested version of FastSafeStrings with your project.

### Method 2 — System Include Directory

If you use FastSafeStrings across many projects, install the headers system-wide so they can be included without a relative path:

```bash
sudo cp faststr.h vbx_file.h /usr/local/include/
```

Then in your code:

```c
#include <faststr.h>
```

You still need to compile and link `vbx_io.c` (or a pre-built library — see below) into each program.

### Building vbx_io.c as a Library

For repeated use across many programs, compile `vbx_io.c` once into a static or shared library rather than recompiling it with every project.

**Static library (.a)**
```bash
gcc -c -O2 vbx_io.c -o vbx_io.o
ar rcs libvbx.a vbx_io.o
sudo cp libvbx.a /usr/local/lib/
sudo cp vbx_file.h /usr/local/include/

# Link against it:
gcc myprog.c -L/usr/local/lib -lvbx -o myprog
```

**Shared library (.so) — Linux**
```bash
gcc -c -fPIC -O2 vbx_io.c -o vbx_io.o
gcc -shared -o libvbx.so vbx_io.o
sudo cp libvbx.so /usr/local/lib/
sudo ldconfig
```

### Notes

1. Static linking (`.a`) is recommended unless you specifically need to update the library without recompiling every program that depends on it.
2. `faststr.h` is entirely macro-based and needs no corresponding `.c` file. Only `vbx_io.c` needs to be compiled and linked.
3. The utility programs in `/utilities` are standalone — build each individually:
   ```bash
   gcc utilityname.c vbx_io.c -o utilityname
   ```
4. `vbx_io.c` compiles cleanly on Windows, Linux, macOS, and z/OS without platform-specific patches — `O_BINARY` and the POSIX I/O headers are handled internally via compile-time guards.

---

## When to Use FastSafeStrings

FSS is a good fit when:
- You have a C codebase that processes high volumes of text or records
- You cannot migrate to Rust, C++, or another memory-safe language
- You are targeting IBM z/OS and want to reduce MSU consumption
- You need CISA 2026 memory-safety compliance without a full rewrite

It is less relevant for:
- Short, infrequent string operations where scanning overhead is negligible
- Greenfield projects where you can choose a memory-safe language from the start

---

## Platform Notes

**IBM z/OS**: The z16 `SRST` hardware instruction accelerates `strlen` and similar scans directly. FSS still provides a ~4× advantage by bypassing the search entirely. MSU reduction from lower CPU consumption translates directly to software licensing cost savings.

**GCC vs Clang**: The benchmark results show GCC's standard C implementation is significantly slower than Clang's on this workload (9.1s vs 4.9s), likely due to differences in `stdio` buffering and `fprintf` optimisation. FSS performance is consistent across both compilers (~0.5s).

---

## Limitations

- Strings must be declared with `DCL` to be managed by the API
- Raw `char *` pointers remain unsafe if used outside the API
- VB file format requires a one-time conversion from plain text
- `memmem` availability varies by platform (a portable fallback is included)

---

## History

FastSafeStrings descends from a library first developed in 1989 for IBM 370 and Intel 8086 systems, where the cost of `while (*dest++ = *src++)` was measurable in real application throughput. The Fourth Edition (2026) adds Variable Blocked file I/O, a C++ template wrapper, and brings the benchmarks up to date on modern GCC, Clang, and z/OS compilers.

---

## License

GPL v3.0

---

## Contact

Clement Clarke — clemclarke@gmail.com
