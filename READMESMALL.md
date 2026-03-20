Processing 5 million records: ~7× faster than standard C
by eliminating repeated string scans.
 
FastSafeStrings is designed to work with variable-blocked
(VB) records, where record lengths are known in advance,
eliminating input scanning.
 
FastSafeStrings improves performance by eliminating repeated
string scans (strlen, strcat, etc.) using length-aware
strings and records.
 
    Standard C:
    read ? scan ? scan ? scan

    FastSafeStrings:
    read ? process
 
## Quick Start

DCL(a,40);
DCL(b,40);

SET(a,"Hello ");
SET(b,"World");

CAT(a,b);

printf("%s\n", a);
 
============= 
 
Standard C (fgets/strcat)        : 1.506 s
FastSafeStrings + VB records     : 0.205 s
Approx 7 times faster.
  
 
FastSafeStrings improves performance by eliminating
repeated string scans, while providing built-in bounds safety.
 
   Operation       C        FastSafeStrings
   ---------------------------------------
   strlen          0.176    0.007    (~25× faster)
   strcpy          0.137    0.044    (~3× faster)
   strcat          0.216    0.043    (~5× faster)
   strcmp          0.049    0.049    (same)
   literal copy    0.040    0.041    (same)
