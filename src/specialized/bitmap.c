/*
 * bitmap.c — Compact bitset with SIMD-style bitwise ops
 */
#include "specialized/bitmap.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define WORD_BITS 64
#define WORD(i)  ((i)/WORD_BITS)
#define BIT(i)   ((i)%WORD_BITS)

Bitmap *bm_create(size_t bits) {
    Bitmap *bm = malloc(sizeof *bm);
    bm->bit_count = bits;
    bm->word_count = (bits + WORD_BITS-1) / WORD_BITS;
    bm->words = calloc(bm->word_count, sizeof(uint64_t));
    return bm;
}

void   bm_destroy(Bitmap *bm)             { free(bm->words); free(bm); }
void   bm_set(Bitmap *bm, size_t i)       { bm->words[WORD(i)] |=  (1ULL<<BIT(i)); }
void   bm_clear(Bitmap *bm, size_t i)     { bm->words[WORD(i)] &= ~(1ULL<<BIT(i)); }
bool   bm_test(const Bitmap *bm, size_t i){ return !!(bm->words[WORD(i)] & (1ULL<<BIT(i))); }
void   bm_toggle(Bitmap *bm, size_t i)    { bm->words[WORD(i)] ^=  (1ULL<<BIT(i)); }

void bm_and(Bitmap *dst, const Bitmap *a, const Bitmap *b) {
    assert(a->word_count == b->word_count);
    for (size_t i=0;i<a->word_count;i++) dst->words[i]=a->words[i]&b->words[i];
}
void bm_or(Bitmap *dst, const Bitmap *a, const Bitmap *b) {
    for (size_t i=0;i<a->word_count;i++) dst->words[i]=a->words[i]|b->words[i];
}
void bm_not(Bitmap *dst, const Bitmap *a) {
    for (size_t i=0;i<a->word_count;i++) dst->words[i]=~a->words[i];
}

/*
 * popcount — number of set bits
 * Uses __builtin_popcountll which compiles to POPCNT instruction on x86
 * Domain: CPU mask cardinality, Roaring bitmap AND cardinality
 */
size_t bm_popcount(const Bitmap *bm) {
    size_t cnt=0;
    for (size_t i=0;i<bm->word_count;i++) cnt+=__builtin_popcountll(bm->words[i]);
    return cnt;
}

/*
 * find_first_set — O(1) via __builtin_ctzll (count trailing zeros)
 * Mirrors Linux's find_first_bit() used in RT scheduler priority bitmap
 * to pick the highest-priority runnable task in O(1)
 */
int bm_find_first_set(const Bitmap *bm) {
    for (size_t i=0;i<bm->word_count;i++)
        if (bm->words[i]) return (int)(i*WORD_BITS + __builtin_ctzll(bm->words[i]));
    return -1;
}

int bm_find_next_set(const Bitmap *bm, int from) {
    size_t start = (size_t)from + 1;
    for (size_t i=start/WORD_BITS; i<bm->word_count; i++) {
        uint64_t w = bm->words[i];
        if (i == start/WORD_BITS) w &= ~((1ULL<<(start%WORD_BITS))-1);
        if (w) return (int)(i*WORD_BITS + __builtin_ctzll(w));
    }
    return -1;
}

void bm_print(const Bitmap *bm) {
    printf("Bitmap(%zu bits): ", bm->bit_count);
    for (size_t i=0;i<bm->bit_count&&i<64;i++) printf("%d", bm_test(bm,i)?1:0);
    if (bm->bit_count>64) printf("...");
    printf(" [popcount=%zu]\n", bm_popcount(bm));
}
