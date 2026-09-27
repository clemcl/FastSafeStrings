#include <stdio.h>
#include <string.h>
#include "fss_json.h"

int main(void) {
    const char *json_doc = 
        "{\n"
        "  \"sender\": \"clem\",\n"
        "  \"command\": \"PROCESS_VB\",\n"
        "  \"record_count\": 5000000,\n"
        "  \"status\": \"ACTIVE\"\n"
        "}";

    size_t doc_len = strlen(json_doc);

    fss_slice_t val;

    printf("Input JSON:\n%s\n\n", json_doc);

    /* 1. Extract string value */
    if (fss_json_find(json_doc, doc_len, "command", 7, &val)) {
        printf("Found 'command': %.*s (Length: %zu)\n", 
               (int)val.len, val.ptr, val.len);

        if (fss_slice_equals(&val, "PROCESS_VB", 10)) {
            printf(" -> Command verified as PROCESS_VB\n");
        }
    }

    /* 2. Extract numeric scalar value */
    if (fss_json_find(json_doc, doc_len, "record_count", 12, &val)) {
        printf("Found 'record_count': %.*s (Length: %zu)\n", 
               (int)val.len, val.ptr, val.len);
    }

    /* 3. Non-existent key test */
    if (!fss_json_find(json_doc, doc_len, "missing_key", 11, &val)) {
        printf("Key 'missing_key' correctly reported as not found.\n");
    }

    return 0;
}