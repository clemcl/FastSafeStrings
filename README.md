# FastSafeStrings

**Descriptor-based strings for C and C++ — eliminating the null-terminator scanning bottleneck.**

FastSafeStrings (FSS) replaces C's null-terminated string model with length-aware "Dope Vector" strings. Every operation knows the length of its data upfront, so nothing ever scans memory searching for `\0`. The result is faster processing, built-in bound
s safety, and a direct path to CISA 2026 memory-safety compliance for existing C codebases.

The library has roots going back to 1989, and has been used on platforms ranging from IBM z/OS mainframes to Windows and Linux.

---

## At a Glance

- **Speed**: On record-processing workloads, 4–7x faster is a repeatable, multi-run-verified result; specific operations (length lookup) show unbounded advantage that *grows* with string size rather than a fixed multiple — see Benchmarks below for why
 that distinction matters.
- **Safety**: Bounds-checked by design — buffer overruns like Heartbleed are architecturally prevented.
- **Portability**: Tested on Windows (GCC/Clang), Linux (GCC/Clang), and IBM z/OS (z16).
- **Incremental adoption**: Drop in alongside existing C code — no full rewrite required. See "Incremental Adoption" below for a concrete migration pattern and its one real caveat.
- **C++ support**: `fast_string<N>` template wrapper included. Verified against `std::string`: 5–13x faster at small sizes, narrowing to ~1.3–1.4x at large sizes, for both construction and append — see C++ Benchmarks for the full table and how a ben
chmarking mistake along the way was caught and fixed.

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

## Benchmark Methodology & Known Pitfalls

We're including this section deliberately, because our own testing turned up several ways this exact kind of benchmark can quietly lie to you — some of which inflated our own numbers before we caught them. Anyone reproducing these results should watch f
or the same things:

- **"Trick" instructions used to defeat dead-code elimination can corrupt the very string they're testing.** A common pattern is writing a throwaway byte into the buffer each iteration so the optimizer can't prove the loop does nothing. If that byte lands
 on the null terminator, every string function called afterward reads undefined memory past the buffer — and the resulting timings are meaningless, usually in the direction of making standard C look artificially slower. Write the throwaway byte to index
 0 instead of the terminator position.
- **A loop-invariant length read can be hoisted out of the loop entirely.** If nothing in the loop body changes the length field, the compiler is free to read it once and reuse that value — timing "almost nothing" instead of the intended number of calls
. A compiler barrier (`__asm__ volatile("" ::: "memory")` on GCC/Clang) forces a fresh read each iteration.
- **An early-exit comparison given an unfair head start looks like a bigger win than it is.** If one side of a `strcmp`/length-aware-compare test gets a forced mismatch at byte 0 while the other has to scan to a difference near the end, you're measuring "
exits after 1 byte" against "scans the whole string" — not comparison throughput. Apply any such perturbation identically to both sides.
- **`clock()`'s resolution can be coarser than the thing you're timing.** A genuinely O(1) operation can finish a million-iteration loop in a few milliseconds, which is right at or below the resolution of `clock()` on some systems, producing a `0.000000s`
 reading that looks like a bug (or an infinite speedup) rather than "too fast for this timer." A higher-resolution timer (`QueryPerformanceCounter` on Windows, `clock_gettime(CLOCK_MONOTONIC, ...)` on POSIX) resolves this.
- **OS file cache state affects record-processing I/O benchmarks non-uniformly.** In our own testing, a text-format input file and a VB-format input file did not show the same cache sensitivity — the VB file's numbers dropped substantially between a col
d first run and a warm subsequent run, while the plain-text baseline stayed flat regardless. Running one format's benchmark interleaved with disk I/O from something else measures something different (contended performance) than running it in isolation (pe
ak performance) — report both, or state clearly which one you're reporting.

