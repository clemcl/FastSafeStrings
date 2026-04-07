# Benchmarking FastSafeStrings

To replicate the 13x (Windows) and 4x (z16) speedups, compile and run the provided test suites located in the `/bench` directory.

### **Test Environment**
* **Baseline**: `UpdateFgets.c` (Standard C `fgets`/`strstr`/`fprintf`).
* **Challenger**: `UpdateVB.c` (FastSafeStrings/Jol Logic).

### **Key Metrics**
* **User CPU Time**: Measures the raw computational efficiency of the string logic.
* **System I/O Time**: Measures the efficiency of writing record-based data to disk.

Users on **z/OS** should note that while the z16 hardware instructions (`SRST`) accelerate standard C, the FastSafeStrings $O(1)$ approach still provides a nearly **4x advantage** by bypassing the search entirely.
