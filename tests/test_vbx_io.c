#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#if __has_include("include/vbx_file.h")
    #include "include/vbx_file.h"
#elif __has_include("vbx_file.h")
    #include "vbx_file.h"
#else
    #include "../include/vbx_file.h"
#endif

#if __has_include("include/faststr.h")
    #include "include/faststr.h"
#elif __has_include("faststr.h")
    #include "faststr.h"
#else
    #include "../include/faststr.h"
#endif

/* ---------------------------------------------------------------------------
 * Test Harness Helpers
 * --------------------------------------------------------------------------- */
static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_SUITE_START(name) \
    printf("\n========================================\n"); \
    printf("  RUNNING TEST SUITE: %s\n", name); \
    printf("========================================\n")

#define TEST_CASE_START(name) \
    printf("  [TEST] %s ... ", name)

#define ASSERT_TRUE(expr) do { \
    g_tests_run++; \
    if (!(expr)) { \
        printf("\n    FAILED at %s:%d: Assertion failed: (%s)\n", __FILE__, __LINE__, #expr); \
        g_tests_failed++; \
    } else { \
        g_tests_passed++; \
    } \
} while(0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))

#define ASSERT_EQ_INT(actual, expected) do { \
    g_tests_run++; \
    int _a = (int)(actual); \
    int _e = (int)(expected); \
    if (_a != _e) { \
        printf("\n    FAILED at %s:%d: Expected %d, got %d (expr: %s)\n", \
               __FILE__, __LINE__, _e, _a, #actual); \
        g_tests_failed++; \
    } else { \
        g_tests_passed++; \
    } \
} while(0)

#define ASSERT_EQ_UINT(actual, expected) do { \
    g_tests_run++; \
    uint32_t _a = (uint32_t)(actual); \
    uint32_t _e = (uint32_t)(expected); \
    if (_a != _e) { \
        printf("\n    FAILED at %s:%d: Expected %u, got %u (expr: %s)\n", \
               __FILE__, __LINE__, _e, _a, #actual); \
        g_tests_failed++; \
    } else { \
        g_tests_passed++; \
    } \
} while(0)

#define ASSERT_STR_EQ(actual, expected) do { \
    g_tests_run++; \
    const char *_act = (const char *)(actual); \
    const char *_exp = (const char *)(expected); \
    if (strcmp(_act, _exp) != 0) { \
        printf("\n    FAILED at %s:%d: Expected \"%s\", got \"%s\" (expr: %s)\n", \
               __FILE__, __LINE__, _exp, _act, #actual); \
        g_tests_failed++; \
    } else { \
        g_tests_passed++; \
    } \
} while(0)

#define ASSERT_MEM_EQ(actual, expected, len) do { \
    g_tests_run++; \
    if (memcmp((actual), (expected), (len)) != 0) { \
        printf("\n    FAILED at %s:%d: Memory mismatch of size %u\n", \
               __FILE__, __LINE__, (unsigned)(len)); \
        g_tests_failed++; \
    } else { \
        g_tests_passed++; \
    } \
} while(0)

/* ---------------------------------------------------------------------------
 * Test Cases
 * --------------------------------------------------------------------------- */

static const char *TEST_FILE_RAW = "temp_test_raw.vbf";
static const char *TEST_FILE_037 = "temp_test_037.vbf";
static const char *TEST_FILE_1047 = "temp_test_1047.vbf";

static void cleanup_temp_files(void) {
    remove(TEST_FILE_RAW);
    remove(TEST_FILE_037);
    remove(TEST_FILE_1047);
}

static void test_write_read_basic(void) {
    TEST_CASE_START("VB Basic Record Write and Read (VB_Put, VB_Get)");
    int initial_failures = g_tests_failed;

    const char *records[] = {
        "RECORD-001: Short header",
        "RECORD-002: A slightly longer record containing standard payload data.",
        "RECORD-003: Third line with numbers 1234567890.",
        "RECORD-004: End of batch."
    };
    const uint32_t num_records = (uint32_t)(sizeof(records) / sizeof(records[0]));

    /* 1. Write */
    vb_handle_t *w = VB_OpenWrite(TEST_FILE_RAW, 512, "");
    ASSERT_TRUE(w != NULL);
    if (w) {
        for (uint32_t i = 0; i < num_records; i++) {
            int rc = VB_Put(w, records[i], (uint32_t)strlen(records[i]));
            ASSERT_EQ_INT(rc, 1);
        }
        VB_Close(w);
    }

    /* 2. Read */
    vb_handle_t *r = VB_OpenRead(TEST_FILE_RAW, "");
    ASSERT_TRUE(r != NULL);
    if (r) {
        char buf[256];
        uint32_t out_len = 0;
        for (uint32_t i = 0; i < num_records; i++) {
            int rc = VB_Get(r, buf, (uint32_t)sizeof(buf), &out_len);
            ASSERT_EQ_INT(rc, 1);
            ASSERT_EQ_UINT(out_len, (uint32_t)strlen(records[i]));
            ASSERT_EQ_INT(buf[out_len], '\0');
            ASSERT_STR_EQ(buf, records[i]);
        }

        /* Test EOF */
        int rc_eof = VB_Get(r, buf, (uint32_t)sizeof(buf), &out_len);
        ASSERT_EQ_INT(rc_eof, 0);

        /* Consecutive EOF calls */
        rc_eof = VB_Get(r, buf, (uint32_t)sizeof(buf), &out_len);
        ASSERT_EQ_INT(rc_eof, 0);

        VB_Close(r);
    }

    if (g_tests_failed == initial_failures) printf("PASS\n");
}