The `/bench` harness (`Var_bench.c` for microbenchmarks) applies all of the above. If you're benchmarking a modified copy of this library, we'd recommend checking your test against this list before trusting a surprising number in either direction.

---

## Benchmarks

### Real-World Workload: 5,000,000 Record Transformation

Both programs perform identical work: read a record, match and replace a 6-byte pattern at a fixed offset, prepend `"PROC:"`, write the result. The baseline uses competent, idiomatic C — `fgets`, `strcpy`, `strcat`, `fprintf` — not deliberately naïve
 code.

| Platform / Compiler       | Standard C  | FastSafeStrings | Speedup |
| :------------------------ | :---------- | :--------------- | :------ |
| Linux (Clang 21.1.8)      | 4.907s      | 0.511s            | ~9.6x *(single run — see note)* |
| Linux (GCC 15.2.0)        | 9.079s      | 0.500s            | ~18.2x *(single run — see note)* |
| Windows (GCC 15.2.0)      | 11.351s     | 0.845s            | ~13.4x *(single run — see note)* |
| IBM z16 / z/OS            | 0.548s      | 0.139s            | ~3.9x *(single run — see note)* |

**Note on these numbers:** these are the original single-pass measurements and have not yet been through the multi-run, isolated-vs-interleaved verification protocol described above. On a closely related program pair we tested extensively (Windows/GCC), a
 single-run comparison originally suggested ~5x, but repeated isolated runs showed a genuine range of **4x (contended with other disk I/O) to 6.2x (isolated, cache-warm)** — both real, honest numbers describing different operating conditions, neither on
e "the" number. We'd recommend re-running the four rows above under the same protocol before treating them as final, and we're doing that as ongoing work rather than asserting figures we haven't re-verified.

The z/OS gap is smaller because IBM's `SRST` hardware instruction accelerates null-terminator searches directly in silicon. Even so, eliminating the search entirely is still meaningfully faster.

**Why the baseline is a fair comparison**: the standard C version already uses `memcmp` at a fixed offset for the search (not `strstr`), uses a comparable I/O buffer size to FSS's VB layer, and both sides process byte-identical output. The remaining gap c
omes from the structural cost of `strcpy`/`strcat`/`fprintf` scanning for `\0` on every record — exactly what FSS eliminates — plus, on the VB side specifically, avoiding `fprintf`'s format-string overhead.

### Microbenchmarks (Windows 10, GCC, high-resolution timer, size-scaled iteration counts)

Rather than a single ratio per operation, here's how each one scales with string size — the shape of the curve is more informative than any individual number, and it's what actually distinguishes an O(1) operation from a constant-factor win:

| Size (bytes) | Length: C `strlen()` / FSS `LEN()` | Length speedup | Copy speedup | Concat speedup | Compare speedup |
| :----------- | :---------------------------------- | :-------------- | :------------ | :-------------- | :---------------- |
| 16    | 0.512s / 0.105s | 4.9x   | 2.36x | 1.72x | 1.57x |
| 64    | 0.771s / 0.095s | 8.1x   | 1.86x | 2.25x | 2.05x |
| 256   | 1.999s / 0.105s | 19.0x  | 5.89x | 4.86x | 2.18x |
| 1024  | 5.925s / 0.103s | 57.5x  | 6.56x | 6.90x | 2.99x |
| 4096  | 20.279s / 0.111s| 182.7x | 6.54x | 6.47x | 2.76x |
| 16384 | 80.574s / 0.110s| 730.9x | 6.07x | 5.46x | 3.05x |

Note the shape of each column: **C's `strlen()` time grows linearly with size (roughly quadrupling every time size quadruples) while FSS's `LEN()` stays flat (~0.10–0.11s) regardless of size** — that's the actual evidence for the O(1)-vs-O(n) claim, n
ot the "730.9x" figure itself, which is just what you get from dividing a constant by something that keeps growing. Copy and Concat plateau around 5.5–7x once string size is large enough for per-byte cost to dominate fixed per-call overhead — a genuin
e constant-factor win, not an unbounded one. Compare plateaus lower, around 2.5–3x.

