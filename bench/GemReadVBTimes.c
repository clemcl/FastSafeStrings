#include <stdio.h>
#include <time.h>
#include "vb_file.h"
#include "faststr.h"

int long long sink = 0;
 
int main() {
    vb_handle_t *in = VB_OpenRead("race_test.vbf");
    if (!in) {
       printf ("Open failed\n");
       return 1;
    }

    DCL(rec_buf, 512);
    DCL(work_area, 1024);
    uint32_t bytes_read;

    uint64_t records_processed = 0;      
         
    clock_t start = clock();
     
    uint64_t dummy_checksum = 0;
 /*    
    while (VB_Get(in, work_area, sizeof(work_area), &bytes_read) > 0) {
    dummy_checksum += work_area[0]; // Force the CPU to touch the data
    // ... your Jol Logic here ...
    }
    printf("Test Complete. Checksum: %llu\n", dummy_checksum);
*/
    // The VB_Get logic jumps to the next record using the length prefix
    while (VB_Get(in, rec_buf, 512, &bytes_read) > 0) {
        // Manually sync the Dope Vector with the read length
        dv_rec_buf.cur_len = bytes_read;

        // Fast Concatenation: No O(n) scanning involved
        SET(work_area, "PROC:");
        CAT(work_area, rec_buf);
        sink += work_area[0];     
        records_processed++;       
    }

    clock_t end = clock();
    VB_Close(in);

    printf("EOF Reached. Processed %llu records.\n", records_processed);        
     
    printf("Challenger (FSS Logic) Time: %f seconds\n", 
           (double)(end - start) / CLOCKS_PER_SEC);
    return 0;
}
