# Benchmarking FastSafeStrings

To replicate the 13x (Windows) and 4x (z16) speedups, compile and run the provided test suites located in the `/bench` directory.

To see the results of two fast Input/Output routines, see AAReadme.txt.  To run the code,
run Speed01.bat and Speed02.bat.
 
For the fast internal string routines, see the table of results in AAStringResults.txt
To run the code, run FastStr.bat
 
The bat files can easily be changed to run Clang instead of GCC.
They can be changed to run on Linux, too
 

Users on **z/OS** should note that while the z16 hardware instructions (`SRST`) accelerate standard C, the FastSafeStrings $O(1)$ approach still provides a nearly **4x advantage** by bypassing the search entirely.
