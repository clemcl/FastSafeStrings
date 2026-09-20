#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "faststr.h"
#include "vbx_file.h"

#define MAX_LINE 4096
#define MAX_FIELD 512
#define ITER 50000000

int i=0;
unsigned int bytes_read;
 
static void to_upper(char *s) {
    for (; *s; s++) *s = toupper((unsigned char)*s);
}

static void to_lower(char *s) {
    for (; *s; s++) *s = tolower((unsigned char)*s);
}

static double elapsed(clock_t start, clock_t end)
{
    return (double)(end-start) / CLOCKS_PER_SEC;
}

int main(int argc, char *argv[]) {
    vb_handle_t *in = VB_OpenRead("bench_data.vb","");
    vb_handle_t *out = VB_OpenWrite("bench_data_updated.vb", 32768, ""); 
    if (!in || !out) return 1;

    FILE *fp;
    clock_t start,end;

    dcl(line,MAX_LINE);
    char field[8][MAX_FIELD];
    int  fieldlen[8];
    long count = 0;
    long active = 0, pending = 0, error_count = 0;

    dcl(output, MAX_LINE);
    DCL(temp, MAX_LINE);
    DCL(f0, MAX_FIELD);
    DCL(f1, MAX_FIELD);
    DCL(f2, MAX_FIELD);
    DCL(f3, MAX_FIELD);
    DCL(f4, MAX_FIELD);
    DCL(f5, MAX_FIELD);
    DCL(f6, MAX_FIELD);
    DCL(f7, MAX_FIELD);

 /*   if (argc < 2) {
        fprintf(stderr, "Usage: %s <datafile>\n", argv[0]);
        return 1;
    }
    fp = fopen(argv[1], "r");
    if (!fp) {
        fprintf(stderr, "Cannot open %s\n", argv[1]);
        return 1;
    }
  */

    start = clock();

   while (VB_Get(in, line, 512, &bytes_read) > 0) {
        dv_line.cur_len = bytes_read; // Sync Dope Vector

        /* remove newline */
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[--len] = '\0';

        /* parse 8 pipe-delimited fields */
        char *p = line;
        for (int i = 0; i < 8; i++) {
            char *sep = strchr(p, '|');
            if (sep) {
                size_t flen = sep - p;
                memcpy(field[i], p, flen);
                field[i][flen] = '\0';
                fieldlen[i]=flen;
                p = sep + 1;
            } else {
                size_t flen = strlen(p);
                memcpy(field[i], p, flen + 1);
                fieldlen[i]=flen;
                p += flen;
            }
        }

        /* count statuses */
        if (memcmp(field[1], "ACTIVE",6) == 0) active++;
        else if (memcmp(field[1], "PENDING",7) == 0) pending++;
        else if (memcmp(field[1], "ERROR",5) == 0) error_count++;

        /* transform: uppercase status, lowercase name */
        to_upper(field[1]);
        to_lower(field[2]);  

        /* load fields into FSS strings (one strlen each, unavoidable from C strings) */
        CPY_CSTR(f0, field[0]);
        CPY_CSTR(f1, field[1]);
        CPY_CSTR(f2, field[2]);
        CPY_CSTR(f3, field[3]);
        CPY_CSTR(f4, field[4]);
        CPY_CSTR(f5, field[5]);
        CPY_CSTR(f6, field[6]);
        CPY_CSTR(f7, field[7]);   

        SET(output,     /* for the writes */
        "REC_1|STATUS=ACTIVE|NAME=tgdctopa|VAL=12380|TAG=november|COMPOSITE=ACTIVE_november_VUZZPQK51FPKH1DN|AMT=232.66|ID=VUZZPQK51FPKH1DN|META=whiskey_whiskey"
        );

        /* build output via FSS CAT chain (O(1) appends, no destination scanning) */
        SET(output, "REC_");
        CAT(output, f0);
        CAT_LIT(output, "|STATUS=");
        CAT(output, f1);
        CAT_LIT(output, "|NAME=");
        CAT(output, f2);
        CAT_LIT(output, "|VAL=");
        CAT(output, f3);
        CAT_LIT(output, "|TAG=");
        CAT(output, f4);

        /* build a secondary composite tag via FSS */
        CPY(temp, f1);
        CATCHAR(temp, '_');
        CAT(temp, f4);
        CATCHAR(temp, '_');
        CAT(temp, f6);

        CAT_LIT(output,  "|COMPOSITE=");
        CAT(output,temp);

        CAT_LIT(output, "|AMT=");
        CAT(output, f5);
        CAT_LIT(output, "|ID=");
        CAT(output, f6);
        CAT_LIT(output, "|META=");
        CAT(output, f7);
   
        count++;
        /* length check (O(1) via dope vector, no scanning) */
        if (LEN(output) > 200) {
            CAT_LIT(output, "|LONG=true");
        }
        VB_Put(out, output, dv_output.cur_len); 

    }

 endskip:
    end = clock();

   printf("Done: %s\n", output); 
    printf("Done: %d\n", dv_output.cur_len); 
    printf("Fast CVC __FILE_     : %f seconds\n\n",elapsed(start,end));

    VB_Close(in); VB_Close(out);
    fprintf(stderr, "Processed: %ld records (ACTIVE=%ld, PENDING=%ld, ERROR=%ld)\n",
            count, active, pending, error_count);
    return 0;
}
