#include <stdio.h>
#include <time.h>
#include "vbx_file.h" // Ensure your corrected Reader is in here

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    vb_handle_t *r = VB_Open(argv[1], "r", 32768);
    if (!r) {
        perror("Failed to open file");
        return 1;
    }

    const char *record_ptr = NULL;  /* Pointer to the data (Const is for safety) */
    char buffer[65536];
    uint32_t count = 0;
    uint32_t last_len = 0;
    uint32_t total_bytes = 0;
    
    printf("Starting Speed Test on %s...\n", argv[1]);
    clock_t start = clock();

/* Inside main() loop */
    while (VB_GetLocate(r, &record_ptr, &last_len) == 1) {
        count++;
        
        // Performance trick: Work directly on the buffer
        uint8_t *data = (uint8_t *)record_ptr; 
        
/* Inside the VB_GetLocate while loop */
uint32_t i = 0;

/* Unroll by 8: The "Heavy Duty" Shunt Loop */
if (last_len >= 8) {
    for (; i <= last_len - 8; i += 8) {
        data[i]     = ASCII_TO_EBCDIC_037[data[i]];
        data[i + 1] = ASCII_TO_EBCDIC_037[data[i + 1]];
        data[i + 2] = ASCII_TO_EBCDIC_037[data[i + 2]];
        data[i + 3] = ASCII_TO_EBCDIC_037[data[i + 3]];
        data[i + 4] = ASCII_TO_EBCDIC_037[data[i + 4]];
        data[i + 5] = ASCII_TO_EBCDIC_037[data[i + 5]];
        data[i + 6] = ASCII_TO_EBCDIC_037[data[i + 6]];
        data[i + 7] = ASCII_TO_EBCDIC_037[data[i + 7]];
    }
}

/* Tail Loop for the remaining bytes */
for (; i < last_len; i++) {
    data[i] = ASCII_TO_EBCDIC_037[data[i]];
}
 
#if 0
/* Unroll by 4: Process 4 bytes at a time */
if (last_len >= 4) {
    for (; i <= last_len - 4; i += 4) {
        data[i]     = ASCII_TO_EBCDIC_037[data[i]];
        data[i + 1] = ASCII_TO_EBCDIC_037[data[i + 1]];
        data[i + 2] = ASCII_TO_EBCDIC_037[data[i + 2]];
        data[i + 3] = ASCII_TO_EBCDIC_037[data[i + 3]];
    }
}

/* Tail Loop: Clean up the remaining 1-3 bytes */
for (; i < last_len; i++) {
    data[i] = ASCII_TO_EBCDIC_037[data[i]];
}
        // The "Hot" Loop - Clang will likely vectorize this
#endif
        total_bytes += last_len; 
    }
 
   
    goto skip;
/* Inside main() loop */
    while (VB_GetLocate(r, &record_ptr, &last_len) == 1) {
        count++;
        
        // Performance trick: Work directly on the buffer
        uint8_t *data = (uint8_t *)record_ptr; 
        
        // The "Hot" Loop - Clang will likely vectorize this
        for (uint32_t i = 0; i < last_len; i++) {
            data[i] = ASCII_TO_EBCDIC_037[data[i]];
        }

        total_bytes += last_len; 
    }
 
   
    goto skip;
// 1. Pre-calculate values to avoid repeated struct lookups
    while (VB_GetLocate(r, &record_ptr, &last_len) == 1) {
        count++;
        // 2. ONLY do the math, NO printing, NO branches
        total_bytes += last_len; 
    }
    goto skip;
    // The Main Loop - No scanning, just jumping!
    while (VB_Get(r, buffer, sizeof(buffer),&last_len) == 1) {
        count++;
        total_bytes += last_len;
        
        // Optional: Periodic progress so you know it's alive
        if (count % 1000000 == 0) {
            printf("Processed %u Million records...\n", count / 1000000);
        }
    }
    skip:

    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    printf("\n--- Results ---\n");
    printf("Total Records: %u\n", count);
    printf("Total Data:    %.2f MB\n", (double)total_bytes / (1024 * 1024));
    printf("Elapsed Time:  %.4f seconds\n", time_spent);
// 3. Print everything at the VERY end
    printf("Final Count: %u\n", count);
    
    if (time_spent > 0) {
        printf("Throughput:    %.2f records/sec\n", (double)count / time_spent);
    }

 //   vbx_close_reader(r);
    return 0;
}
