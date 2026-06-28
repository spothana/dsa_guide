/*
 * test_bits.c - unit tests for the bit-operations module
 */
#include "dsa.h"
#include <stdio.h>
#include <stdlib.h>

static int tests_run = 0, tests_passed = 0;

#define TEST(name, cond) do { \
    tests_run++; \
    if (cond) { tests_passed++; printf("  PASS  %s\n", name); } \
    else       { printf("  FAIL  %s  (line %d)\n", name, __LINE__); } \
} while(0)

static void test_fundamentals(void) {
    printf("\n[Fundamentals]\n");
    uint32_t x = 0b10110100u;
    TEST("bit_test set",      bit_test(x, 2));
    TEST("bit_test clear",   !bit_test(x, 0));
    TEST("bit_set",           bit_set(x, 0)    == 0b10110101u);
    TEST("bit_clear",         bit_clear(x, 2)  == 0b10110000u);
    TEST("bit_toggle on",     bit_toggle(x, 0) == 0b10110101u);
    TEST("bit_toggle off",    bit_toggle(x, 2) == 0b10110000u);
    TEST("bit_field [2..5]",  bit_field(x, 2, 5) == 0xDu);    /* 1101 */
    TEST("bit_field_set",     bit_field_set(x, 2, 5, 4u) == (x & ~(0xFu<<2)) | (4u<<2));
}

static void test_arithmetic(void) {
    printf("\n[Arithmetic without operators]\n");
    TEST("bit_add 17+25",     bit_add(17, 25)    ==  42);
    TEST("bit_add -10+4",     bit_add(-10, 4)    ==  -6);
    TEST("bit_add 0+0",       bit_add(0, 0)      ==   0);
    TEST("bit_negate 5",      bit_negate(5)      ==  -5);
    TEST("bit_negate -42",    bit_negate(-42)    ==  42);
    TEST("bit_sub 100-58",    bit_sub(100, 58)   ==  42);
    TEST("bit_mul 6*7",       bit_mul(6, 7)      ==  42);
    TEST("bit_mul -3*14",     bit_mul(-3, 14)    == -42);
    TEST("bit_mul 0*99",      bit_mul(0, 99)     ==   0);
    TEST("bit_div 100/7",     bit_div(100, 7)    ==  14u);
    TEST("bit_div 8/2",       bit_div(8, 2)      ==   4u);
    TEST("bit_div 1/1",       bit_div(1, 1)      ==   1u);
    TEST("bit_abs -42",       bit_abs(-42)       ==  42);
    TEST("bit_abs 42",        bit_abs(42)        ==  42);
    TEST("bit_abs 0",         bit_abs(0)         ==   0);
}

static void test_counting(void) {
    printf("\n[Counting]\n");
    TEST("popcount32 0",       bit_popcount32(0)         ==  0);
    TEST("popcount32 0xFF",    bit_popcount32(0xFFu)     ==  8);
    TEST("popcount32 0b1011",  bit_popcount32(0b1011u)   ==  3);
    TEST("popcount64 0xFFFF",  bit_popcount64(0xFFFFu)   == 16);
    TEST("parity 0b10110110",  bit_parity(0b10110110u)   ==  1);
    TEST("parity 0b11110000",  bit_parity(0b11110000u)   ==  0);
    TEST("parity 0",           bit_parity(0)             ==  0);
    TEST("ctz 0b1000",         bit_ctz(0b1000u)          ==  3);
    TEST("ctz 1",              bit_ctz(1u)               ==  0);
    TEST("ctz 0",              bit_ctz(0u)               == 32);
    TEST("clz 0x80000000",     bit_clz(0x80000000u)      ==  0);
    TEST("clz 1",              bit_clz(1u)               == 31);
    TEST("floor_log2 8",       bit_floor_log2(8u)        ==  3);
    TEST("floor_log2 9",       bit_floor_log2(9u)        ==  3);
    TEST("floor_log2 1",       bit_floor_log2(1u)        ==  0);
    TEST("floor_log2 0",       bit_floor_log2(0u)        == -1);
}

