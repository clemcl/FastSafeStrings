# **FastSafeStrings**

**Safe, fast string handling for C and C++ — without the performance penalty.**

**Processing 5,000,000 records: ~7× faster than standard C by eliminating repeated string scans.**

FastSafeStrings introduces a **descriptor-based string model** into C, inspired by PL/I-style dope vectors, and is designed to work naturally with **variable-blocked (VB) file formats**.

FastSafeStrings also includes a lightweight C++ wrapper, providing the same length-aware, bounds-safe model with a modern operator-based interface.

Together, they eliminate repeated string scanning and significantly improve real-world performance.

FastSafeStrings improves performance by eliminating repeated string scans (strlen, strcat, etc.) using length-aware strings and records.

## Quick Start

   DCL(a,40);
   DCL(b,40);

   SET(a,"Hello ");
   SET(b,"World");

   CAT(a,b);

   printf("%s\n", a);
  
### C++ Example

FastSafeStrings also includes a lightweight C++ wrapper with a modern interface.

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
This provides the same benefits as the C API:

- O(1) length access  
- safe bounded operations  
- efficient append without scanning  
 
### Compared to std::string

Unlike `std::string`, FastSafeStrings:

- uses fixed-capacity buffers (no allocations)
- guarantees no buffer overruns
- avoids repeated length scanning in string operations
 
  
## **⚡ Performance**

## **🚀 Real-World Example**

Processing **5,000,000 records (~35 bytes each)**:

Standard C (fgets/strcat)        : 1.506 seconds  
VB  FastSafeStrings              : 0.205 seconds

👉 **~7× faster**

Benchmarks compare typical C usage (strcpy/strcat/strlen), not manually optimised memcpy-based code.

### **Micro benchmarks**

Operation       C        FastSafeStrings  
---------------------------------------  
strlen          0.176    0.007    (~25× faster)  
strcpy          0.137    0.044    (~3× faster)  
strcat          0.216    0.043    (~5× faster)  
strcmp          0.049    0.049    (same)  
literal copy    0.040    0.041    (same)

## **🔍 Why this is faster **

The improvement comes from eliminating repeated work:

### **Standard C**

read → scan → scan → scan

### **FastSafeStrings + VB**

read → process (no scans)

## **🚀 Key Features**

* **Memory safe by design**  
  All operations enforce bounds (len ≤ max_len)  
* **O(1) string length**  
  No strlen scans  
* **Fast append (CAT)**  
  No destination scanning (strcat replacement)  
* **Zero-copy substring views**  
  Efficient slicing without allocation  
* **Variable-blocked file support (VB/VBF)**  
  Length-prefixed records eliminate input scanning  
* **Incremental adoption**  
  Works alongside existing C code

## **📌 Example**

#include "faststr.h"

DCL(name,40);  
DCL(msg,80);

SET(name,"Clement");  
SET(msg,"Hello ");

CAT(msg,name);

printf("%s\n", msg);

## **🧠 Design Overview**

FastSafeStrings uses a **descriptor (dope vector)**:

typedef struct {  
    uint32_t cur_len;  
    uint32_t max_len;  
} vb_meta_t;

Each string has:

* data buffer  
* stored length  
* known capacity

## **📂 Variable Blocked Files (VB)**

The included VB file system stores records as:

 [length][data]  
 [length][data]  
...

Optionally grouped into blocks for efficiency.

From the implementation:

* length prefix (16-bit or 32-bit)  
* optional block buffering  
* portable C stdio backend

### **Why VB matters**

Standard C text processing:

fgets → scan for newline  
strlen → scan again  
strcat → scan destination

👉 Multiple passes over the same data

### **VB + FastSafeStrings**

read length → copy → append

👉 **No scanning anywhere**

## **💡 Key Insight**

FastSafeStrings does not try to replace libc.

Instead, it improves performance by:

**eliminating unnecessary scans**

* no repeated strlen  
* no strcat destination scan  
* no delimiter search in input

## **🛡️ Safety**

FastSafeStrings guarantees:

* no buffer overruns  
* safe truncation  
* predictable behaviour

Traditional C string handling can overflow buffers if not carefully managed.

## **🔧 API Overview**

### **Strings**

DCL(name,size)  
SET(dst,"text")  
CPY(dst,src)  
CAT(dst,src)  
CMP(a,b)  
LEN(x)  
SUBSTR(dst,src,...)

### **VB File API**

vb_handle_t *VB_OpenRead(path);  
VB_Get(handle, buf, max_len, &len);  
VB_Put(handle, data, len);  
VB_Close(handle);

## **📈 When to Use**

FastSafeStrings is useful when:

* processing large volumes of text or records  
* performance matters  
* safety is important  
* rewriting in another language is not practical

## **⚠️ Limitations**

* Requires DCL for managed strings  
* Raw char * remains unsafe outside the API  
* VB format requires preprocessing or conversion of text data
 
## ?? Documentation

- [How to use FastSafeStrings](HOWTO.md)

## **📜 License**

MIT License

## **🧾 Summary**

FastSafeStrings combines:

* **descriptor-based strings**  
* **length-aware record I/O**

to eliminate repeated scanning in C programs.

**Fewer passes over data → faster execution and safer code**

