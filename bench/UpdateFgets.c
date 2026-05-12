#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#define MAX_REC 512

#include <stdio.h>

void print_build_identity() {
    printf("--- Build Identity ---\n");
    printf("File: %s\n", __FILE__);
    
    #if defined(__clang__)
        printf("Compiler: Clang %d.%d.%d\n", __clang_major__, __clang_minor__, __clang_patchlevel__);
    #elif defined(__GNUC__) || defined(__GNUG__)
        printf("Compiler: GCC %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
    #elif defined(__BORLANDC__)
        printf("Compiler: Borland %0X)\n", __BORLANDC__);
    #elif defined(_MSC_VER)
        printf("Compiler: Microsoft Visual C++ (Internal Version %d)\n", _MSC_VER);
    #elif defined(__xlC__)
        printf("Compiler: IBM XL C/C++ (z/OS or AIX)\n");
    #else
        printf("Compiler: Unknown/Generic C\n");
    #endif
    
    printf("----------------------\n\n");
}

// typedef unsigned __int64 uint64_t;
 
int main() {

    print_build_identity();

    FILE *fp = fopen("race_test38.txt", "r");
    FILE *out = fopen("race_test_std_out.txt", "w");
     
    if (!fp || !out) {
        printf("Error: Could not open files.\n");
        return 1;
    }

    char buffer[MAX_REC];
    char work_area[MAX_REC + 64];
    const char *search_for = "_Item_"; // Pattern to find
    const char *replace_with = "_DATA_"; // Pattern to write
    uint64_t records_processed = 0;      
     
    uint64_t match_count = 0; 
    clock_t start = clock();
     
    while (fgets(buffer, MAX_REC, fp)) {
        // Pass 1: Find newline (Scan)
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') buffer[len-1] = '\0';
      // We know "_Item_" starts at index 20 (for example)
        if (memcmp(buffer + 19, "_Item_", 6) == 0) {
           match_count++;
           memcpy(buffer + 19, "_DATA_", 6); // Surgical modification
        }

        // Pass 2: Search (Scan-heavy strstr)
   /*     char *match = strstr(buffer, "_Item_"); 
        if (match) {
            match_count++;
            memcpy(match, "_DATA_", 6); // Modify in place
        }  */

        // Pass 3: Concatenation (Scan to find end of "PROC:")
        strcpy(work_area, "PROC:");
        strcat(work_area, buffer); 
        
        // Pass 4: Write to disk (Scan for \0)
        fprintf(out, "%s\n", work_area);
        records_processed++;     
    }
    
    
    
    clock_t end = clock();
    printf("Total Matches Found: %llu\n", match_count);
    printf("Standard C (fgets/strstr/fprintf) Time: %f seconds\n", 
           ((double) (end - start)) / CLOCKS_PER_SEC);
           
    fclose(fp);
    fclose(out);
    return 0;
}
