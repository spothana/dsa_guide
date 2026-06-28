/*
 * bits.c - Bit-manipulation algorithms
 *
 * Every comment block explains:
 *   - The bit invariant being exploited
 *   - Why the trick works (the mathematical argument)
 *   - Where it appears in real systems
 */
#include "bits/bits.h"
#include <stdio.h>
#include <assert.h>

/* === 2. Arithmetic without +/-/(mul)/div === */

/*
 * bit_add - add without the + operator
 *
 * How it works:
 *   XOR  computes the sum of bits with no carry:  a ^ b
 *   AND  computes the carry positions:            (a & b) << 1
 *
 * Repeat until there is no carry left.  Each iteration propagates
 * carries at least one position to the left, so this terminates in
 * at most 32 iterations (one per bit).
 *
 * Domain: adder circuit simulation, interview classic, embedded
 *         environments where + is unavailable (DSP ALUs).
 */
int32_t bit_add(int32_t a, int32_t b) {
    while (b != 0) {
        uint32_t carry = (uint32_t)a & (uint32_t)b;  /* carry positions   */
        a = a ^ b;                                    /* partial sum       */
        b = (int32_t)(carry << 1);                   /* shift carry left  */
    }
    return a;
}

/*
 * bit_mul - multiply without the * operator
 *
 * How it works (Russian peasant / shift-and-add):
 *   For each set bit k in b, add (a << k) to the result.
 *   This is exactly how hardware multipliers work in silicon.
 *
 * Invariant: at each step, result + a*b == original product.
 */
int32_t bit_mul(int32_t a, int32_t b) {
    int32_t result = 0;
    bool negative = (a < 0) ^ (b < 0);
    uint32_t ua = (uint32_t)(a < 0 ? -a : a);
    uint32_t ub = (uint32_t)(b < 0 ? -b : b);
    while (ub) {
        if (ub & 1u) result = bit_add(result, (int32_t)ua);  /* add a if lsb of b set */
        ua <<= 1;   /* double a */
        ub >>= 1;   /* halve  b */
    }
    return negative ? -result : result;
}

/*
 * bit_div - unsigned divide without the / operator
 *
 * How it works (long division in binary):
 *   Find the highest shift s such that (b << s) <= a.
 *   Subtract (b << s) from a, set bit s in quotient.
 *   Repeat on remainder.
 *
 * This is exactly the restoring-division algorithm taught in
 * computer architecture courses.
 */
uint32_t bit_div(uint32_t a, uint32_t b) {
    assert(b != 0);
    uint32_t quotient = 0;
    /* clamp shift to avoid b<<s overflow */
    int clz_b = b ? __builtin_clz(b) : 31;
    int max_s = 31 - (31 - clz_b);  /* = clz_b */
    for (int s = max_s; s >= 0; s--) {
        if ((b << s) <= a) {            /* b fits at position s */
            a -= (b << s);              /* subtract - no / used  */
            quotient |= (1u << s);      /* record this bit       */
        }
    }
    return quotient;
}

/* === 3. Counting === */

/*
 * bit_parity - XOR reduction: fold all bits together.
 *
 * How it works:
 *   Repeatedly XOR the upper half into the lower half, halving the
 *   "width" each time.  The parity of n bits equals parity of
 *   (upper n/2) XOR (lower n/2) - parity is linear over GF(2).
 *
 *   x ^= x >> 16   (fold 32 ? 16 bits)
 *   x ^= x >> 8    (fold 16 ? 8 bits)
 *   x ^= x >> 4    (fold 8  ? 4 bits)
 *   x ^= x >> 2    (fold 4  ? 2 bits)
 *   x ^= x >> 1    (fold 2  ? 1 bit)
 *   result = lowest bit
 *
 * Domain: Hamming code syndrome computation, CRC bit checking,
 *         RAID parity stripe, error detection in comms links.
 */
int bit_parity(uint32_t x) {
    x ^= x >> 16;
    x ^= x >> 8;
    x ^= x >> 4;
    x ^= x >> 2;
    x ^= x >> 1;
    return (int)(x & 1u);
}

/* === 4. Rounding & alignment === */

