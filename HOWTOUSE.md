# How to Use FastSafeStrings

FastSafeStrings is designed to be easy to integrate into existing C and C++ projects.

---

## ?? Quick Start (C)

```c
#include "faststr.h"

DCL(a,40);
DCL(b,40);

SET(a,"Hello ");
SET(b,"World");

CAT(a,b);

printf("%s\n", a);
```

---

## ?? Quick Start (C++)

```cpp
#include <iostream>
#include "fast_string.hpp"

int main()
{
    fast_string<256> name;
    fast_string<256> surname("Smith");

    name = "John";
    name += " ";
    name += surname;

    std::cout << name.c_str() << "\n";
}
```

---

## ?? Compilation

### Simple (recommended)

Compile your program with `vb_io.c`:

```bash
gcc myprog.c vb_io.c -o myprog
```

---

### Separate compilation

```bash
gcc -c vb_io.c
gcc myprog.c vb_io.o -o myprog
```

---

### Windows (MinGW)

```bash
gcc myprog.c vb_io.c -o myprog.exe
```

---

## ?? Using Variable Blocked (VB) Files

Open a VB file:

```c
vb_handle_t *in = VB_OpenRead("data.vb");
```

Read records:

```c
uint32_t len;

while (VB_Get(in, buffer, max_len, &len) > 0) {
    dv_buffer.cur_len = len;

    // process record
}
```

---

## ?? Core Concepts

### Descriptor-Based Strings

Each string stores:

* current length
* maximum length

This allows:

* O(1) length access
* safe operations
* no repeated scanning

---

### Key Macros (C)

```c
DCL(name,size)     // declare string
SET(dst,"text")    // assign literal
CPY(dst,src)       // copy
CAT(dst,src)       // append
LEN(x)             // length
CMP(a,b)           // compare
```

---

### C++ Interface

```cpp
s = "text";        // assign
s += other;        // append
s.length();        // get length
```

---

## ?? Notes

* Strings must be declared using `DCL`
* Operations are automatically bounded
* Data is always null-terminated (if enabled)

---

## ?? Tips

* Best performance comes when lengths are reused (e.g. loops, record processing)
* Combine VB files + FastSafeStrings for maximum benefit
* Avoid mixing raw `char*` with managed strings where possible

---

## ?? Summary

FastSafeStrings is easiest to adopt incrementally:

1. Replace string declarations with `DCL`
2. Replace `strcpy` ? `CPY`
3. Replace `strcat` ? `CAT`
4. Replace `strlen` ? `LEN`

This gives immediate safety and performance improvements.
