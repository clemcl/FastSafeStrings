#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "vb_file.h"

#define BUF_SIZE 65536

static void
usage(const char *prog)
{
    fprintf(stderr,
        "usage:\n"
        "  %s -t2v [-n|-0] [-b block] [-4] in.txt out.vb\n"
        "  %s -v2t [-n|-0] in.vb out.txt\n",
        prog, prog);
    exit(1);
}

int
main(int argc, char **argv)
{
    int t2v = 0, v2t = 0;
    int delim = '\n';
    uint32_t block_size = 0;
    vb_lenfmt_t lenfmt = VB_LEN16;
    int i;

    if (argc < 4)
        usage(argv[0]);

    /* parse mode */
    if (strcmp(argv[1], "-t2v") == 0)
        t2v = 1;
    else if (strcmp(argv[1], "-v2t") == 0)
        v2t = 1;
    else
        usage(argv[0]);

    /* parse options */
    for (i = 2; i < argc - 2; i++) {
        if (strcmp(argv[i], "-n") == 0)
            delim = '\n';
        else if (strcmp(argv[i], "-0") == 0)
            delim = '\0';
        else if (strcmp(argv[i], "-4") == 0)
            lenfmt = VB_LEN32;
        else if (strcmp(argv[i], "-b") == 0) {
            if (++i >= argc - 2)
                usage(argv[0]);
            block_size = (uint32_t)atoi(argv[i]);
        } else {
            usage(argv[0]);
        }
    }

    if (t2v) {
        FILE *in;
        vb_handle_t *out;
        uint8_t *buf;
        size_t len = 0;

        in = fopen(argv[argc - 2], "rb");
        if (!in) {
            perror("fopen");
            return 1;
        }

        out = VB_OpenWrite(argv[argc - 1], block_size, lenfmt);
        if (!out) {
            perror("VB_OpenWrite");
            fclose(in);
            return 1;
        }

        buf = malloc(BUF_SIZE);
        if (!buf) {
            perror("malloc");
            fclose(in);
            VB_Close(out);
            return 1;
        }

        while ((len = fread(buf, 1, BUF_SIZE, in)) > 0) {
            size_t start = 0, j;
            for (j = 0; j < len; j++) {
                if (buf[j] == delim) {
                    VB_Put(out, buf + start, (uint32_t)(j - start));
                    start = j + 1;
                }
            }
            if (start < len) {
                VB_Put(out, buf + start, (uint32_t)(len - start));
            }
        }

        free(buf);
        fclose(in);
        VB_Close(out);
        return 0;
    }

    if (v2t) {
        vb_handle_t *in;
        FILE *out;
        uint8_t *buf;
        uint32_t len;

        in = VB_OpenRead(argv[argc - 2]);
        if (!in) {
            perror("VB_OpenRead");
            return 1;
        }

        out = fopen(argv[argc - 1], "wb");
        if (!out) {
            perror("fopen");
            VB_Close(in);
            return 1;
        }

        buf = malloc(in->block_size);
        if (!buf) {
            perror("malloc");
            VB_Close(in);
            fclose(out);
            return 1;
        }

        while (VB_Get(in, buf, in->block_size, &len) > 0) {
            if (len > 0)
                fwrite(buf, 1, len, out);
            fputc(delim, out);
        }

        free(buf);
        VB_Close(in);
        fclose(out);
        return 0;
    }

    usage(argv[0]);
    return 1;
}
