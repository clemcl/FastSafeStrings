#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#if __has_include("include/faststr.h")
    #include "include/faststr.h"
#elif __has_include("faststr.h")
    #include "faststr.h"
#else
    #include "../include/faststr.h"
#endif

/* ---------------------------------------------------------------------------
 * Minimal Self-Contained Test Harness
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
        printf("\n    FAILED at %s:%d: Memory comparison mismatch of size %u\n", \
               __FILE__, __LINE__, (unsigned)(len)); \
        g_tests_failed++; \
    } else { \
        g_tests_passed++; \
    } \
} while(0)

/* ---------------------------------------------------------------------------
 * Test Cases: Legacy Macros
 * --------------------------------------------------------------------------- */

static void test_declaration_and_init(void) {
    TEST_CASE_START("Declaration & Initialization (DCL, LEN, MAXLEN)");
    int initial_failures = g_tests_failed;

    DCL(s1, 16);
    ASSERT_EQ_UINT(LEN(s1), 0);
    ASSERT_EQ_UINT(MAXLEN(s1), 16);
    ASSERT_EQ_INT(s1[0], '\0');
    ASSERT_STR_EQ(s1, "");

    /* Lowercase aliases */
    DCL(s2, 32);
    ASSERT_EQ_UINT(len(s2), 0);
    ASSERT_EQ_UINT(maxlen(s2), 32);
    ASSERT_EQ_INT(s2[0], '\0');

    /* Zero capacity */
    DCL(s0, 0);
    ASSERT_EQ_UINT(LEN(s0), 0);
    ASSERT_EQ_UINT(MAXLEN(s0), 0);
    ASSERT_EQ_INT(s0[0], '\0');

    if (g_tests_failed == initial_failures) printf("PASS\n");
}

static void test_set_literal(void) {
    TEST_CASE_START("SET Literal and Truncation");
    int initial_failures = g_tests_failed;

    DCL(s, 10);
    SET(s, "Hello");
    ASSERT_EQ_UINT(LEN(s), 5);
    ASSERT_EQ_UINT(MAXLEN(s), 10);
    ASSERT_STR_EQ(s, "Hello");
    ASSERT_EQ_INT(s[5], '\0');

    /* Exact fit */
    SET(s, "0123456789");
    ASSERT_EQ_UINT(LEN(s), 10);
    ASSERT_STR_EQ(s, "0123456789");
    ASSERT_EQ_INT(s[10], '\0');

    /* Truncation on overflow */
    SET(s, "0123456789EXTRA_TEXT");
    ASSERT_EQ_UINT(LEN(s), 10);
    ASSERT_STR_EQ(s, "0123456789");
    ASSERT_EQ_INT(s[10], '\0');

    /* Set empty literal */
    SET(s, "");
    ASSERT_EQ_UINT(LEN(s), 0);
    ASSERT_STR_EQ(s, "");
    ASSERT_EQ_INT(s[0], '\0');

    if (g_tests_failed == initial_failures) printf("PASS\n");
}

static void test_copy_fss(void) {
    TEST_CASE_START("CPY (FSS to FSS) and Truncation");
    int initial_failures = g_tests_failed;

    DCL(src, 32);
    DCL(dst_small, 8);
    DCL(dst_large, 64);

    SET(src, "FastSafeStrings");
    ASSERT_EQ_UINT(LEN(src), 15);

    /* Copy into larger destination */
    CPY(dst_large, src);
    ASSERT_EQ_UINT(LEN(dst_large), 15);
    ASSERT_STR_EQ(dst_large, "FastSafeStrings");
    ASSERT_EQ_INT(dst_large[15], '\0');

    /* Copy into smaller destination (truncation) */
    CPY(dst_small, src);
    ASSERT_EQ_UINT(LEN(dst_small), 8);
    ASSERT_STR_EQ(dst_small, "FastSafe");
    ASSERT_EQ_INT(dst_small[8], '\0');

    /* Copy empty string */
    CLEAR(src);
    CPY(dst_large, src);
    ASSERT_EQ_UINT(LEN(dst_large), 0);
    ASSERT_STR_EQ(dst_large, "");

    if (g_tests_failed == initial_failures) printf("PASS\n");
}

