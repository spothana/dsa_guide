/*
 * bits.h - Bit-manipulation algorithms
 *
 * Every function here solves a real problem using only bitwise operators
 * (&  |  ^  ~  <<  >>).  No loops, no division, no multiplication -
 * just single-instruction or O(1) / O(log n) bit tricks.
 *
 * Organised by theme:
 *
 *   Fundamentals    - test / set / clear / toggle a single bit
 *   Arithmetic      - add, subtract, multiply, divide without +/-/(mul)/div
 *   Counting        - popcount, parity, count trailing/leading zeros
 *   Rounding        - next/prev power of two, alignment
 *   Isolation       - lowest set bit, highest set bit, bit reversal
 *   Predicates      - power-of-two, same sign, min/max without branch
 *   Integer coding  - Gray code, zigzag, byte swap, bit interleave
 *   Classic puzzles - single number (XOR), missing number, two non-dup
 *   Subsets         - enumerate all subsets of a bitmask
 *   Fixed-width     - operations on packed nibbles / bytes
 *
 * Domain use:
 *   OS   : CPU / IRQ masks, RT sched priority bitmap, free-page bitmap
 *   Net  : CIDR mask arithmetic, VLAN tags, packet flags
 *   GPU  : warp lane masks (__ballot_sync), Morton codes (texture cache)
 *   DB   : Roaring bitmaps, hash table probing, column filter pushdown
 *   HW   : CRC / parity, register field extraction, peripheral bitmaps
 *   Algo : subset-sum DP, bitmask BFS, XOR trie, O(1) min/max
 *
 * All functions operate on uint32_t or uint64_t so behaviour is
 * fully defined - no undefined-behaviour signed-overflow traps.
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* === 1. Fundamentals === */

/* Test / set / clear / toggle bit k in x */
static inline bool     bit_test  (uint32_t x, int k) { return !!(x & (1u << k)); }
static inline uint32_t bit_set   (uint32_t x, int k) { return   x | (1u << k);  }
static inline uint32_t bit_clear (uint32_t x, int k) { return   x & ~(1u << k); }
static inline uint32_t bit_toggle(uint32_t x, int k) { return   x ^ (1u << k);  }

/* Extract a field: bits [hi..lo] inclusive */
static inline uint32_t bit_field(uint32_t x, int lo, int hi) {
    return (x >> lo) & ((1u << (hi - lo + 1)) - 1u);
}

/* Replace field [hi..lo] with val (val must fit in hi-lo+1 bits) */
static inline uint32_t bit_field_set(uint32_t x, int lo, int hi, uint32_t val) {
    uint32_t mask = ((1u << (hi - lo + 1)) - 1u) << lo;
    return (x & ~mask) | ((val << lo) & mask);
}

/* === 2. Arithmetic without +/-/(mul)/div === */

/* Add two integers using only bitwise ops + loops (O(log n) iterations) */
int32_t bit_add(int32_t a, int32_t b);

/* Negate: ~x + 1  (two's complement) */
static inline int32_t bit_negate(int32_t x) { return ~x + 1; }

/* Subtract: a - b  =  a + (~b + 1) */
static inline int32_t bit_sub(int32_t a, int32_t b) { return bit_add(a, bit_negate(b)); }

/* Multiply via repeated shift-and-add - O(32) */
int32_t bit_mul(int32_t a, int32_t b);

/* Divide (unsigned) via shift - O(32) */
uint32_t bit_div(uint32_t a, uint32_t b);

/* Absolute value without branch: uses arithmetic right-shift mask */
static inline int32_t bit_abs(int32_t x) {
    int32_t mask = x >> 31;          /* all 0s if positive, all 1s if negative */
    return (x ^ mask) - mask;        /* flip bits + 1 if negative, else identity */
}

/* === 3. Counting === */

/* Population count - number of set bits.
 * Uses __builtin_popcountll (compiles to POPCNT on x86, CNT on ARM). */
static inline int bit_popcount32(uint32_t x) { return __builtin_popcount(x);   }
static inline int bit_popcount64(uint64_t x) { return __builtin_popcountll(x); }

/* Parity: 1 if odd number of set bits, 0 if even.
 * Domain: CRC parity check, Hamming code syndrome */
int bit_parity(uint32_t x);

/* Count trailing zeros (index of lowest set bit).
 * Returns 32 if x == 0.
 * Domain: find_first_bit() in Linux RT scheduler */
static inline int bit_ctz(uint32_t x) { return x ? __builtin_ctz(x)  : 32; }
static inline int bit_ctz64(uint64_t x){ return x ? __builtin_ctzll(x): 64; }

/* Count leading zeros (distance from MSB to highest set bit).
 * Returns 32 if x == 0. */
static inline int bit_clz(uint32_t x) { return x ? __builtin_clz(x)  : 32; }
static inline int bit_clz64(uint64_t x){ return x ? __builtin_clzll(x): 64; }

/* Floor log2 (position of highest set bit), -1 if x==0 */
static inline int bit_floor_log2(uint32_t x) { return x ? 31 - bit_clz(x) : -1; }

/* === 4. Rounding & alignment === */

/* Next power of two >= x (x must be > 0 and <= 2^31) */
uint32_t bit_next_pow2(uint32_t x);

/* Previous power of two <= x */
uint32_t bit_prev_pow2(uint32_t x);

