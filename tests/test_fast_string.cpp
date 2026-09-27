#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cassert>
#include <cstring>
#include <cstdint>

#if __has_include("include/fast_string.hpp")
    #include "include/fast_string.hpp"
#elif __has_include("fast_string.hpp")
    #include "fast_string.hpp"
#else
    #include "../include/fast_string.hpp"
#endif

// Compile-time constexpr validation
#if defined(__cpp_constexpr) && __cpp_constexpr >= 201304L
constexpr bool test_constexpr_eval() {
    fss::fast_string<16> s("ConstexprTest");
    if (s.size() != 13) return false;
    if (s.empty()) return false;
    if (s[0] != 'C') return false;
    return true;
}
static_assert(test_constexpr_eval(), "fast_string constexpr evaluation failed");
#endif

/* ---------------------------------------------------------------------------
 * Minimal Test Framework for C++
 * --------------------------------------------------------------------------- */
static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_SUITE_START(name) \
    std::cout << "\n========================================\n" \
              << "  RUNNING TEST SUITE: " << name << "\n" \
              << "========================================\n"

#define TEST_CASE_START(name) \
    std::cout << "  [TEST] " << name << " ... "

#define ASSERT_TRUE(expr) do { \
    g_tests_run++; \
    if (!(expr)) { \
        std::cout << "\n    FAILED at " << __FILE__ << ":" << __LINE__ \
                  << ": Assertion failed: (" << #expr << ")\n"; \
        g_tests_failed++; \
    } else { \
        g_tests_passed++; \
    } \
} while(0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))

#define ASSERT_EQ(actual, expected) do { \
    g_tests_run++; \
    if ((actual) != (expected)) { \
        std::cout << "\n    FAILED at " << __FILE__ << ":" << __LINE__ \
                  << ": Expected [" << (expected) << "], got [" << (actual) \
                  << "] (expr: " << #actual << ")\n"; \
        g_tests_failed++; \
    } else { \
        g_tests_passed++; \
    } \
} while(0)

#define ASSERT_STR_EQ(actual, expected) do { \
    g_tests_run++; \
    std::string_view _a(actual); \
    std::string_view _e(expected); \
    if (_a != _e) { \
        std::cout << "\n    FAILED at " << __FILE__ << ":" << __LINE__ \
                  << ": Expected [\"" << _e << "\"], got [\"" << _a \
                  << "\"] (expr: " << #actual << ")\n"; \
        g_tests_failed++; \
    } else { \
        g_tests_passed++; \
    } \
} while(0)

/* ---------------------------------------------------------------------------
 * Test Cases
 * --------------------------------------------------------------------------- */

static void test_constructors_and_properties() {
    TEST_CASE_START("Constructors & Basic Properties");
    int initial_failures = g_tests_failed;

    // Default constructor: buffer size 16, capacity 15 content bytes
    fss::fast_string<16> s1;
    ASSERT_EQ(s1.size(), 0u);
    ASSERT_EQ(s1.length(), 0u);
    ASSERT_EQ(s1.max_size(), 15u);
    ASSERT_EQ(s1.capacity(), 15u);
    ASSERT_EQ(s1.buffer_size, 16u);
    ASSERT_TRUE(s1.empty());
    ASSERT_STR_EQ(s1.c_str(), "");
    ASSERT_STR_EQ(s1.data(), "");

    // String literal constructor
    fss::fast_string<16> s2("Hello World");
    ASSERT_EQ(s2.size(), 11u);
    ASSERT_FALSE(s2.empty());
    ASSERT_STR_EQ(s2.view(), "Hello World");
    ASSERT_STR_EQ(s2.c_str(), "Hello World");

    // std::string_view constructor
    std::string_view sv = "StringViewInit";
    fss::fast_string<32> s3(sv);
    ASSERT_EQ(s3.size(), 14u);
    ASSERT_STR_EQ(s3.view(), "StringViewInit");

    // Constructor with truncation (capacity 7 for buffer size 8)
    fss::fast_string<8> s4("0123456789ABCDEF");
    ASSERT_EQ(s4.size(), 7u);
    ASSERT_STR_EQ(s4.view(), "0123456");

    // Pointer + count constructor
    fss::fast_string<16> s5("ABCDEFGHIJ", 5);
    ASSERT_EQ(s5.size(), 5u);
    ASSERT_STR_EQ(s5.view(), "ABCDE");

    // Copy constructor across different capacities
    fss::fast_string<32> s6(s2);
    ASSERT_EQ(s6.size(), 11u);
    ASSERT_STR_EQ(s6.view(), "Hello World");

    fss::fast_string<5> s7(s2); // Truncation during copy construction (capacity 4)
    ASSERT_EQ(s7.size(), 4u);
    ASSERT_STR_EQ(s7.view(), "Hell");

    if (g_tests_failed == initial_failures) std::cout << "PASS\n";
}

