/*
 * bitmap.h — Compact bitmap / bitset
 *
 * Algorithms: SIMD-style bitwise ops, Roaring bitmaps, O(1) priority find-first
 * Domain use: CPU masks, free-page bitmap, RT sched bitmap, block allocation
 *             (ext4), Roaring bitmap index (Druid, ClickHouse), warp lane mask
 */
#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint64_t *words;
    size_t    bit_count;
    size_t    word_count;
} Bitmap;

Bitmap *bm_create(size_t bits);
void    bm_destroy(Bitmap *bm);

void    bm_set(Bitmap *bm, size_t bit);
void    bm_clear(Bitmap *bm, size_t bit);
bool    bm_test(const Bitmap *bm, size_t bit);
void    bm_toggle(Bitmap *bm, size_t bit);

/* bitwise operations (result into dst — must be same size) */
void    bm_and(Bitmap *dst, const Bitmap *a, const Bitmap *b);
void    bm_or(Bitmap *dst, const Bitmap *a, const Bitmap *b);
void    bm_not(Bitmap *dst, const Bitmap *a);

size_t  bm_popcount(const Bitmap *bm);              /* number of set bits */
int     bm_find_first_set(const Bitmap *bm);        /* O(1) via __builtin_ctzll */
int     bm_find_next_set(const Bitmap *bm, int from);
void    bm_print(const Bitmap *bm);

/*
 * wavelet_tree.h — Wavelet tree for rank/select on sequences
 *
 * Algorithms: O(log σ) rank/select, Huffman-shaped wavelet,
 *             succinct representation
 * Domain use: compressed kernel symbol table, succinct column store (DuckDB),
 *             entropy-based anomaly detection, discrete sequence modeling
 */
typedef struct WTNode {
    int           lo, hi;          /* alphabet range [lo, hi] */
    Bitmap       *bm;              /* 1 = right child, 0 = left child */
    int           n;
    struct WTNode *left, *right;
} WTNode;

typedef struct {
    WTNode *root;
    int    *seq;
    int     n;
    int     sigma;   /* alphabet size */
} WaveletTree;

WaveletTree *wt_build(const int *seq, int n, int sigma); /* O(n log σ) */
void         wt_destroy(WaveletTree *wt);

/* rank(c, i): count of c in seq[0..i-1] — O(log σ) */
int          wt_rank(const WaveletTree *wt, int c, int i);

/* select(c, k): position of k-th occurrence of c — O(log σ) */
int          wt_select(const WaveletTree *wt, int c, int k);

/* quantile(l, r, k): k-th smallest in seq[l..r] — O(log σ) */
int          wt_quantile(const WaveletTree *wt, int l, int r, int k);