/* Round x up to nearest multiple of align (align must be power of two) */
static inline uint32_t bit_align_up(uint32_t x, uint32_t align) {
    return (x + align - 1u) & ~(align - 1u);
}

/* Round x down to nearest multiple of align */
static inline uint32_t bit_align_down(uint32_t x, uint32_t align) {
    return x & ~(align - 1u);
}

/* Test whether x is already aligned to align */
static inline bool bit_is_aligned(uint32_t x, uint32_t align) {
    return (x & (align - 1u)) == 0;
}

/* === 5. Isolation === */

/* Isolate lowest set bit: bit_lsb(0b1010) ? 0b0010 */
static inline uint32_t bit_lsb(uint32_t x) { return x & (uint32_t)(-(int32_t)x); }

/* Clear lowest set bit: bit_clear_lsb(0b1010) ? 0b1000 */
static inline uint32_t bit_clear_lsb(uint32_t x) { return x & (x - 1u); }

/* Isolate highest set bit */
static inline uint32_t bit_msb(uint32_t x) {
    return x ? (1u << (31 - bit_clz(x))) : 0u;
}

/* Reverse all 32 bits: bit_reverse(0x80000000) ? 0x00000001 */
uint32_t bit_reverse(uint32_t x);

/* === 6. Branchless predicates === */

/* Is x a power of two? (0 returns false) */
static inline bool bit_is_pow2(uint32_t x) { return x && !(x & (x - 1u)); }

/* Do a and b have the same sign? */
static inline bool bit_same_sign(int32_t a, int32_t b) { return !((a ^ b) >> 31); }

/* Branchless min / max using arithmetic right-shift */
int32_t bit_min(int32_t a, int32_t b);
int32_t bit_max(int32_t a, int32_t b);

/* Swap a and b in-place using XOR (no temporary variable) */
static inline void bit_xor_swap(int32_t *a, int32_t *b) {
    if (a != b) { *a ^= *b; *b ^= *a; *a ^= *b; }
}

/* === 7. Integer coding === */

/* Gray code: binary ? Gray and Gray ? binary.
 * Adjacent Gray codes differ in exactly 1 bit.
 * Domain: rotary encoders, error correction, Karnaugh maps */
static inline uint32_t bit_to_gray(uint32_t n)   { return n ^ (n >> 1); }
uint32_t bit_from_gray(uint32_t g);

/* Zigzag encoding: maps signed integers to unsigned for varint storage.
 *   0?0, -1?1, 1?2, -2?3, 2?4 ?
 * Domain: Protocol Buffers sint32/sint64, delta encoding */
static inline uint32_t bit_zigzag_encode(int32_t n) {
    return (uint32_t)((n << 1) ^ (n >> 31));
}
static inline int32_t bit_zigzag_decode(uint32_t n) {
    return (int32_t)((n >> 1) ^ -(int32_t)(n & 1u));
}

/* Byte-swap (endian flip): 0x11223344 ? 0x44332211 */
static inline uint32_t bit_bswap32(uint32_t x) { return __builtin_bswap32(x); }
static inline uint64_t bit_bswap64(uint64_t x) { return __builtin_bswap64(x); }

/* Bit interleave (Morton code / Z-order curve):
 * Interleave bits of two 16-bit values into one 32-bit Morton code.
 * Domain: 2D spatial indexing, texture cache locality, GPU Z-order tiles */
uint32_t bit_morton_encode(uint16_t x, uint16_t y);
void     bit_morton_decode(uint32_t z, uint16_t *x, uint16_t *y);

/* === 8. Classic bit-trick puzzles === */

/* Single number: find the one element that appears an odd number of times.
 * All others appear an even number of times.  XOR cancels pairs. O(n) */
int32_t bit_single_number(const int32_t *arr, int n);

/* Missing number in [0..n]: XOR all indices and values, differences cancel */
int32_t bit_missing_number(const int32_t *arr, int n);

/* Two non-duplicate numbers: given an array where every element except
 * two appears twice, find both.  O(n) time, O(1) space. */
void bit_two_singles(const int32_t *arr, int n, int32_t *a, int32_t *b);

/* === 9. Subset enumeration === */

/* Call visit(subset, ctx) for every non-empty subset of mask, in
 * decreasing order.  Uses the "submask trick": next = (s-1) & mask.
 * O(2^popcount(mask)) - used in bitmask DP */
void bit_each_subset(uint32_t mask, void (*visit)(uint32_t subset, void *ctx),
                     void *ctx);

/* Count subsets of mask: 2^popcount(mask) */
static inline uint32_t bit_subset_count(uint32_t mask) {
    return 1u << bit_popcount32(mask);
}

/* === 10. Packed nibble / byte operations === */

/* Parallel compare: return a byte mask where byte i of result is 0xFF
 * if byte i of a equals byte i of b, else 0x00.
 * Domain: SWAR (SIMD Within A Register) string search */
uint32_t bit_byte_eq_mask(uint32_t a, uint32_t b);

/* Detect a zero byte within a 32-bit word (hasless trick).
 * Domain: strlen without SIMD, packet payload scan */
static inline bool bit_has_zero_byte(uint32_t x) {
    return !!((x - 0x01010101u) & ~x & 0x80808080u);
}

void bits_demo_print_u32(const char *label, uint32_t x);