static void test_fss_macros_integration(void) {
    TEST_CASE_START("FSS Macros Integration (PUT_REC, GET_REC)");
    int initial_failures = g_tests_failed;

    DCL(out_rec, 64);
    DCL(in_rec, 64);

    vb_handle_t *w = VB_OpenWrite(TEST_FILE_RAW, 1024, "");
    ASSERT_TRUE(w != NULL);
    if (w) {
        SET(out_rec, "Alpha Payload");
        ASSERT_EQ_INT(PUT_REC(w, out_rec), 1);

        SET(out_rec, "Beta Payload 12345");
        ASSERT_EQ_INT(PUT_REC(w, out_rec), 1);

        VB_Close(w);
    }

    vb_handle_t *r = VB_OpenRead(TEST_FILE_RAW, "");
    ASSERT_TRUE(r != NULL);
    if (r) {
        CLEAR(in_rec);
        ASSERT_EQ_INT(GET_REC(r, in_rec), 1);
        ASSERT_EQ_UINT(LEN(in_rec), 13);
        ASSERT_STR_EQ(in_rec, "Alpha Payload");

        CLEAR(in_rec);
        ASSERT_EQ_INT(GET_REC(r, in_rec), 1);
        ASSERT_EQ_UINT(LEN(in_rec), 18);
        ASSERT_STR_EQ(in_rec, "Beta Payload 12345");

        /* EOF */
        ASSERT_EQ_INT(GET_REC(r, in_rec), 0);

        VB_Close(r);
    }

    if (g_tests_failed == initial_failures) printf("PASS\n");
}

static void test_locate_mode_and_skip(void) {
    TEST_CASE_START("Zero-Copy Locate Mode (VB_GetLocate) & Skip (VB_Skip)");
    int initial_failures = g_tests_failed;

    /* Write 5 records */
    vb_handle_t *w = VB_OpenWrite(TEST_FILE_RAW, 1024, "");
    ASSERT_TRUE(w != NULL);
    if (w) {
        VB_Put(w, "REC_0", 5);
        VB_Put(w, "REC_1_SKIP", 10);
        VB_Put(w, "REC_2_LOCATE", 12);
        VB_Put(w, "REC_3_SKIP", 10);
        VB_Put(w, "REC_4_LAST", 10);
        VB_Close(w);
    }

    /* Read with locate and skip */
    vb_handle_t *r = VB_OpenRead(TEST_FILE_RAW, "");
    ASSERT_TRUE(r != NULL);
    if (r) {
        const char *loc_ptr = NULL;
        uint32_t loc_len = 0;
        uint32_t skipped_len = 0;

        // 1. Read REC_0
        ASSERT_EQ_INT(VB_GetLocate(r, &loc_ptr, &loc_len), 1);
        ASSERT_EQ_UINT(loc_len, 5);
        ASSERT_MEM_EQ(loc_ptr, "REC_0", 5);

        // 2. Skip REC_1
        ASSERT_EQ_INT(VB_Skip(r, &skipped_len), 1);
        ASSERT_EQ_UINT(skipped_len, 10);

        // 3. Read REC_2 with locate
        ASSERT_EQ_INT(VB_GetLocate(r, &loc_ptr, &loc_len), 1);
        ASSERT_EQ_UINT(loc_len, 12);
        ASSERT_MEM_EQ(loc_ptr, "REC_2_LOCATE", 12);

        // 4. Test FIND_REC macro to skip 1 record
        int stat = 0;
        FIND_REC(r, 1, stat);
        ASSERT_EQ_INT(stat, 1);

        // 5. Read REC_4_LAST
        ASSERT_EQ_INT(VB_GetLocate(r, &loc_ptr, &loc_len), 1);
        ASSERT_EQ_UINT(loc_len, 10);
        ASSERT_MEM_EQ(loc_ptr, "REC_4_LAST", 10);

        // EOF check on locate
        ASSERT_EQ_INT(VB_GetLocate(r, &loc_ptr, &loc_len), 0);

        VB_Close(r);
    }

    if (g_tests_failed == initial_failures) printf("PASS\n");
}

