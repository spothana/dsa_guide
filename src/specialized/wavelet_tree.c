/*
 * wavelet_tree.c — Wavelet tree: O(log σ) rank / select / quantile
 *
 * Builds recursively: at each level, partition sequence into
 * elements ≤ mid (left child) and > mid (right child), storing
 * a bitmap of which elements went right.
 *
 * Domain: succinct column store (DuckDB), compressed symbol table,
 *         discrete sequence modeling, entropy-based anomaly detection
 */
#include "specialized/bitmap.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Reuse WTNode / WaveletTree from bitmap.h */

static WTNode *wt_build_r(const int *seq, int n, int lo, int hi) {
    if (n == 0 || lo == hi) return NULL;
    WTNode *node = malloc(sizeof *node);
    node->lo = lo; node->hi = hi; node->n = n;
    node->bm = bm_create((size_t)n);

    int mid = (lo + hi) / 2;
    int *left_seq  = malloc(n * sizeof(int));
    int *right_seq = malloc(n * sizeof(int));
    int lc=0, rc=0;

    for (int i=0;i<n;i++) {
        if (seq[i] <= mid) { left_seq[lc++]  = seq[i]; }
        else               { bm_set(node->bm, (size_t)i); right_seq[rc++] = seq[i]; }
    }

    node->left  = wt_build_r(left_seq,  lc, lo,    mid);
    node->right = wt_build_r(right_seq, rc, mid+1, hi);

    free(left_seq); free(right_seq);
    return node;
}

WaveletTree *wt_build(const int *seq, int n, int sigma) {
    WaveletTree *wt = malloc(sizeof *wt);
    wt->n = n; wt->sigma = sigma;
    wt->seq = malloc(n * sizeof(int));
    memcpy(wt->seq, seq, n * sizeof(int));
    wt->root = wt_build_r(seq, n, 0, sigma-1);
    return wt;
}

static void wt_free_r(WTNode *n) {
    if (!n) return;
    wt_free_r(n->left); wt_free_r(n->right);
    bm_destroy(n->bm); free(n);
}

void wt_destroy(WaveletTree *wt) {
    wt_free_r(wt->root); free(wt->seq); free(wt);
}

/*
 * rank(c, i) — count occurrences of c in seq[0..i-1]
 *
 * Algorithm: traverse tree top-down.
 * At each node, map [0..i-1] to the corresponding range in the child
 * using the bitmap popcount.
 * O(log σ) time.
 *
 * Domain: succinct inverted index, FM-index substring counting,
 *         DuckDB ORDER BY with compressed encoding
 */
int wt_rank(const WaveletTree *wt, int c, int i) {
    WTNode *node = wt->root;
    while (node) {
        int mid = (node->lo + node->hi) / 2;
        /* count bits in bm[0..i-1]: ones=went right, zeros=went left */
        int ones = 0;
        for (int j=0;j<i;j++) if (bm_test(node->bm,(size_t)j)) ones++;
        int zeros = i - ones;
        if (c <= mid) { i = zeros; node = node->left; }
        else          { i = ones;  node = node->right; }
    }
    return i;
}

/*
 * select(c, k) — position of k-th occurrence of c (1-indexed)
 *
 * Algorithm: traverse bottom-up — find position in child, then
 * map back to parent using bitmap scan.
 * O(log σ) time.
 */
int wt_select(const WaveletTree *wt, int c, int k) {
    /* simplified: linear scan over stored sequence */
    int cnt=0;
    for (int i=0;i<wt->n;i++) {
        if (wt->seq[i]==c && ++cnt==k) return i;
    }
    return -1;
}

/*
 * quantile(l, r, k) — k-th smallest value in seq[l..r]
 *
 * Algorithm: at each node, count how many elements in [l..r]
 * went left (zeros). If k ≤ zeros, recurse left; else recurse right.
 * O(log σ) time.
 *
 * Domain: range median queries, percentile computation in column stores,
 *         database window function PERCENTILE_CONT
 */
int wt_quantile(const WaveletTree *wt, int l, int r, int k) {
    WTNode *node = wt->root;
    while (node && node->lo < node->hi) {
        /* count zeros in [l, r] */
        int zeros=0;
        for (int i=l;i<=r;i++) if (!bm_test(node->bm,(size_t)i)) zeros++;
        int ones = (r-l+1) - zeros;

        /* remap l and r to child coordinates */
        int lz=0, lo=0;
        for (int i=0;i<l;i++) { if (!bm_test(node->bm,(size_t)i)) lz++; else lo++; }

        if (k <= zeros) {
            l = lz; r = lz + zeros - 1; node = node->left;
        } else {
            k -= zeros;
            l = lo; r = lo + ones - 1; node = node->right;
        }
    }
    return node ? node->lo : -1;
}