/*
 * bit_next_pow2 - round up to next power of two
 *
 * How it works:
 *   Decrement x so that exact powers of two round to themselves.
 *   Then fill all bits below the highest set bit using OR-shifting:
 *     v |= v >> 1   fills 2 trailing bits
 *     v |= v >> 2   fills 4 trailing bits
 *     v |= v >> 4   fills 8 trailing bits ? etc.
 *   After 5 steps every bit below MSB is 1.  Add 1 ? next power.
 *
 * Visual example for x = 5 (0b0101):
 *   x-1  = 4 = 0b0100
 *   >> 1:     0b0110
 *   >> 2:     0b0111
 *   >> 4:     0b0111  (no change - only 3 bits)
 *   + 1:  8 = 0b1000  ?
 *
 * Domain: allocator slab sizes, ring buffer capacities, GPU warp counts,
 *         hash table bucket counts (must be power-of-2 for mask trick).
 */
uint32_t bit_next_pow2(uint32_t x) {
    if (x == 0) return 1;
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
}

uint32_t bit_prev_pow2(uint32_t x) {
    if (x == 0) return 0;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x - (x >> 1);   /* isolate highest bit */
}

/* === 5. Isolation === */

/*
 * bit_reverse - reverse all 32 bits
 *
 * How it works (divide-and-conquer on bits):
 *   Step 1: swap adjacent single bits  (mask 0x55555555 = 0101?)
 *   Step 2: swap adjacent pairs        (mask 0x33333333 = 0011?)
 *   Step 3: swap adjacent nibbles      (mask 0x0F0F0F0F = 00001111?)
 *   Step 4: swap bytes via bswap32
 *
 * Each step independently reverses within its group.
 * 5 operations total - O(log 32) = O(1).
 *
 * Domain: bit-reversed addressing in FFT butterfly, CRC computation,
 *         reversing bit-packed fields in hardware registers.
 */
uint32_t bit_reverse(uint32_t x) {
    x = ((x & 0x55555555u) << 1)  | ((x >> 1)  & 0x55555555u);  /* swap bits   */
    x = ((x & 0x33333333u) << 2)  | ((x >> 2)  & 0x33333333u);  /* swap pairs  */
    x = ((x & 0x0F0F0F0Fu) << 4)  | ((x >> 4)  & 0x0F0F0F0Fu);  /* swap nibbles*/
    return __builtin_bswap32(x);                                   /* swap bytes  */
}

/* === 6. Branchless predicates === */

/*
 * bit_min / bit_max - branchless using arithmetic right-shift
 *
 * How it works:
 *   diff = a - b
 *   mask = diff >> 31   ? 0xFFFFFFFF if diff < 0 (a < b), else 0x00000000
 *
 *   min: a + (diff & mask)
 *     if a < b: mask = all-1s ? a + (a-b) = b  ?
 *     if a ? b: mask = all-0s ? a + 0    = a  ?
 *
 * This eliminates branch misprediction - critical in hot paths like
 * sorting networks, min-heap sift-down, and media codec inner loops.
 *
 * Domain: sorting networks, SIMD emulation, real-time audio/video
 *         inner loops where branch misprediction is expensive.
 */
int32_t bit_min(int32_t a, int32_t b) {
    /* mask = 0xFFFFFFFF if a < b, else 0
     * min = b + ((a-b) & mask)
     *   a<b: b + (negative) = b + (a-b) = a  (correct)
     *   a>=b: b + 0 = b                       (correct)
     */
    int32_t diff = a - b;
    int32_t mask = diff >> 31;
    return b + (diff & mask);
}

int32_t bit_max(int32_t a, int32_t b) {
    /* mask = 0xFFFFFFFF if a < b, else 0
     * max = a + ((b-a) & mask)  -- if a<b add (b-a) to get b, else add 0
     */
    int32_t diff = b - a;
    int32_t mask = diff >> 31;  /* 0xFFF if b<a (a is max), 0 if b>=a */
    return a + (diff & ~mask);  /* if b>a: ~mask=0xFFF, add b-a; else add 0 */
}

/* === 7. Integer coding === */