static void test_rounding(void) {
    printf("\n[Rounding & alignment]\n");
    TEST("next_pow2 1",        bit_next_pow2(1)   ==   1u);
    TEST("next_pow2 5",        bit_next_pow2(5)   ==   8u);
    TEST("next_pow2 8",        bit_next_pow2(8)   ==   8u);
    TEST("next_pow2 9",        bit_next_pow2(9)   ==  16u);
    TEST("next_pow2 1000",     bit_next_pow2(1000)==1024u);
    TEST("prev_pow2 1",        bit_prev_pow2(1)   ==   1u);
    TEST("prev_pow2 5",        bit_prev_pow2(5)   ==   4u);
    TEST("prev_pow2 8",        bit_prev_pow2(8)   ==   8u);
    TEST("align_up 13,8",      bit_align_up(13,8) ==  16u);
    TEST("align_up 16,8",      bit_align_up(16,8) ==  16u);
    TEST("align_down 13,8",    bit_align_down(13,8)==  8u);
    TEST("is_aligned 16,8",    bit_is_aligned(16,8));
    TEST("is_aligned 13,8",   !bit_is_aligned(13,8));
}

static void test_isolation(void) {
    printf("\n[Isolation]\n");
    TEST("lsb 0b1010",         bit_lsb(0b1010u)        == 0b0010u);
    TEST("lsb 0b1000",         bit_lsb(0b1000u)        == 0b1000u);
    TEST("clear_lsb 0b1010",   bit_clear_lsb(0b1010u)  == 0b1000u);
    TEST("msb 0b10110",        bit_msb(0b10110u)        == 0b10000u);
    TEST("msb 1",              bit_msb(1u)              == 1u);
    TEST("msb 0",              bit_msb(0u)              == 0u);
    TEST("reverse roundtrip",  bit_reverse(bit_reverse(0xDEADBEEFu)) == 0xDEADBEEFu);
    TEST("reverse 0x80000000", bit_reverse(0x80000000u) == 1u);
    TEST("reverse 1",          bit_reverse(1u)          == 0x80000000u);
}

static void test_predicates(void) {
    printf("\n[Branchless predicates]\n");
    TEST("is_pow2 64",         bit_is_pow2(64));
    TEST("is_pow2 0",         !bit_is_pow2(0));
    TEST("is_pow2 63",        !bit_is_pow2(63));
    TEST("same_sign -1,-99",   bit_same_sign(-1,-99));
    TEST("same_sign 1,99",     bit_same_sign(1,99));
    TEST("same_sign -1,1",    !bit_same_sign(-1,1));
    TEST("min 17,42",          bit_min(17,42) == 17);
    TEST("min -5,3",           bit_min(-5,3)  == -5);
    TEST("min equal",          bit_min(7,7)   ==  7);
    TEST("max 17,42",          bit_max(17,42) == 42);
    TEST("max -5,3",           bit_max(-5,3)  ==  3);
    int32_t a=7, b=13; bit_xor_swap(&a,&b);
    TEST("xor_swap",           a==13 && b==7);
    int32_t c=5; bit_xor_swap(&c,&c);          /* same pointer - must be no-op */
    TEST("xor_swap self",      c==5);
}

static void test_coding(void) {
    printf("\n[Integer coding]\n");
    for (uint32_t n = 0; n < 16; n++)
        TEST("gray roundtrip",  bit_from_gray(bit_to_gray(n)) == n);
    /* adjacent gray codes differ in exactly 1 bit */
    for (uint32_t n = 0; n < 15; n++) {
        uint32_t diff = bit_to_gray(n) ^ bit_to_gray(n+1);
        TEST("gray adjacent 1 bit", bit_popcount32(diff) == 1);
    }
    TEST("zigzag  0",          bit_zigzag_encode(0)  == 0u);
    TEST("zigzag -1",          bit_zigzag_encode(-1) == 1u);
    TEST("zigzag  1",          bit_zigzag_encode(1)  == 2u);
    TEST("zigzag -2",          bit_zigzag_encode(-2) == 3u);
    TEST("zigzag decode 5",    bit_zigzag_decode(5)  == -3);
    for (int32_t n = -100; n <= 100; n++)
        TEST("zigzag roundtrip", bit_zigzag_decode(bit_zigzag_encode(n)) == n);
    TEST("bswap32",            bit_bswap32(0x11223344u) == 0x44332211u);
    TEST("bswap64",            bit_bswap64(0x0102030405060708ULL) == 0x0807060504030201ULL);
    uint32_t m = bit_morton_encode(3, 5);
    uint16_t mx, my;
    bit_morton_decode(m, &mx, &my);
    TEST("morton roundtrip x", mx == 3);
    TEST("morton roundtrip y", my == 5);
    TEST("morton(0,0)",        bit_morton_encode(0,0) == 0u);
    TEST("morton(1,1)",        bit_morton_encode(1,1) == 3u);  /* 0b11 */
}

