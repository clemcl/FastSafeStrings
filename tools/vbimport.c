/* vbcopy.c snippet */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "vbx_file.h"

/* This program imports and converts text files to the IBM Compatible 
   Variable Blocked file format.
    
   Clement Clarke, 2026.
    
*/   
 
void print_build_identity_internal() {
    printf("--- Build Identity ---\n");
    printf("File: %s\n", __FILE__);
    printf("Date: %s\n", __DATE__);      
    
    #if defined(__64BIT__) || defined(__x86_64__) || defined(__ppc64__) || defined(_M_X64)
       #define IS_64BIT 1
       printf ("Compiler is 64 bit.\n");
    #else
       #define IS_64BIT 0
       printf ("Compiler is 32 bit.\n");
    #endif

    #if defined(__clang__)
        printf("Compiler: Clang %d.%d.%d\n", __clang_major__, __clang_minor__, __clang_patchlevel__);
    #elif defined(__GNUC__) || defined(__GNUG__)
        printf("Compiler: GCC %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
    #elif defined(__BORLANDC__)
        printf("Compiler: Borland %0X\n", __BORLANDC__);
    #elif defined(_MSC_VER)
        printf("Compiler: Microsoft Visual C++ (Internal Version %d)\n", _MSC_VER);
    #elif defined(__xlC__)
        printf("Compiler: IBM XL C/C++ (z/OS or AIX)\n");
    #else
        printf("Compiler: Unknown/Generic C\n");
    #endif
    
    printf("----------------------\n\n");
}

int main(int argc, char **argv) {
    char line[65536];
    uint32_t count = 0;
    uint32_t total_bytes = 0;
    char openoptions[40];
    
    print_build_identity_internal(); 
     

    printf("This program imports and converts text files to the\n");  
    printf("IBM Compatible Variable Blocked file format\n \n");
     
    printf("It is part of the Fast Safe String system by Clement Clarke\n\n");
     
    printf("Usage: <input text file> <output VB file> <optional conversion>\n");
    printf("       where <optional conversion> for EBCDIC is 037 or 1047\n \n");
     
    // Check if any arguments were passed (besides the program name)
    if (argc < 2) {
        printf("No arguments provided.\n\n");
    } else {
        printf("Program name: %s\n", argv[0]);
        printf("Number of arguments: %d\n", argc - 1);

        // Loop through the arguments
        for (int i = 1; i < argc; i++) {
            printf("Argument %d: %s\n", i, argv[i]);
        }
    }

    // ... arg checking ...
    if (argc < 2) {
        printf("Usage: %s <input filename> <output filename> <conversion table>\n", argv[0]);
        return 1;
    }

    strcpy(openoptions,"w");
    /* Set the table based on user input.
       FIX: this previously checked for the string "37" (no leading zero),
       but the usage message above (and every other program in this
       package) documents the codeset as "037". Typing "037" exactly as
       instructed silently produced an untranslated, plain-ASCII output
       file with no error or warning. Confirmed by testing: "037" did
       nothing, "37" (undocumented) was the only string that worked. */
    if (argc == 4) {
      if (strcmp(argv[3], "037") == 0)
          strcat(openoptions,",codeset=037");
      else if (strcmp(argv[3], "1047") == 0)
          strcat(openoptions,",codeset=1047");
      else {
          fprintf(stderr,
              "Error: conversion table must be \"037\" or \"1047\", got \"%s\"\n",
              argv[3]);
          return 1;
      }
    }

    vb_handle_t *vb = VB_Open(argv[2], openoptions, 32760-4); // max blocksize

    if (!vb) {
        perror("Failed to open output file");
        return 1;
    }
                            
    FILE *f_in = fopen(argv[1], "r");     

    if (!f_in) {
        perror("Failed to open input file");
        return 1;
    }
     

    clock_t start = clock();

    while (fgets(line, sizeof(line), f_in)) {
        size_t len = strlen(line);
        count++;
        total_bytes += len;
        
        // Strip newlines safely[cite: 1]
        while(len > 0 && (line[len-1] == '\r' || line[len-1] == '\n')) len--;
        VB_Put(vb, line, (uint32_t)len);
    }
    clock_t end = clock();
     
    // ...
    VB_Close(vb);
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    printf("\n--- Results ---\n");
    printf("Total Records: %u\n", count);
    printf("Total Data:    %.2f MB\n", (double)total_bytes / (1024 * 1024));
    printf("Elapsed Time:  %.4f seconds\n", time_spent);
    
    if (time_spent > 0) {
        printf("Throughput:    %.2f records/sec\n", (double)count / time_spent);
    }

    fclose(f_in);   /* FIX: was never closed — minor resource leak */
    return 0;       /* FIX: explicit return for portability to older compilers */
}
