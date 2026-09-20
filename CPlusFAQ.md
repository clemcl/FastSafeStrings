# FAQ: "C++ already has the string length, so this isn't a problem"

This is the most common pushback FastSafeStrings gets from C++ audiences, and it's worth answering carefully because it's half right — and the half that's wrong is the half that matters.

## The short answer

`std::string::size()` genuinely is O(1) — no argument there. But that only helps once you already *have* a `std::string`. Getting one from a file, a socket, or any existing `const char*` still means finding where it ends first — the standard doesn't guarantee otherwise, and whether your compiler folds that away for a literal is luck, not a language guarantee. And the moment that string crosses into a C API (which is most of them), the callee has no access to `std::string`'s internal length field at all — it's scanning, same as always.

We benchmarked this directly rather than argued it: 5–13x faster at small sizes, a real ~1.3–1.4x at large sizes, against `std::string`, once we'd found and fixed our own benchmark's bugs along the way. See [Verified Results](#verified-results) below, and the methodology in the main README.

## The longer answer

The "C++ already solves this" claim conflates two different things: `std::string` tracking its *own* length once built (true, O(1)) versus *acquiring* that length from an external source (not solved at all). Three concrete places the gap shows up:

1. **Constructing or assigning from a runtime `const char*`** — a file read, a network buffer, another function's return value — still requires an O(n) scan to find the end. The standard makes no guarantee here, and compiler folding only applies to compile-time-constant literals, not to the realistic case of a source whose length isn't known until runtime.
2. **Interop with any C API.** The callee only sees a raw pointer, with zero access to whatever length-tracking the caller was doing on its own string type.
3. **`operator+=`/`operator+` with a `const char*` right-hand side** — same reason as (1): the length of that RHS has to be found before the buffer can grow to fit it.

We measured all three directly against `fast_string<N>`, a dope-vector-style string type (length stored alongside the buffer, no terminator scanning, ever): 5–13x at small sizes, narrowing to ~1.3–1.4x at large sizes as raw copy cost comes to dominate both sides equally.

## "Just use `std::string_view`"

`string_view` doesn't solve this, it just makes the same problem visible at the call site. It's a non-owning pointer+length pair — if you already have the length (a literal, or something that already tracked it), great, that part's free. But construct one from a plain `const char*` the normal way, `std::string_view sv(ptr);`, and it calls `strlen()` internally to fill in that length, for exactly the same reason `std::string`'s constructor does. `string_view` shifts *where* the scan happens and removes an allocation — it doesn't remove the scan. The only way to skip it is to already know the length going in and pass it explicitly (`string_view(ptr, len)`), which just relocates the problem to "something else has to have tracked the length already." That's the entire premise of a dope-vector approach, not an alternative to it.

## Verified Results

Windows 10, GCC, high-resolution timer, size-scaled iteration counts. Full methodology — including two benchmarking bugs found and fixed along the way (an invariant-hoisting issue and a C++ overload-resolution trap) — is in the main [README](../README.md).

| Size (bytes) | Construct from runtime source | C-API interop | `operator+=` with `const char*` |
| :----------- | :------------------------------ | :--------------- | :--------------------------------- |
| 16    | 12.93x | ~tied | 11.33x |
| 64    | 4.68x  | ~tied | 4.98x  |
| 256   | 2.59x  | ~tied | 3.84x  |
| 1024  | 1.58x  | ~tied | 2.12x  |
| 4096  | 1.03x  | ~tied | 1.27x  |
| 16384 | 1.27x  | ~tied | 1.41x  |

Both operations show the honest shape of a real, constant-factor win: large at small sizes where fixed per-call overhead dominates, narrowing toward parity as strings get big enough for raw byte-copy cost to dominate both sides equally. Interop stays at parity throughout, exactly as predicted — once a `const char*` crosses a C API boundary, the caller's string type stops mattering.

Reproduce with `/bench/cpp_bench.cpp`.

## Why we're showing the mistakes too

Earlier passes of this exact benchmark reported speedups as high as 350x for `operator+=`. That number was wrong — not because either string type behaved unexpectedly, but because of a C++ overload-resolution trap in the test code: `fast_string<N>` provides both a literal-array constructor and a runtime `const char*` constructor, and passing a named array without an explicit cast silently selected the wrong one, measuring a fixed-size `memcpy` with no length-finding step at all rather than the intended comparison. We caught it by checking the generated assembly for real `strlen` calls, not by trusting a number that looked good.

We're including that story here on purpose. A number that survives someone actively trying to break it is worth more than one that looked clean the first time — and we'd rather you trust the smaller, real numbers above than a bigger, unexamined one.