static void test_concat_fss(void) {
    TEST_CASE_START("CAT (FSS to FSS) and Truncation");
    int initial_failures = g_tests_failed;

    DCL(dst, 12);
    DCL(src1, 10);
    DCL(src2, 10);

    SET(src1, "Hello");
    SET(src2, " World!!!");

    CLEAR(dst);
    CAT(dst, src1);
    ASSERT_EQ_UINT(LEN(dst), 5);
    ASSERT_STR_EQ(dst, "Hello");

    /* Concatenating with remaining capacity */
    CAT(dst, src2);
    ASSERT_EQ_UINT(LEN(dst), 12);
    ASSERT_STR_EQ(dst, "Hello World!");
    ASSERT_EQ_INT(dst[12], '\0');

    /* Concatenating into fully filled buffer */
    CAT(dst, src1);
    ASSERT_EQ_UINT(LEN(dst), 12);
    ASSERT_STR_EQ(dst, "Hello World!");

    if (g_tests_failed == initial_failures) printf("PASS\n");
}

static void test_single_char_ops(void) {
    TEST_CASE_START("CPYCHAR & CATCHAR");
    int initial_failures = g_tests_failed;

    DCL(s, 5);
    CPYCHAR(s, 'A');
    ASSERT_EQ_UINT(LEN(s), 1);
    ASSERT_STR_EQ(s, "A");
    ASSERT_EQ_INT(s[1], '\0');

    CATCHAR(s, 'B');
    CATCHAR(s, 'C');
    CATCHAR(s, 'D');
    CATCHAR(s, 'E');
    ASSERT_EQ_UINT(LEN(s), 5);
    ASSERT_STR_EQ(s, "ABCDE");
    ASSERT_EQ_INT(s[5], '\0');

    /* CATCHAR when full (should do nothing) */
    CATCHAR(s, 'F');
    ASSERT_EQ_UINT(LEN(s), 5);
    ASSERT_STR_EQ(s, "ABCDE");

    /* CPYCHAR on zero capacity */
    DCL(s0, 0);
    CPYCHAR(s0, 'Z');
    ASSERT_EQ_UINT(LEN(s0), 0);
    ASSERT_EQ_INT(s0[0], '\0');

    if (g_tests_failed == initial_failures) printf("PASS\n");
}

static void test_clear(void) {
    TEST_CASE_START("CLEAR / clear");
    int initial_failures = g_tests_failed;

    DCL(s, 20);
    SET(s, "HighPerformance");
    ASSERT_EQ_UINT(LEN(s), 15);

    CLEAR(s);
    ASSERT_EQ_UINT(LEN(s), 0);
    ASSERT_STR_EQ(s, "");
    ASSERT_EQ_INT(s[0], '\0');

    /* Verify capacity preserved */
    ASSERT_EQ_UINT(MAXLEN(s), 20);

    /* Concatenate after clear */
    CAT_LIT(s, "Reused");
    ASSERT_EQ_UINT(LEN(s), 6);
    ASSERT_STR_EQ(s, "Reused");

    if (g_tests_failed == initial_failures) printf("PASS\n");
}

static void test_view_and_substr(void) {
    TEST_CASE_START("VIEW & SUBSTR");
    int initial_failures = g_tests_failed;

    DCL(src, 32);
    SET(src, "ABCDEFGHIJ");

    /* Test VIEW */
    {
        VIEW(v1, src, 3, 4);
        ASSERT_EQ_UINT(LEN(v1), 4);
        ASSERT_EQ_UINT(MAXLEN(v1), 4);
        ASSERT_MEM_EQ(v1, "DEFG", 4);
    }

    /* Test VIEW past length */
    {
        VIEW(v2, src, 15, 5);
        ASSERT_EQ_UINT(LEN(v2), 0);
        ASSERT_TRUE(v2 != NULL);
    }

    /* Test SUBSTR */
    {
        DCL(sub, 16);
        SUBSTR(sub, src, 2, 5);
        ASSERT_EQ_UINT(LEN(sub), 5);
        ASSERT_STR_EQ(sub, "CDEFG");
        ASSERT_EQ_INT(sub[5], '\0');

        /* Substr with start > length */
        SUBSTR(sub, src, 20, 5);
        ASSERT_EQ_UINT(LEN(sub), 0);
        ASSERT_STR_EQ(sub, "");

        /* Substr with start + length > src length */
        SUBSTR(sub, src, 7, 10);
        ASSERT_EQ_UINT(LEN(sub), 3);
        ASSERT_STR_EQ(sub, "HIJ");

        /* Substr with target buffer smaller than requested slice */
        DCL(sub_small, 2);
        SUBSTR(sub_small, src, 0, 5);
        ASSERT_EQ_UINT(LEN(sub_small), 2);
        ASSERT_STR_EQ(sub_small, "AB");
    }

    if (g_tests_failed == initial_failures) printf("PASS\n");
}

