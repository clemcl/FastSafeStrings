#ifdef _MSC_VER
  #include <string.h>          /* Required for memcpy */
  #define __attribute__(x)
  #define inline __inline
  #define restrict __restrict
  
  /* Map GCC/Clang built-ins to Standard C for MSVC */
  #define __builtin_memcpy memcpy
#endif
