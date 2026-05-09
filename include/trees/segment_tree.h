/*
 * segment_tree.h — Segment / interval tree
 *
 * Algorithms: O(log n + k) range query, lazy propagation, augmented BST
 * Domain use: VMA lookup, firewall port-range rules, temporal range queries,
 *             MVCC, attention span indexing
 */
#pragma once
#include <stddef.h>

/* ── Segment tree (range sum + range min/max) ────────────────────────────── */
typedef struct {
    int   *tree;   /* 1-indexed, size = 4*n */
    int   *lazy;   /* lazy propagation tags */
    int    n;
} SegTree;

SegTree *segtree_create(const int *arr, int n);
void     segtree_destroy(SegTree *st);

/* point update — O(log n) */
void     segtree_update(SegTree *st, int pos, int val);

/* range update (add delta to [l,r]) with lazy prop — O(log n) */
void     segtree_range_update(SegTree *st, int l, int r, int delta);

/* range sum query [l,r] — O(log n) */
int      segtree_query_sum(SegTree *st, int l, int r);

/* range min query [l,r] — O(log n) */
int      segtree_query_min(SegTree *st, int l, int r);

/* ── Interval tree (stabbing / overlap query) ────────────────────────────── */
typedef struct ITreeNode {
    int              lo, hi;      /* interval */
    int              max_hi;      /* augmented max of subtree */
    int              val;
    struct ITreeNode *left, *right;
} ITreeNode;

typedef struct { ITreeNode *root; } IntervalTree;

IntervalTree *itree_create(void);
void          itree_destroy(IntervalTree *t);
void          itree_insert(IntervalTree *t, int lo, int hi, int val);

/* stabbing query: find any interval containing point p */
ITreeNode    *itree_stab(IntervalTree *t, int p);

/* overlap query: find all intervals overlapping [lo,hi] */
void          itree_overlap(IntervalTree *t, int lo, int hi,
                            void (*visit)(ITreeNode *));
