#include <stdio.h>
#include <stdlib.h>
#include "vb_file.h"

int main() {
    const int NUM_RECORDS = 5000000;
    const char *line_text = "Data_Record_Payload_Item_Number_";

    // 1. Generate the VBF1 file (Your High-Speed Format)
    vb_handle_t *vbf_out = VB_OpenWrite("race_test.vbf", 32768, VB_LEN16); //
    
    // 2. Generate the Standard Text file (with LF for fgets)
    FILE *txt_out = fopen("race_test.txt", "w");

    char buffer[128];
    for (int i = 0; i < NUM_RECORDS; i++) {
        int len = sprintf(buffer, "%s%d", line_text, i);
        
        // Write to VBF1
        VB_Put(vbf_out, buffer, len);
        
        // Write to Standard Text with Newline
        fprintf(txt_out, "%s\n", buffer);
    }

    VB_Close(vbf_out); //
    fclose(txt_out);
    
    printf("Generated 5,000,000 records in both .vbf and .txt formats.\n");
    return 0;
}
