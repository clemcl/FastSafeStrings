#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "vb_file.h"

static void
usage(const char *prog)
{
    fprintf(stderr,
        "usage: %s [-s] file.vb\n"
        "  -s  scan records for statistics\n",
        prog);
    exit(1);
}

int
main(int argc, char **argv)
{
    int scan = 0;
    const char *path;
    vb_handle_t *vb;
    vb_file_header_t hdr;
    FILE *fp;
    uint8_t *buf = NULL;
    uint32_t len;
    uint64_t count = 0;
    uint64_t total = 0;
    uint32_t min = 0xFFFFFFFFu;
    uint32_t max = 0;
    int i;

    if (argc < 2)
        usage(argv[0]);

    for (i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "-s") == 0)
            scan = 1;
        else
            usage(argv[0]);
    }

    path = argv[argc - 1];

    /* Read header directly */
    fp = fopen(path, "rb");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    if (fread(&hdr, 1, sizeof hdr, fp) != sizeof hdr) {
        fprintf(stderr, "vbinfo: short read\n");
        fclose(fp);
        return 1;
    }

    fclose(fp);

    /* Validate header */
    if (hdr.magic != VB_FILE_MAGIC) {
        fprintf(stderr, "vbinfo: not a VB file\n");
        return 1;
    }

    /* Print header info */
    printf("File:        %s\n", path);
    printf("Format:      %s\n",
           hdr.lenfmt == VB_LEN32 ? "VB4" : "VB");
    printf("Blocking:    %s\n",
           hdr.block_size ? "VBB" : "VB");
    printf("Block size:  %u\n", hdr.block_size);
    printf("Header size: %u bytes\n", hdr.header_size);
    printf("Version:     %u\n\n", hdr.version);

    if (!scan)
        return 0;

    /* Open via VB API for scanning */
    vb = VB_OpenRead(path);
    if (!vb) {
        perror("VB_OpenRead");
        return 1;
    }

    buf = malloc(vb->block_size);
    if (!buf) {
        perror("malloc");
        VB_Close(vb);
        return 1;
    }

    while (VB_Get(vb, buf, vb->block_size, &len) > 0) {
        count++;
        total += len;
        if (len < min) min = len;
        if (len > max) max = len;
    }

    VB_Close(vb);
    free(buf);

    if (count == 0) {
        printf("Records:     0\n");
        return 0;
    }

    printf("Records:     %llu\n", (unsigned long long)count);
    printf("Min length:  %u\n", min);
    printf("Max length:  %u\n", max);
    printf("Avg length:  %.2f\n",
           (double)total / (double)count);

    return 0;
}
