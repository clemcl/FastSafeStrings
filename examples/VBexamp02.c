/* vbexamp01.c */

/* This shows how to use the VB File sysstem using
   Standard C Strings.
    
   To see the VB File system using the Fasst safe strings, 
   see vbexamp02.c */
    
#include "vb_file.h"
#include "faststr.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    vb_handle_t *out;
    vb_handle_t *in;
     
    int rc;

    DCL(line1, 25);
    DCL(line2, 25);
    DCL(line3, 25);
    DCL(buf  , 256);

    SET(line1, "Hello");
    SET(line2, "Variable");
    SET(line3, "Blocked IO");
     
    out = VB_OpenWrite("test.vb", 4096,2);
    if (!out) return 1;

    PUT_REC(out,line1);
    PUT_REC(out,line2);
    PUT_REC(out,line3);

    VB_Close(out);

    in = VB_OpenRead("test.vb");

    while ((rc = GET_REC(in, buf,rc)) > 0) {
        printf("Record: %s\n", buf);
    }

    VB_Close(in);
    return 0;
}