static void test_element_access_and_iterators() {
    TEST_CASE_START("Element Access & Iterators");
    int initial_failures = g_tests_failed;

    fss::fast_string<10> s("ABCDE");

    // Operator[]
    ASSERT_EQ(s[0], 'A');
    ASSERT_EQ(s[4], 'E');

    // front() & back()
    ASSERT_EQ(s.front(), 'A');
    ASSERT_EQ(s.back(), 'E');

    // Modification via operator[]
    s[1] = 'X';
    ASSERT_STR_EQ(s.view(), "AXCDE");

    // Iterators
    std::string collected;
    for (char ch : s) {
        collected.push_back(ch);
    }
    ASSERT_EQ(collected, "AXCDE");

    // Const iterators & reverse iterators
    const auto& cs = s;
    std::string rev;
    for (auto it = cs.rbegin(); it != cs.rend(); ++it) {
        rev.push_back(*it);
    }
    ASSERT_EQ(rev, "EDCXA");

    if (g_tests_failed == initial_failures) std::cout << "PASS\n";
}

static void test_mutations_and_appends() {
    TEST_CASE_START("Mutations, Appends, & Push/Pop");
    int initial_failures = g_tests_failed;

    fss::fast_string<16> s;

    // push_back & pop_back
    s.push_back('X');
    s.push_back('Y');
    ASSERT_EQ(s.size(), 2u);
    ASSERT_STR_EQ(s.view(), "XY");

    s.pop_back();
    ASSERT_EQ(s.size(), 1u);
    ASSERT_STR_EQ(s.view(), "X");

    // append string_view / C-string
    s.append("yz");
    ASSERT_STR_EQ(s.view(), "Xyz");

    // operator+=
    s += "123";
    ASSERT_STR_EQ(s.view(), "Xyz123");

    s += '!';
    ASSERT_STR_EQ(s.view(), "Xyz123!");

    // append another fast_string
    fss::fast_string<8> other("456");
    s += other;
    ASSERT_STR_EQ(s.view(), "Xyz123!456");

    // Truncating append when capacity is reached (15 chars max for fast_string<16>)
    s += "789ABCDEF_OVERFLOW";
    ASSERT_EQ(s.size(), 15u);
    ASSERT_STR_EQ(s.view(), "Xyz123!456789AB");

    // clear()
    s.clear();
    ASSERT_TRUE(s.empty());
    ASSERT_EQ(s.size(), 0u);
    ASSERT_STR_EQ(s.c_str(), "");

    if (g_tests_failed == initial_failures) std::cout << "PASS\n";
}

static void test_string_operations() {
    TEST_CASE_START("String Operations (find, rfind, substr, sub_string, starts/ends_with, contains)");
    int initial_failures = g_tests_failed;

    fss::fast_string<32> s("Hello, Modern World!");

    // substr (returns std::string_view)
    auto sub = s.substr(7, 6);
    ASSERT_EQ(sub, "Modern");

    // sub_string (returns fast_string)
    auto sub_fs = s.sub_string<16>(0, 5);
    ASSERT_STR_EQ(sub_fs.view(), "Hello");

    // find
    ASSERT_EQ(s.find("Modern"), 7u);
    ASSERT_EQ(s.find('M'), 7u);
    ASSERT_EQ(s.find("NotFound"), fss::fast_string<32>::npos);

    // rfind
    fss::fast_string<16> rep("ab_ab_ab");
    ASSERT_EQ(rep.rfind("ab"), 6u);
    ASSERT_EQ(rep.rfind('b'), 7u);

    // starts_with & ends_with
    fss::fast_string<20> path("/usr/local/bin");
    ASSERT_TRUE(path.starts_with("/usr"));
    ASSERT_TRUE(path.starts_with('/'));
    ASSERT_FALSE(path.starts_with("etc"));
    ASSERT_TRUE(path.ends_with("bin"));
    ASSERT_TRUE(path.ends_with('n'));
    ASSERT_FALSE(path.ends_with("local"));

    // contains
    ASSERT_TRUE(path.contains("local"));
    ASSERT_FALSE(path.contains("opt"));

    if (g_tests_failed == initial_failures) std::cout << "PASS\n";
}