Reproduce with `Var_bench.c` in `/bench`.

### Reproducing the Real-World Benchmark

```bash
# Build both benchmarks (Linux/GCC example)
gcc -O3 -o bench_fgets UpdateFgets.c
gcc -O3 -o bench_vb    UpdateVB.c vb_io.c

# Generate test data (5,000,000 records)
# See /bench/gen_test_data.c

./bench_fgets
./bench_vb
```

Run each **at least 5 times**, both isolated (nothing else touching disk) and interleaved with the other program, and report the range rather than a single number — see Methodology above for why.

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

## C++ Benchmarks: Does `std::string` Already Solve This?

Short answer: partially, and it's worth being precise about which part.

`std::string` does carry its length internally, so `s.length()`/`s.size()` genuinely is O(1) — on that specific point, the "the compiler/library already knows the length" claim is correct. But that's not the whole picture:

1. **Constructing or assigning a `std::string` from a `const char*` still requires finding that source's length**, and if the source isn't a compile-time-constant literal (e.g. it came from a file, a network buffer, or another function's return value — 
the realistic case), the standard library has no way to know its length without scanning for the terminator. Whether a compiler manages to fold this away for a literal known at compile time is a quality-of-implementation detail that varies by compiler and
 optimization level, not something the standard guarantees — which matches what you'd heard: the "C++ doesn't do this" claim isn't reliably true.
2. **Interop with C APIs reopens the problem.** Calling any function expecting `const char*` (including most system and library calls) means the callee has no access to `std::string`'s internal length field — if that function needs the length, it's find
ing it the old way, regardless of how the `std::string` on the caller's side was managing things internally.
3. **`operator+=` and `operator+` still have to determine the length of a `const char*` right-hand side** before they can know how much to grow the buffer, for the same reason as (1).

`/bench/cpp_bench.cpp` tests these three cases directly against `std::string`, rather than asserting the answer — each case reports actual timings so the comparison is evidence, not assumption.

### Results (Windows 10, GCC, high-resolution timer, size-scaled iteration counts)

| Size (bytes) | Construct from runtime source | C-API interop | `operator+=` with `const char*` |
| :----------- | :------------------------------ | :--------------- | :--------------------------------- |
| 16    | 12.93x | ~tied | 11.33x |
| 64    | 4.68x  | ~tied | 4.98x  |
| 256   | 2.59x  | ~tied | 3.84x  |
| 1024  | 1.58x  | ~tied | 2.12x  |
| 4096  | 1.03x  | ~tied | 1.27x  |
| 16384 | 1.27x  | ~tied | 1.41x  |

Both operations show the same honest shape: a large advantage at small sizes (fixed per-call overhead dominates), narrowing toward roughly parity as strings get large enough for raw byte-copy cost to dominate both sides equally. Interop stays at parity ac
ross every size, as predicted — once a `const char*` crosses into a C API boundary, the caller's string type stops mattering, because the callee has no access to either type's internal length bookkeeping.

**A note on how these numbers were arrived at**, because the path there is itself informative: an earlier version of this benchmark reported "speedups" as high as 350x for `operator+=`, which turned out to be wrong — not because `std::string` or `fast_s
tring` behaved unexpectedly, but because of a C++ overload-resolution trap in the test code itself. `fast_string<N>` provides both a `fast_string(const char (&lit)[M])` constructor (for compile-time literals) and a `fast_string(const char* s)` constructor
 (for runtime data). Passing a named array by value, without an explicit `(const char*)` cast, silently selects the *literal* overload — which does a fixed-size `memcpy` with no length-finding step at all, determined by the array's declared size rather 
than its actual content. That produced numbers that looked spectacular but were comparing two different operations, not the same operation done two different ways. Once every call site was cast explicitly (confirmed by checking the generated assembly for 
real `strlen` calls, not just trusting the fix), the numbers above are what's left — smaller, but real.