/*
 * bit_from_gray - Gray code to binary
 *
 * How it works:
 *   Binary b[k] = XOR of all Gray bits from position k to MSB.
 *   This is an O(log n) prefix-XOR scan:
 *     g ^= g >> 1;   b[k] = XOR of g[k] and g[k+1]
 *     g ^= g >> 2;   accumulate 2 more positions
 *     etc.
 *
 * Domain: rotary encoder decoding, error-correcting codes,
 *         Hamiltonian path on hypercube, Karnaugh map ordering.
 */
uint32_t bit_from_gray(uint32_t g) {
    g ^= g >> 16;
    g ^= g >> 8;
    g ^= g >> 4;
    g ^= g >> 2;
    g ^= g >> 1;
    return g;
}

/*
 * Morton code (Z-order curve / bit interleave)
 *
 * How it works:
 *   Interleave bits of x and y so that spatially close 2D points
 *   map to nearby 1D indices.  GPU texture caches use Morton order
 *   to maximise cache hits for 2D access patterns.
 *
 *   Spread each 16-bit input to 32 bits with zeros in between:
 *     x = 0x0000XXXX ? 0x0X0X0X0X0X0X0X0X  (even bit positions)
 *     y ? odd bit positions
 *   OR together ? 32-bit Morton code.
 *
 *   The "spread" uses the same divide-and-conquer as bit_reverse:
 *     move bits 15-8  to positions 30-16 (fill 8-bit gaps)
 *     move bits 7-4   to positions 14-8  (fill 4-bit gaps)
 *     etc.
 *
 * Domain: GPU Z-order tiles (texture cache hit rate),
 *         spatial hash tables, quadtree node indexing,
 *         database spatial indexes (PostGIS, S2 geometry).
 */
static uint32_t _spread(uint16_t v) {
    uint32_t x = v;
    x = (x | (x << 8))  & 0x00FF00FFu;  /* ....XXXXXXXX....XXXXXXXX */
    x = (x | (x << 4))  & 0x0F0F0F0Fu;  /* ....XXXX....XXXX....XXXX */
    x = (x | (x << 2))  & 0x33333333u;  /* ..XX..XX..XX..XX..XX..XX */
    x = (x | (x << 1))  & 0x55555555u;  /* .X.X.X.X.X.X.X.X.X.X.X. */
    return x;
}

static uint16_t _compact(uint32_t x) {
    x &= 0x55555555u;
    x = (x | (x >> 1))  & 0x33333333u;
    x = (x | (x >> 2))  & 0x0F0F0F0Fu;
    x = (x | (x >> 4))  & 0x00FF00FFu;
    x = (x | (x >> 8))  & 0x0000FFFFu;
    return (uint16_t)x;
}

uint32_t bit_morton_encode(uint16_t x, uint16_t y) {
    return _spread(x) | (_spread(y) << 1);
}

void bit_morton_decode(uint32_t z, uint16_t *x, uint16_t *y) {
    *x = _compact(z);
    *y = _compact(z >> 1);
}

/* === 8. Classic bit-trick puzzles === */

/*
 * bit_single_number - find the lone element appearing an odd number of times
 *
 * How it works:
 *   XOR is its own inverse: a ^ a = 0, a ^ 0 = a.
 *   XOR-ing an array cancels every element that appears an even number
 *   of times.  Only the odd-count element survives.
 *
 *   Example: [2, 3, 2, 4, 3]
 *     0 ^ 2 ^ 3 ^ 2 ^ 4 ^ 3
 *     = (2^2) ^ (3^3) ^ 4 = 0 ^ 0 ^ 4 = 4  ?
 *
 * Domain: differential backup checksums, RAID parity recovery,
 *         finding the modified packet in an IDS stream.
 */
int32_t bit_single_number(const int32_t *arr, int n) {
    int32_t result = 0;
    for (int i = 0; i < n; i++) result ^= arr[i];
    return result;
}

/*
 * bit_missing_number - find the missing number in [0..n]
 *
 * How it works:
 *   XOR all expected indices (0..n) with all actual values.
 *   Matching pairs cancel.  The missing index is all that remains.
 *
 *   Alternative: expected_sum = n*(n+1)/2 ? actual_sum - but XOR
 *   avoids integer overflow on large n.
 */
int32_t bit_missing_number(const int32_t *arr, int n) {
    int32_t result = n;                          /* XOR in n first */
    for (int i = 0; i < n; i++) result ^= i ^ arr[i];
    return result;
}