static void test_comparisons_and_views() {
    TEST_CASE_START("Comparisons & std::string_view Interoperability");
    int initial_failures = g_tests_failed;

    fss::fast_string<16> a("Alpha");
    fss::fast_string<32> b("Alpha");
    fss::fast_string<16> c("Beta");

    // Comparison across different template capacities
    ASSERT_TRUE(a == b);
    ASSERT_FALSE(a != b);
    ASSERT_TRUE(a < c);
    ASSERT_TRUE(a <= c);
    ASSERT_TRUE(c > a);
    ASSERT_TRUE(c >= a);

    // Comparison with std::string_view and literals
    ASSERT_TRUE(a == "Alpha");
    ASSERT_TRUE(a == std::string_view("Alpha"));
    ASSERT_TRUE(a < "Beta");
    ASSERT_TRUE("Alpha" == a);
    ASSERT_TRUE("Beta" > a);

    // Comparison with std::string
    std::string std_alpha = "Alpha";
    ASSERT_TRUE(a == std_alpha);

    // Implicit/explicit conversion to std::string_view
    std::string_view sv_a = a;
    ASSERT_EQ(sv_a, "Alpha");

    // Concatenation operator+
    auto combined = a + "_" + "Beta";
    ASSERT_STR_EQ(combined.view(), "Alpha_Beta");

    if (g_tests_failed == initial_failures) std::cout << "PASS\n";
}

static void test_stream_and_hashing() {
    TEST_CASE_START("Stream Output & std::hash / Unordered Containers");
    int initial_failures = g_tests_failed;

    fss::fast_string<24> s("StreamTest_123");

    // Stream operator<<
    std::ostringstream oss;
    oss << "Prefix: " << s << " :Suffix";
    ASSERT_EQ(oss.str(), "Prefix: StreamTest_123 :Suffix");

    // std::hash and std::unordered_set
    std::unordered_set<fss::fast_string<32>> set;
    set.insert(fss::fast_string<32>("Key1"));
    set.insert(fss::fast_string<32>("Key2"));
    set.insert(fss::fast_string<32>("Key1")); // Duplicate

    ASSERT_EQ(set.size(), 2u);
    ASSERT_TRUE(set.find(fss::fast_string<32>("Key1")) != set.end());
    ASSERT_TRUE(set.find(fss::fast_string<32>("Key3")) == set.end());

    // std::unordered_map
    std::unordered_map<fss::fast_string<16>, int> map;
    map[fss::fast_string<16>("Counter")] = 42;
    ASSERT_EQ(map[fss::fast_string<16>("Counter")], 42);

    if (g_tests_failed == initial_failures) std::cout << "PASS\n";
}

/* ---------------------------------------------------------------------------
 * Main Test Runner
 * --------------------------------------------------------------------------- */
int main() {
    TEST_SUITE_START("FastSafeStrings C++ fast_string<N> Test Suite");

    test_constructors_and_properties();
    test_element_access_and_iterators();
    test_mutations_and_appends();
    test_string_operations();
    test_comparisons_and_views();
    test_stream_and_hashing();

    std::cout << "\n========================================\n"
              << "  TEST RESULTS: " << g_tests_passed << " Passed, "
              << g_tests_failed << " Failed, " << g_tests_run << " Total Assertions\n"
              << "========================================\n";

    return (g_tests_failed == 0) ? 0 : 1;
}
