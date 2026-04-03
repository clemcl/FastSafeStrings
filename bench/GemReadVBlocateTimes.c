#include <stdio.h>
#include <time.h>
#include "vb_file.h"
#include "faststr.h"

/* LOCATE_REC: The C equivalent of a QSAM GET Locate */
#define LOCATE_REC(vb, fss_name) do { \
    const char *_p; \
    uint32_t _l; \
    if (VB_GetLocate(vb, &_p, &_l) > 0) { \
        fs_##fss_name.data = (char *)_p; \
        dv_##fss_name.cur_len = _l; \
    } \
} while(0)
 
int long long sink = 0;
 
int main() {
    vb_handle_t *in = VB_OpenRead("race_test.vbf");
    if (!in) {
       printf ("Open failed\n");
       return 1;
    }

  // Declare a "View" record (no buffer needed, just the metadata)
    DCL(rec_view, 0);
    const char *p_loc;
    uint32_t l_loc;
    uint64_t records_processed = 0;

    DCL(rec_buf, 512);
    DCL(work_area, 40);
    uint32_t bytes_read;

    clock_t start = clock();
     
    uint64_t dummy_checksum = 0;
 /*    
    while (VB_Get(in, work_area, sizeof(work_area), &bytes_read) > 0) {
    dummy_checksum += work_area[0]; // Force the CPU to touch the data
    // ... your Jol Logic here ...
    }
    printf("Test Complete. Checksum: %llu\n", dummy_checksum);
*/
// The "QSAM GET" Loop
while (VB_GetLocate(in, &p_loc, &l_loc) > 0) {
    
    // Update the descriptor pointer (The "Pointer Swap")
    fs_rec_view.data = (char *)p_loc;
    dv_rec_view.cur_len = l_loc;

    // --- YOUR LOGIC HERE ---
    dummy_checksum += fs_rec_view.data[0]; 
    records_processed++;
}

printf("EOF Reached. Processed %llu records.\n", records_processed);
 
  goto eof;
 
while (1) {
    // 1. Point rec_view directly into the I/O buffer
    LOCATE_REC(in, rec_view);
    
    // 2. Process it immediately (e.g., Checksum or Copy)
    dummy_checksum += rec_view[0]; 
    
    // If you need to move it to a permanent home later:
    // CPY(permanent_rec, rec_view); 
}
 
    // The VB_Get logic jumps to the next record using the length prefix
    while (VB_Get(in, rec_buf, 512, &bytes_read) > 0) {
        // Manually sync the Dope Vector with the read length
        dv_rec_buf.cur_len = bytes_read;

        // Fast Concatenation: No O(n) scanning involved
        SET(work_area, "PROC:");
        CAT(work_area, rec_buf);
        sink += work_area[0];     
    }
eof:
    clock_t end = clock();
    VB_Close(in);

    printf("Challenger (Jol Logic) Time: %f seconds\n", 
           (double)(end - start) / CLOCKS_PER_SEC);
    return 0;
}
