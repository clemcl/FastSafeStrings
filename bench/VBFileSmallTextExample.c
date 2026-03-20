#include <stdio.h>
#include <string.h>
#include "vb_file.h"

int main(void)
{
    vb_handle_t *out;
    vb_handle_t *in;
    const char *records[] = {
        "Hello, world",
        "Variable Blocked I/O",
        "Fast and safe",
        "Goodbye"
    };
    char buffer[256];
    size_t i;
    uint32_t len;   

    /* --- write --- */
    out = VB_OpenWrite("TestSmallTextFile.vb", 4096,2);
    if (!out) {
        perror("VB_OpenWrite");
        return 1;
    }

    for (i = 0; i < sizeof(records)/sizeof(records[0]); i++) {
        VB_Put(out, records[i], strlen(records[i]));
    }

    VB_Close(out);

    /* --- read --- */
    in = VB_OpenRead("TestSmallTextFile.vb");
    if (!in) {
        perror("VB_OpenRead");
        return 1;
    }

     while (VB_Get(in, buffer, sizeof(buffer), &len) > 0) { 
        printf("Read [%ld]: %.*s\n",
               (long)len, (int)len, buffer);
    }

    printf("Read after eof[%ld]: %.*s\n",
            (long)len, (int)len, buffer);
    VB_Close(in);
    return 0;
}