static void test_puzzles(void) {
    printf("\n[Classic puzzles]\n");
    int32_t a1[] = {4,1,2,1,2};
    TEST("single_number",      bit_single_number(a1,5) == 4);
    int32_t a2[] = {2,3,0,1};          /* missing 4 */
    TEST("missing_number",     bit_missing_number(a2,4) == 4);
    int32_t a3[] = {0,1,3};            /* missing 2 */
    TEST("missing_number 2",   bit_missing_number(a3,3) == 2);
    int32_t a4[] = {1,2,1,3,2,5};
    int32_t sa, sb;
    bit_two_singles(a4,6,&sa,&sb);
    /* result pair is {3,5} in some order */
    TEST("two_singles pair",   (sa==3&&sb==5)||(sa==5&&sb==3));
}

static int g_subset_count;
static void count_subset(uint32_t s, void *ctx) { (void)s; (void)ctx; g_subset_count++; }

static void test_subsets(void) {
    printf("\n[Subset enumeration]\n");
    g_subset_count = 0;
    bit_each_subset(0b0111u, count_subset, NULL);
    TEST("subset count k=3",  g_subset_count == 7);   /* 2^3-1 */
    g_subset_count = 0;
    bit_each_subset(0b1111u, count_subset, NULL);
    TEST("subset count k=4",  g_subset_count == 15);  /* 2^4-1 */
    TEST("subset_count macro", bit_subset_count(0b0111u) == 8u);
}

static void test_packed(void) {
    printf("\n[Packed byte ops]\n");
    TEST("has_zero_byte no",   !bit_has_zero_byte(0x41424344u));
    TEST("has_zero_byte yes",   bit_has_zero_byte(0x41004344u));
    TEST("has_zero_byte front", bit_has_zero_byte(0x00424344u));
    TEST("has_zero_byte back",  bit_has_zero_byte(0x41424300u));
    /* byte_eq_mask: bits[15:8] and [31:24] match (0x43==0x43, 0x41==0x41) */
    uint32_t mask = bit_byte_eq_mask(0x41424344u, 0x41004399u);
    /* byte 0 (0x41==0x41) ? 0xFF, byte 2 (0x43==0x43) ? 0xFF, others 0x00 */
    TEST("byte_eq_mask b0",    (mask & 0x000000FFu) == 0x00u);
    TEST("byte_eq_mask b1",    (mask & 0x0000FF00u) == 0x0000FF00u);
    TEST("byte_eq_mask b2",    (mask & 0x00FF0000u) == 0x00u);
    TEST("byte_eq_mask b3",    (mask & 0xFF000000u) == 0xFF000000u);
}

int main(void) {
    printf("????????????????????????????????????????????????????\n");
    printf("?       Bit-Operations Module - Test Suite         ?\n");
    printf("????????????????????????????????????????????????????\n");

    test_fundamentals();
    test_arithmetic();
    test_counting();
    test_rounding();
    test_isolation();
    test_predicates();
    test_coding();
    test_puzzles();
    test_subsets();
    test_packed();

    printf("\n??????????????????????????????????????????????????\n");
    printf("Results: %d / %d passed\n", tests_passed, tests_run);
    printf("??????????????????????????????????????????????????\n");
    return (tests_passed == tests_run) ? 0 : 1;
}