static void test_cstr_interop(void) {
    TEST_CASE_START("C-String Interoperability (CPY_CSTR, CAT_CSTR, CAT_LIT)");
    int initial_failures = g_tests_failed;

    DCL(s, 16);
    const char *ptr = "Dynamic C-String";

    CPY_CSTR(s, ptr);
    ASSERT_EQ_UINT(LEN(s), 16);
    ASSERT_STR_EQ(s, "Dynamic C-String");

    CLEAR(s);
    CAT_LIT(s, "Prefix: ");
    ASSERT_STR_EQ(s, "Prefix: ");

    CAT_CSTR(s, "1234567890");
    ASSERT_EQ_UINT(LEN(s), 16);
    ASSERT_STR_EQ(s, "Prefix: 12345678");

    if (g_tests_failed == initial_failures) printf("PASS\n");
}

static void test_comparison(void) {
    TEST_CASE_START("Comparison (CMP, CMP_LIT, CMPCHAR)");
    int initial_failures = g_tests_failed;

    DCL(a, 16);
    DCL(b, 16);
    DCL(c, 16);

    SET(a, "Apple");
    SET(b, "Apple");
    SET(c, "Banana");

    ASSERT_EQ_INT(CMP(a, b), 0);
    ASSERT_TRUE(CMP(a, c) < 0);
    ASSERT_TRUE(CMP(c, a) > 0);

    /* Different lengths with common prefix */
    DCL(p1, 16);
    DCL(p2, 16);
    SET(p1, "Test");
    SET(p2, "Testing");
    ASSERT_TRUE(CMP(p1, p2) < 0);
    ASSERT_TRUE(CMP(p2, p1) > 0);

    /* CMP_LIT */
    ASSERT_EQ_INT(CMP_LIT(a, "Apple"), 0);
    ASSERT_TRUE(CMP_LIT(a, "Apples") < 0);
    ASSERT_TRUE(CMP_LIT(a, "App") > 0);

    /* CMPCHAR */
    ASSERT_EQ_INT(CMPCHAR(a, 'A'), 0);
    ASSERT_TRUE(CMPCHAR(a, 'B') < 0);
    ASSERT_TRUE(CMPCHAR(a, '0') > 0);

    DCL(empty, 8);
    ASSERT_EQ_INT(CMPCHAR(empty, 'A'), -1);

    if (g_tests_failed == initial_failures) printf("PASS\n");
}

static void test_boundary_and_stress(void) {
    TEST_CASE_START("Boundary & Capacity Clamping");
    int initial_failures = g_tests_failed;

    /* 1-byte capacity */
    DCL(s1, 1);
    SET(s1, "X");
    ASSERT_EQ_UINT(LEN(s1), 1);
    ASSERT_STR_EQ(s1, "X");

    SET(s1, "XYZ");
    ASSERT_EQ_UINT(LEN(s1), 1);
    ASSERT_STR_EQ(s1, "X");

    CAT(s1, s1);
    ASSERT_EQ_UINT(LEN(s1), 1);
    ASSERT_STR_EQ(s1, "X");

    /* 256-byte capacity */
    DCL(s256, 256);
    for (int i = 0; i < 25; i++) {
        CAT_LIT(s256, "0123456789");
    }
    ASSERT_EQ_UINT(LEN(s256), 250);

    CAT_LIT(s256, "ABCDEFGH");
    ASSERT_EQ_UINT(LEN(s256), 256);
    ASSERT_EQ_INT(s256[256], '\0');

    if (g_tests_failed == initial_failures) printf("PASS\n");
}

/* ---------------------------------------------------------------------------
 * Test Cases: Modern fss_* Function APIs
 * --------------------------------------------------------------------------- */