static void test_multiblock_spanning(void) {
    TEST_CASE_START("Multi-Block Spanning & Buffer Flushing");
    int initial_failures = g_tests_failed;

    // Small block size (256 bytes) to force multiple block flushes
    const uint32_t block_size = 256;
    const int total_recs = 100;

    vb_handle_t *w = VB_OpenWrite(TEST_FILE_RAW, block_size, "");
    ASSERT_TRUE(w != NULL);
    if (w) {
        char payload[64];
        for (int i = 0; i < total_recs; i++) {
            snprintf(payload, sizeof(payload), "RECORD_PAYLOAD_INDEX_%05d_DATA", i);
            int rc = VB_Put(w, payload, (uint32_t)strlen(payload));
            ASSERT_EQ_INT(rc, 1);
        }
        VB_Close(w);
    }

    vb_handle_t *r = VB_OpenRead(TEST_FILE_RAW, "");
    ASSERT_TRUE(r != NULL);
    if (r) {
        char buf[128];
        char expected[64];
        uint32_t out_len = 0;
        for (int i = 0; i < total_recs; i++) {
            snprintf(expected, sizeof(expected), "RECORD_PAYLOAD_INDEX_%05d_DATA", i);
            int rc = VB_Get(r, buf, (uint32_t)sizeof(buf), &out_len);
            ASSERT_EQ_INT(rc, 1);
            ASSERT_EQ_UINT(out_len, (uint32_t)strlen(expected));
            ASSERT_STR_EQ(buf, expected);
        }
        ASSERT_EQ_INT(VB_Get(r, buf, (uint32_t)sizeof(buf), &out_len), 0);
        VB_Close(r);
    }

    if (g_tests_failed == initial_failures) printf("PASS\n");
}

static void test_ebcdic_translation(void) {
    TEST_CASE_START("EBCDIC Code Page Translation (037 & 1047)");
    int initial_failures = g_tests_failed;

    const char *sample_ascii = "IBM-MAINFRAME EBCDIC 037/1047 VALIDATION string 1234567890 !@#$%&*()";
    uint32_t sample_len = (uint32_t)strlen(sample_ascii);

    /* Test 037 Translation */
    {
        vb_handle_t *w = VB_OpenWrite(TEST_FILE_037, 512, "037");
        ASSERT_TRUE(w != NULL);
        if (w) {
            VB_Put(w, sample_ascii, sample_len);
            VB_Close(w);
        }

        vb_handle_t *r = VB_OpenRead(TEST_FILE_037, "037");
        ASSERT_TRUE(r != NULL);
        if (r) {
            char buf[256];
            uint32_t out_len = 0;
            int rc = VB_Get(r, buf, (uint32_t)sizeof(buf), &out_len);
            ASSERT_EQ_INT(rc, 1);
            ASSERT_EQ_UINT(out_len, sample_len);
            ASSERT_STR_EQ(buf, sample_ascii);
            VB_Close(r);
        }
    }

    /* Test 1047 Translation */
    {
        vb_handle_t *w = VB_OpenWrite(TEST_FILE_1047, 512, "1047");
        ASSERT_TRUE(w != NULL);
        if (w) {
            VB_Put(w, sample_ascii, sample_len);
            VB_Close(w);
        }

        vb_handle_t *r = VB_OpenRead(TEST_FILE_1047, "1047");
        ASSERT_TRUE(r != NULL);
        if (r) {
            char buf[256];
            uint32_t out_len = 0;
            int rc = VB_Get(r, buf, (uint32_t)sizeof(buf), &out_len);
            ASSERT_EQ_INT(rc, 1);
            ASSERT_EQ_UINT(out_len, sample_len);
            ASSERT_STR_EQ(buf, sample_ascii);
            VB_Close(r);
        }
    }

    /* Direct translation table round-trip verification */
    /* Direct translation table round-trip verification for standard ASCII (0..127) */
    for (int i = 0; i < 128; i++) {
        uint8_t c = (uint8_t)i;
        uint8_t e037 = ASCII_TO_EBCDIC_037[c];
        uint8_t a037 = EBCDIC_037_TO_ASCII[e037];
        ASSERT_EQ_UINT(a037, c);

        uint8_t e1047 = ASCII_TO_EBCDIC_1047[c];
        uint8_t a1047 = EBCDIC_1047_TO_ASCII[e1047];
        ASSERT_EQ_UINT(a1047, c);
    }

    if (g_tests_failed == initial_failures) printf("PASS\n");
}

/* ---------------------------------------------------------------------------
 * Main Test Runner
 * --------------------------------------------------------------------------- */
int main(void) {
    TEST_SUITE_START("VBX Variable-Block Record I/O Test Suite");

    cleanup_temp_files();

    test_write_read_basic();
    test_fss_macros_integration();
    test_locate_mode_and_skip();
    test_multiblock_spanning();
    test_ebcdic_translation();

    cleanup_temp_files();

    printf("\n========================================\n");
    printf("  TEST RESULTS: %d Passed, %d Failed, %d Total Assertions\n",
           g_tests_passed, g_tests_failed, g_tests_run);
    printf("========================================\n");

    return (g_tests_failed == 0) ? 0 : 1;
}
