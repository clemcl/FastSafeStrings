/* vbexamp01.c */

/* This shows how to use the VB File sysstem using
   Standard C Strings.
    
   To see the VB File system using the Fasst safe strings, 
   see vbexamp02.c */
    
#include "vb_file.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    vb_handle_t *out;
    vb_handle_t *in;
    char buf[128];
    uint32_t len;  
    int rc;

    out = VB_OpenWrite("test.vb", 4096,2);
    if (!out) return 1;

    VB_Put(out, "Hello", 5);
    VB_Put(out, "Variable", 8);
    VB_Put(out, "Blocked IO", 10);

    VB_Close(out);

    in = VB_OpenRead("test.vb");

    while ((rc = VB_Get(in, buf, sizeof(buf), &len)) > 0) {
        buf[len] = '\0';
        printf("Record: %s\n", buf);
    }

    VB_Close(in);
    return 0;
}