static void test_modern_fss_search_and_query(void) {
    TEST_CASE_START("Modern fss_* Search (find_str, find_char, rfind_char, starts/ends_with)");
    int initial_failures = g_tests_failed;

    const char *text = "FasterByDesign: FastSafeStrings Modern Engine";
    uint32_t len = (uint32_t)strlen(text);

    /* fss_find_str */
    ASSERT_EQ_INT(fss_find_str(text, len, "FastSafeStrings", 15), 16);
    ASSERT_EQ_INT(fss_find_str(text, len, "FasterByDesign", 14), 0);
    ASSERT_EQ_INT(fss_find_str(text, len, "NotFound", 8), -1);

    /* fss_find_char & fss_rfind_char */
    ASSERT_EQ_INT(fss_find_char(text, len, ':'), 14);
    ASSERT_EQ_INT(fss_find_char(text, len, 'M'), 32);
    ASSERT_EQ_INT(fss_find_char(text, len, 'Z'), -1);
    ASSERT_EQ_INT(fss_rfind_char(text, len, 'e'), 44);

    /* fss_starts_with & fss_ends_with */
    ASSERT_TRUE(fss_starts_with(text, len, "Faster", 6));
    ASSERT_FALSE(fss_starts_with(text, len, "Modern", 6));
    ASSERT_TRUE(fss_ends_with(text, len, "Engine", 6));
    ASSERT_FALSE(fss_ends_with(text, len, "Design", 6));

    if (g_tests_failed == initial_failures) printf("PASS\n");
}

static void test_modern_fss_transforms_and_trim(void) {
    TEST_CASE_START("Modern fss_* Transforms & Trim (toupper, tolower, ltrim, rtrim, trim)");
    int initial_failures = g_tests_failed;

    DCL(buf, 32);

    /* Case conversions */
    fss_toupper(buf, &dv_buf, "Hello World 123", 15);
    ASSERT_STR_EQ(buf, "HELLO WORLD 123");
    ASSERT_EQ_UINT(LEN(buf), 15);

    fss_tolower(buf, &dv_buf, "Hello World 123", 15);
    ASSERT_STR_EQ(buf, "hello world 123");
    ASSERT_EQ_UINT(LEN(buf), 15);

    /* Trim views */
    const char *padded = "   \t  Trimmed Content   \r\n  ";
    uint32_t plen = (uint32_t)strlen(padded);

    fss_view_t lv = fss_ltrim(padded, plen);
    ASSERT_TRUE(fss_starts_with(lv.data, lv.len, "Trimmed", 7));

    fss_view_t rv = fss_rtrim(padded, plen);
    ASSERT_TRUE(fss_ends_with(rv.data, rv.len, "Content", 7));

    fss_view_t tv = fss_trim(padded, plen);
    ASSERT_EQ_UINT(tv.len, 15);
    ASSERT_MEM_EQ(tv.data, "Trimmed Content", 15);

    if (g_tests_failed == initial_failures) printf("PASS\n");
}

static void test_modern_fss_formatting(void) {
    TEST_CASE_START("Modern fss_* Numeric Formatting (from_uint, from_int)");
    int initial_failures = g_tests_failed;

    DCL(num_str, 32);

    /* Unsigned */
    fss_from_uint(num_str, &dv_num_str, 1234567890ULL);
    ASSERT_STR_EQ(num_str, "1234567890");
    ASSERT_EQ_UINT(LEN(num_str), 10);

    fss_from_uint(num_str, &dv_num_str, 0ULL);
    ASSERT_STR_EQ(num_str, "0");
    ASSERT_EQ_UINT(LEN(num_str), 1);

    /* Signed */
    fss_from_int(num_str, &dv_num_str, -9876543210LL);
    ASSERT_STR_EQ(num_str, "-9876543210");
    ASSERT_EQ_UINT(LEN(num_str), 11);

    fss_from_int(num_str, &dv_num_str, 42LL);
    ASSERT_STR_EQ(num_str, "42");
    ASSERT_EQ_UINT(LEN(num_str), 2);

    if (g_tests_failed == initial_failures) printf("PASS\n");
}

/* ---------------------------------------------------------------------------
 * Main Test Runner
 * --------------------------------------------------------------------------- */
int main(void) {
    TEST_SUITE_START("FastSafeStrings C API Test Suite");

    /* Legacy Macro Tests */
    test_declaration_and_init();
    test_set_literal();
    test_copy_fss();
    test_concat_fss();
    test_single_char_ops();
    test_clear();
    test_view_and_substr();
    test_cstr_interop();
    test_comparison();
    test_boundary_and_stress();

    /* Modern fss_* Function API Tests */
    test_modern_fss_search_and_query();
    test_modern_fss_transforms_and_trim();
    test_modern_fss_formatting();

    printf("\n========================================\n");
    printf("  TEST RESULTS: %d Passed, %d Failed, %d Total Assertions\n",
           g_tests_passed, g_tests_failed, g_tests_run);
    printf("========================================\n");

    return (g_tests_failed == 0) ? 0 : 1;
}
