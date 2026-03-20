# FastSafeStrings

**Fast, safe string handling for C and C++ — without repeated scanning.**


FastSafeStrings improves performance by eliminating repeated string scans
(`strlen`, `strcat`, delimiter searches) using length-aware strings.

In real workloads, this can be several times faster than typical C string handling.

---

## 🚀 Quick Example (C)

```c
#include "faststr.h"

// Declare fixed-capacity strings (stack allocated)
DCL(a,40);
DCL(b,40);

SET(a,"Hello ");
SET(b,"World");

CAT(a,b);

printf("%s\n", a);
```

---

## 🚀 C++ Example

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
    std::cout << "Length: " << name.length() << "\n";

    return 0;
}
```

Features:

* O(1) length access
* Safe bounded operations
* Efficient append without scanning

---

## ⚡ Performance

### Real-world workload

Processing **5,000,000 records (~35 bytes each)**:

* Standard C (fgets + strcat + strlen): 1.506s
* FastSafeStrings (length-aware + VB):   0.205s

👉 **~7× faster in this workload by eliminating repeated scans**

Benchmarks compare typical idiomatic C usage, not hand-optimised memcpy-based code.

---

### Microbenchmarks (GCC -O3, x86_64)

| Operation    | C     | FastSafeStrings | Speedup |
| ------------ | ----- | --------------- | ------- |
| strlen       | 0.176 | 0.007           | ~25×    |
| strcpy       | 0.137 | 0.044           | ~3×     |
| strcat       | 0.216 | 0.043           | ~5×     |
| strcmp       | 0.049 | 0.049           | same    |
| literal copy | 0.040 | 0.041           | same    |

---

## 🔍 Why this is faster

Traditional C string handling repeatedly scans memory:

```
read → scan → scan → scan
```

FastSafeStrings avoids this:

```
read → process (no scans)
```

Key idea:
👉 **eliminate unnecessary passes over the same data**

---

## 🛡️ Safety

FastSafeStrings enforces:

* All operations are bounds-checked (length ≤ capacity)
* Strings always track their current length
* Writes are safely truncated if needed (no buffer overflow)

---

## 🚀 Key Features

* Descriptor-based strings (stored length + capacity)
* O(1) length access (no `strlen`)
* Fast append (no destination scanning)
* Zero-copy substring views
* Incremental adoption alongside existing C code

---

## ❓ Why not std::string / Rust / other libraries?

FastSafeStrings is designed for:

* Existing C codebases where changing language is not practical
* Low-level or embedded environments
* Workloads where repeated string scanning is a bottleneck

It focuses on improving C, not replacing it.

---

## 📂 Variable Blocked (VB) Files

FastSafeStrings includes support for variable-length records:

```
[length][data]
[length][data]
```

This allows:

* Reading records without delimiter scanning
* Efficient processing of structured text data

### Why this matters

Standard C text processing:

```
fgets → scan for newline  
strlen → scan again  
strcat → scan destination  
```

FastSafeStrings + VB:

```
read length → copy → append
```

👉 No scanning anywhere

---

## 🔧 API Overview

### Strings

```
DCL(name,size)  
SET(dst,"text")  
CPY(dst,src)  
CAT(dst,src)  
CMP(a,b)  
LEN(x)  
SUBSTR(dst,src,...)
```

### VB File API

```
vb_handle_t *VB_OpenRead(path);  
VB_Get(handle, buf, max_len, &len);  
VB_Put(handle, data, len);  
VB_Close(handle);
```

---

## 📈 When to Use

FastSafeStrings is useful when:

* Processing large volumes of text or records
* Performance matters
* Safety is important
* Rewriting in another language is not practical

---

## ⚠️ Limitations

* Requires `DCL` for managed strings
* Raw `char *` remains unsafe outside the API
* VB format requires preprocessing or conversion

---

## 📜 License

MIT License

---

## 🧾 Summary

FastSafeStrings combines:

* descriptor-based strings
* length-aware processing

to eliminate repeated scanning in C programs.

👉 **Fewer passes over data = faster and safer code**