**This is worth knowing if you use `fast_string` yourself, not just for benchmarking it**: `fast_string<64> f(some_runtime_buffer);` where `some_runtime_buffer` is a plain `char[256]` you filled at runtime will silently hit the same trap — no compiler w
arning, no error, just a wrong length computed from the buffer's declared size instead of its actual content. Cast to `(const char*)` explicitly when constructing or assigning from a non-literal buffer until/unless this is addressed at the API level.

---

## Variable Blocked (VB) File I/O

FastSafeStrings includes a VB file library modelled on IBM mainframe Variable Blocked records. Each record is prefixed with its length, so reading requires no delimiter scanning.

```
Standard text file:  H e l l o \n W o r l d \n   (scan for \n to find each record)
VB file:            [5] H e l l o [5] W o r l d   (length prefix, no scanning needed)
```

VB files can be transferred to IBM z/OS and used directly as native VB datasets.

**Cache behavior note:** in our testing, VB-format files showed noticeably more sensitivity to OS file-cache eviction than the equivalent plain-text files — likely due to VB's larger on-disk footprint from block/record overhead, though we haven't confir
med this as the specific mechanism. If you're benchmarking VB I/O under realistic conditions (i.e. not the only thing touching disk), measure it that way rather than in isolation — see Methodology above.

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
| `CPY_CSTR(dst, src)`     | Copy from a raw, null-terminated `char*` (one `strlen`, unavoidable — this is the bridge for interop with unconverted code; see Incremental Adoption) | O(n) on `src` |
| `CAT(dst, src)`          | Append FSS string                  | O(1)       |
| `CATLIT(dst, "literal")` | Append a string literal            | O(1)       |
| `CATCHAR(dst, 'c')`      | Append a single character          | O(1)       |
| `CMP(a, b)`              | Compare two FSS strings            | O(n)       |
| `CMPLIT(a, "literal")`   | Compare FSS string to literal      | O(n)       |
| `LEN(x)`                 | Get current length                 | O(1)       |
| `CLEAR(x)`               | Set string to empty                | O(1)       |

All operations are bounds-checked. Writes that would exceed capacity are safely truncated — no buffer overrun.

---

## Incremental Adoption Without a Full Rewrite

A natural question: if an existing C codebase already allocates fixed-size buffers (`char buf[N];`) and calls `strcpy`/`strcat`/`strlen` on them, could those calls be redirected to FSS equivalents via macros — `#define strcat CAT` and similar — to get
 the speed and safety benefits without touching the call sites?

**The idea is sound, but a blind `#define` of the libc names is the wrong way to do it, for one specific reason:** FSS's O(1) guarantee depends entirely on the dope vector (the length side-record) staying in sync with the buffer's actual contents. Every o
peration that touches the buffer has to go through something that updates that side-record. If even one code path still calls the *real* `strcat` or writes to the buffer directly — which is easy to miss in a large, older codebase, especially anything to
uching the buffer through a pointer passed to a third function — the dope vector goes stale, and every subsequent FSS operation on that buffer is now working from a wrong length. That's not a slowdown; it's a correctness bug, potentially a silent one.

The safer migration pattern:

1. **Convert the declaration**, `char buf[N];` → `DCL(buf, N);` — this is a pure text substitution, easy to script.
2. **Convert calls you can find and control** — `strcpy`/`strcat`/`strlen` on that buffer — to their `CPY`/`CAT`/`LEN` equivalents.
3. **For calls into code you can't easily convert** (a third-party library, a function pointer, anything that takes a raw `char*` and does its own thing to it), use `CPY_CSTR` at the boundary to re-sync: pull the result back in through `CPY_CSTR`, which d
oes the one unavoidable `strlen` at the handoff point and re-establishes a correct dope vector from there. This confines the "unknown code touched my buffer" risk to an explicit, visible resync point instead of letting it happen silently everywhere.
4. **Don't `#define` the standard library names.** Keep `CAT`/`CPY`/`LEN` as their own names so every call site is visibly opted in, and grep for stray real `strcat`/`strcpy` calls on any `DCL`'d buffer as a mechanical safety check before shipping a conve
rted file.

