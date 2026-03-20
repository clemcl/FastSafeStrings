/*
 * vbcat - dump records from a VB / VBB file
 *
 * Usage:
 *   vbcat -n | -0 | -r file.vb
 *
 * Behaviour:
 *   - Reads records sequentially
 *   - Writes payload bytes to stdout
 *   - Appends a newline after each record (for visibility)
 *
 * Notes:
 *   - Binary-safe (uses fwrite)
 *   - Does NOT interpret encoding
 *   - Intended for inspection and testing
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "vb_file.h"

#define VB_MAX_REC 65536  /* enough for VB; VB4 will realloc */

static void
usage(const char *prog)
{
    fprintf(stderr,
        "usage: %s [-n|-0|-r] file.vb\n"
        "  -n  newline delimiter (default)\n"
        "  -0  NUL delimiter\n"
        "  -r  raw (no delimiter)\n",
        prog);
    exit(1);
}

int
main(int argc, char **argv)
{
    vb_handle_t *vb;
    uint8_t *buf;
    uint32_t len;
    int delim = '\n';
    int raw = 0;
    int i;

    if (argc < 2)
        usage(argv[0]);

    for (i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            delim = '\n';
        } else if (strcmp(argv[i], "-0") == 0) {
            delim = '\0';
        } else if (strcmp(argv[i], "-r") == 0) {
            raw = 1;
        } else {
            usage(argv[0]);
        }
    }

    vb = VB_OpenRead(argv[argc - 1]);
    if (!vb) {
        perror("VB_OpenRead");
        return 1;
    }

    buf = malloc(VB_MAX_REC); //vb->max_rec_len);
    if (!buf) {
        perror("malloc");
        VB_Close(vb);
        return 1;
    }

    while (VB_Get(vb, buf, VB_MAX_REC, &len) > 0) {
        if (len > 0) {
            if (fwrite(buf, 1, len, stdout) != len) {
                perror("write");
                break;
            }
        }
        if (!raw) {
            fputc(delim, stdout);
        }
    }

    free(buf);
    VB_Close(vb);
    return 0;
}
