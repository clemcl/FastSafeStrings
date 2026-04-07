# FastSafeStrings (Version 2.0)

FastSafeStrings (FSS) is a high-performance, memory-safe C library designed to eliminate the 50-year-old bottleneck of null-terminated string handling. By replacing $O(n)$ scanning with $O(1)$ length-aware "Dope Vector" logic, FSS delivers massive speedu

## **At a Glance**
* **Speed**: Up to **13.4x faster** on Windows (GCC 15) and **4x faster** on IBM z16 (z/OS).
* **Efficiency**: Reduces User CPU cycles by over **70%** on mainframe workloads.
* **Security**: Architecturally prevents buffer-overrun vulnerabilities like Heartbleed by eliminating null-terminator searches.
* **Compliance**: Designed for **CISA 2026 "Secure by Design"** standards.

## **The Performance Gap**
Standard C functions (`fgets`, `strlen`, `strcat`, `fprintf`) force the CPU to scan the same memory multiple times to find the `\0` terminator. FSS uses a single-pass, length-prefixed approach.

### **Benchmark: Record Transformation (5,000,000 Records)**
| Platform / Compiler | Standard C Time | FastSafeStrings Time | **Speedup** |
| :--- | :--- | :--- | :--- |
| **Windows (GCC 15.2.0)** | 11.351s | **0.845s** | **13.4x** |
| **z16 (z/OS OMVS)** | 0.5478s | **0.1391s** | **3.94x** |
| **Linux (GCC/Clang)** | 0.2926s | **0.0841s** | **3.48x** |

## **Economic & Environmental Impact**
* **MSU Reduction**: On IBM zSystems, FSS significantly lowers Million Service Units (MSUs) consumption, directly reducing software licensing costs.
* **Green Computing**: By reducing CPU cycles by up to 92%, FSS provides a scalable solution for lowering the carbon footprint of global data centers.

## **CISA 2026 & Memory Safety**
FSS provides a pragmatic path to memory safety for legacy C infrastructure. It enforces strict boundary checks through synchronized Dope Vectors, meeting CISA's 2026 requirements without the overhead of a full language rewrite.

## **License**
Distributed under the **GPL v3.0 License**.
