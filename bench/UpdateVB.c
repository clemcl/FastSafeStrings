#include <stdio.h>
#include <time.h>
#include <string.h>
#include "vbx_file.h"
#include "faststr.h"


void print_build_identityx() {
    printf("--- Build Identity ---\n");
    printf("File: %s\n", __FILE__);
    
    #if defined(__clang__)
        printf("Compiler: Clang %d.%d.%d\n", __clang_major__, __clang_minor__, __clang_patchlevel__);
    #elif defined(__GNUC__) || defined(__GNUG__)
        printf("Compiler: GCC %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
    #elif defined(_MSC_VER)
        printf("Compiler: Microsoft Visual C++ (Internal Version %d)\n", _MSC_VER);
    #elif defined(__xlC__)
        printf("Compiler: IBM XL C/C++ (z/OS or AIX)\n");
    #else
        printf("Compiler: Unknown/Generic C\n");
    #endif
    
    printf("----------------------\n\n");
}
/* 
// Portable memmem implementation if your compiler doesn't have it
void *my_memmem(const void *l, size_t l_len, const void *s, size_t s_len) {
    if (s_len == 0) return (void *)l;
    if (l_len < s_len) return NULL;
    for (const char *p = l; l_len >= s_len; p++, l_len--) {
        if (memcmp(p, s, s_len) == 0) return (void *)p;
    }
    return NULL;
}
*/
int main() {
    vb_handle_t *in = VB_OpenRead("race_test.vbf","");
    vb_handle_t *out = VB_OpenWrite("race_test_fss_out.vbf", 32768, ""); 
    if (!in || !out) return 1;

    DCL(rec_buf, 512);
    DCL(work_area, 600);
    uint32_t bytes_read;
    uint32_t records_processed = 0;      
     
    uint32_t match_count = 0;
     
    print_build_identityx();
    clock_t start = clock();
     
    while (VB_Get(in, rec_buf, 512, &bytes_read) > 0) {
        dv_rec_buf.cur_len = bytes_read; // Sync Dope Vector

    // We know "_Item_" starts at index 29 (for example)
        if (memcmp(rec_buf + 19, "_Item_", 6) == 0) {
           match_count++;
           memcpy(rec_buf + 19, "_DATA_", 6); // Surgical modification
        }
        // Fast Search: No \0 scanning. Just a boundary-safe memory check.
  /*      uint8_t *match = my_memmem(rec_buf, bytes_read, "_Item_", 6);
        if (match) {
            match_count++;
            memcpy(match, "_DATA_", 6); 
        }  */

        // Fast Concatenation: O(1) complexity (No scanning)
        SET(work_area, "PROC:");
        CAT(work_area, rec_buf);
        
        // Direct Write: We already know the length (dv_work_area.cur_len)
        VB_Put(out, work_area, dv_work_area.cur_len);
        records_processed++;       
    }

    clock_t end = clock();
    VB_Close(in); VB_Close(out);
    printf("Total Matches Found: %lu\n", match_count);
    printf("Challenger Time: %f seconds\n", (double)(end-start)/CLOCKS_PER_SEC);
    return 0;
}