/*
 * bit_two_singles - find two elements that appear exactly once;
 *                   all others appear exactly twice.
 *
 * How it works:
 *   Step 1: XOR all elements ? xor_all = a ^ b (the two singles).
 *   Step 2: Find any set bit in xor_all - call it the "pivot bit".
 *           a and b differ in this bit (otherwise xor_all wouldn't
 *           have it set).
 *   Step 3: Partition the array into two groups by the pivot bit.
 *           Each group contains one of {a, b} plus pairs (which cancel).
 *           XOR each group independently ? recovers a and b.
 *
 * Domain: finding two corrupted packets in a network stream,
 *         identifying two changed files in a directory checksum.
 */
void bit_two_singles(const int32_t *arr, int n, int32_t *a, int32_t *b) {
    int32_t xor_all = 0;
    for (int i = 0; i < n; i++) xor_all ^= arr[i];

    /* isolate the lowest set bit where a and b differ */
    int32_t pivot = xor_all & -xor_all;   /* same as bit_lsb */

    *a = 0; *b = 0;
    for (int i = 0; i < n; i++) {
        if (arr[i] & pivot) *a ^= arr[i];  /* group with pivot bit set    */
        else                *b ^= arr[i];  /* group with pivot bit cleared */
    }
}

/* === 9. Subset enumeration === */

/*
 * bit_each_subset - enumerate all non-empty subsets of a bitmask
 *
 * How it works (the "submask trick"):
 *   Start with s = mask (the full set).
 *   Next subset: s = (s - 1) & mask
 *     Subtracting 1 flips the lowest set bit of s and all bits below it.
 *     AND with mask clears any bits not in mask.
 *   This visits every subset in decreasing order in exactly 2^k steps
 *   where k = popcount(mask).
 *
 * Example: mask = 0b0111  (bits 0,1,2)
 *   0b0111 ? 0b0110 ? 0b0101 ? 0b0100 ? 0b0011 ? 0b0010 ? 0b0001 ? stop
 *
 * Domain:
 *   Bitmask DP (TSP, set-cover, scheduling): iterate over all subsets
 *   of n ? 20 items in O(3^n) total across all masks (each element
 *   appears in 2^(n-1) subsets).
 *   Combinatorial optimization, game-tree search with bitmask states.
 */
void bit_each_subset(uint32_t mask,
                     void (*visit)(uint32_t subset, void *ctx),
                     void *ctx) {
    for (uint32_t s = mask; s; s = (s - 1u) & mask)
        visit(s, ctx);
}

/* === 10. Packed byte operations === */

/*
 * bit_byte_eq_mask - SWAR (SIMD Within A Register) byte equality
 *
 * How it works:
 *   XOR zeroes bytes where a and b are equal.
 *   Then detect zero bytes using the standard "hasless" trick:
 *     (v - 0x01010101) & ~v & 0x80808080
 *   If byte i is zero: subtracting 1 causes a borrow into bit 7,
 *   and bit 7 of ~v is 1 ? both conditions satisfied ? bit 7 set.
 *   Map each such byte to 0xFF to produce a full byte mask.
 *
 * Domain: SWAR string search (memchr without SIMD), packet payload
 *         scanning, compressed column equality filters.
 */
uint32_t bit_byte_eq_mask(uint32_t a, uint32_t b) {
    uint32_t v = a ^ b;                                /* 0x00 where bytes equal */
    uint32_t z = (v - 0x01010101u) & ~v & 0x80808080u;/* 0x80 at bit 7 of zero bytes */
    /* expand 0x80 -> 0xFF per byte:
     *   z          has 0x80 at byte-top of each match
     *   z - (z>>7) has 0x7F across the byte of each match
     *   OR together -> 0xFF for matching bytes, 0x00 for non-matching */
    return z | (z - (z >> 7));
}

/*  Demo / print helpers (used by main.c)  */

void bits_demo_print_u32(const char *label, uint32_t x) {
    printf("%-28s = 0x%08X  (%u)  0b", label, x, x);
    for (int i = 31; i >= 0; i--) {
        printf("%c", (x >> i) & 1 ? '1' : '0');
        if (i % 4 == 0 && i > 0) printf("_");
    }
    printf("\n");
}