Done this way, a heavily-used, long-lived C codebase genuinely can pick up meaningful speed and bounds-safety incrementally, function by function, without the cost (or risk) of a full rewrite in Rust or another memory-safe language — which is exactly th
e CISA 2026 use case this library targets. The honest caveat is that the *savings* scale with how much of the buffer's lifetime is spent inside converted code versus bouncing out to unconverted callers; a buffer that's 90% touched by third-party code and 
10% by yours will see correspondingly less benefit than one fully under your control.

---

## Safety and CISA 2026

Buffer overruns in null-terminated C strings are the root cause of vulnerabilities like Heartbleed. They occur because functions like `strcpy` and `strcat` have no inherent knowledge of destination capacity — they write until they hit `\0` or until they
 overwrite something they shouldn't.

FSS strings carry their capacity in the Dope Vector. Every write checks `cur_len + write_len ≤ max_len` before proceeding. There is no way to overflow an FSS string through the API — though see "Incremental Adoption" above for the one way that guarant
ee can be undermined in a partially-converted codebase (a stale dope vector from an unconverted call path), and how to guard against it.

This provides a pragmatic path to CISA 2026 "Secure by Design" compliance for legacy C codebases that cannot be rewritten in a memory-safe language.

---

## Installation

### Package Layout

The package is organised into three directories:

```
/source       — the library itself: faststr.h, vbx_file.h, vbx_io.c
/utilities    — VBIMPORT, VBEXPORT, VBINFO (standalone command-line tools)
/bench        — benchmark programs: Var_bench.c (C microbenchmarks), cpp_bench.cpp (C++ microbenchmarks), UpdateFgets.c, UpdateVB.c, test data generators
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

**IBM z/OS**: The z16 `SRST` hardware instruction accelerates `strlen` and similar scans directly. FSS still provides a meaningful advantage by bypassing the search entirely. MSU reduction from lower CPU consumption translates directly to software licensi
ng cost savings.

**GCC vs Clang**: Our results showed GCC's standard C implementation slower than Clang's on one workload (9.1s vs 4.9s), plausibly due to differences in `stdio` buffering and `fprintf` optimisation — but we haven't isolated that variable directly the wa
y we did for the Windows buffer-size question, so treat it as an observation rather than a confirmed cause until it's been tested the same way.

---

## Limitations

- Strings must be declared with `DCL` to be managed by the API
- Raw `char *` pointers remain unsafe if used outside the API — see "Incremental Adoption" for how to bridge this safely rather than by convention alone
- VB file format requires a one-time conversion from plain text
- `memmem` availability varies by platform (a portable fallback is included)
- **C++**: `fast_string<N>`'s array-reference literal constructor and its runtime `const char*` constructor can be selected silently based on argument type — passing a non-literal `char[]` buffer by name (rather than cast to `const char*`) will hit the 
literal overload and produce a wrong length with no warning. See "C++ Benchmarks" above for how this was found. Cast explicitly until this is addressed at the API level.

---

## History

FastSafeStrings descends from a library first developed in 1989 for IBM 370 and Intel 8086 systems, where the cost of `while (*dest++ = *src++)` was measurable in real application throughput. The Fourth Edition (2026) adds Variable Blocked file I/O, a C++
 template wrapper, and brings the benchmarks up to date on modern GCC, Clang, and z/OS compilers.

---

## License

GPL v3.0

---

## Contact

Clement Clarke — clemclarke@gmail.com
Clement Clarke — admin@Oscar-Jol.com
