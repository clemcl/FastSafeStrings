#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_LINE 4096
#define MAX_FIELD 512
#define ITER 50000000

int i=0;
 

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
    FILE *fp;
    FILE *out = fopen("claude_race_test_std_out.txt", "w");    
    static char out_buf[32768];              /* the buffer setvbuf will use */
    setvbuf(out, out_buf, _IOFBF, 32768);
    clock_t start,end;
    char line[MAX_LINE];
    char field[8][MAX_FIELD];
    char output[MAX_LINE];
    char temp[MAX_LINE];
    long count = 0;
    long active = 0, pending = 0, error_count = 0;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <datafile>\n", argv[0]);
        return 1;
    }
    fp = fopen(argv[1], "r");
    if (!fp) {
        fprintf(stderr, "Cannot open %s\n", argv[1]);
        return 1;
    }

    start = clock();

    while (fgets(line, sizeof(line), fp)) {
        /* remove newline */
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[--len] = '\0';

        /* parse 8 pipe-delimited fields */
        char *p = line;
        for (int i = 0; i < 8; i++) {
            char *sep = strchr(p, '|');
            if (sep) {
                size_t flen = sep - p;
                strncpy(field[i], p, flen);
                field[i][flen] = '\0';
                p = sep + 1;
            } else {
                strcpy(field[i], p);
                p = p + strlen(p);
            }
        }

        /* count statuses */
        if (strcmp(field[1], "ACTIVE") == 0) active++;
        else if (strcmp(field[1], "PENDING") == 0) pending++;
        else if (strcmp(field[1], "ERROR") == 0) error_count++;

        /* transform: uppercase status, lowercase name */
        to_upper(field[1]);
        to_lower(field[2]);

 /*    for(int i=0;i<ITER;i++)  
     { */
        /* build output via strcat chain (the classic antipattern FSS targets) */
        strcpy(output, "REC_");
        strcat(output, field[0]);
        strcat(output, "|STATUS=");
        strcat(output, field[1]);
        strcat(output, "|NAME=");
        strcat(output, field[2]);
        strcat(output, "|VAL=");
        strcat(output, field[3]);
        strcat(output, "|TAG=");
        strcat(output, field[4]);

        /* build a secondary tag via more strcat */
        strcpy(temp, field[1]);
        strcat(temp, "_");
        strcat(temp, field[4]);
        strcat(temp, "_");
        strcat(temp, field[6]);

        strcat(output, "|COMPOSITE=");
        strcat(output, temp);

        strcat(output, "|AMT=");
        strcat(output, field[5]);
        strcat(output, "|ID=");
        strcat(output, field[6]);
        strcat(output, "|META=");
        strcat(output, field[7]);
         
        count++;
 /*   }
    goto endskip;  */

        /* length check (another strlen call on the growing output) */
        if (strlen(output) > 200) {
            strcat(output, "|LONG=true");
        }

        fprintf(out, "%s\n", output);            
    }

 endskip:
    end = clock();

    printf("Done: %s\n", output); 
    printf("Strcat and Write Update File: %f seconds\n\n",elapsed(start,end));

    fclose(fp);
    fclose(out);
    fprintf(stderr, "Processed: %ld records (ACTIVE=%ld, PENDING=%ld, ERROR=%ld)\n",
            count, active, pending, error_count);
    return 0;
}
