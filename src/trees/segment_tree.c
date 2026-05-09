/*
 * segment_tree.c — Segment tree with lazy propagation + interval tree
 */
#include "trees/segment_tree.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

/* ── Segment tree ────────────────────────────────────────────────────────── */

static void build(int *tree, int *lazy, const int *arr, int node, int lo, int hi) {
    lazy[node] = 0;
    if (lo == hi) { tree[node] = arr[lo]; return; }
    int mid = (lo+hi)/2;
    build(tree, lazy, arr, 2*node,   lo,    mid);
    build(tree, lazy, arr, 2*node+1, mid+1, hi);
    tree[node] = tree[2*node] + tree[2*node+1];
}

SegTree *segtree_create(const int *arr, int n) {
    SegTree *st = malloc(sizeof *st);
    st->n    = n;
    st->tree = calloc(4*n, sizeof(int));
    st->lazy = calloc(4*n, sizeof(int));
    build(st->tree, st->lazy, arr, 1, 0, n-1);
    return st;
}

void segtree_destroy(SegTree *st) { free(st->tree); free(st->lazy); free(st); }

static void push_down(int *tree, int *lazy, int node, int lo, int hi) {
    if (lazy[node] == 0) return;
    int mid = (lo+hi)/2;
    /* propagate to children */
    tree[2*node]   += lazy[node] * (mid - lo + 1);
    tree[2*node+1] += lazy[node] * (hi - mid);
    lazy[2*node]   += lazy[node];
    lazy[2*node+1] += lazy[node];
    lazy[node] = 0;
}

/*
 * Point update — O(log n)
 * Domain: update a single VMA size, single counter in eBPF map
 */
void segtree_update(SegTree *st, int pos, int val) {
    int node=1, lo=0, hi=st->n-1;
    while (lo < hi) {
        push_down(st->tree, st->lazy, node, lo, hi);
        int mid=(lo+hi)/2;
        if (pos<=mid) { node=2*node;   hi=mid; }
        else          { node=2*node+1; lo=mid+1; }
    }
    st->tree[node] = val;
}

/*
 * Range update with lazy propagation — O(log n)
 * Defers work: marks a segment as "add delta" without visiting all elements.
 * Domain: bulk memory region permission update, time-series windowed aggregation
 */
static void range_update_r(int *tree, int *lazy, int node,
                            int lo, int hi, int l, int r, int delta) {
    if (r < lo || hi < l) return;
    if (l <= lo && hi <= r) {
        tree[node] += delta * (hi - lo + 1);
        lazy[node] += delta;
        return;
    }
    push_down(tree, lazy, node, lo, hi);
    int mid=(lo+hi)/2;
    range_update_r(tree,lazy,2*node,  lo,  mid,l,r,delta);
    range_update_r(tree,lazy,2*node+1,mid+1,hi,l,r,delta);
    tree[node] = tree[2*node] + tree[2*node+1];
}

void segtree_range_update(SegTree *st, int l, int r, int delta) {
    range_update_r(st->tree, st->lazy, 1, 0, st->n-1, l, r, delta);
}

static int query_sum_r(int *tree, int *lazy, int node,
                        int lo, int hi, int l, int r) {
    if (r < lo || hi < l) return 0;
    if (l <= lo && hi <= r) return tree[node];
    push_down(tree, lazy, node, lo, hi);
    int mid=(lo+hi)/2;
    return query_sum_r(tree,lazy,2*node,  lo,  mid,l,r)
         + query_sum_r(tree,lazy,2*node+1,mid+1,hi,l,r);
}

/*
 * Range sum query [l,r] — O(log n)
 * Domain: prefix byte count in network packet stream,
 *         cumulative score over time window
 */
int segtree_query_sum(SegTree *st, int l, int r) {
    return query_sum_r(st->tree, st->lazy, 1, 0, st->n-1, l, r);
}

static int query_min_r(int *tree, int node, int lo, int hi, int l, int r) {
    if (r < lo || hi < l) return INT_MAX;
    if (l <= lo && hi <= r) return tree[node];
    int mid=(lo+hi)/2;
    int lv = query_min_r(tree, 2*node,   lo,   mid, l, r);
    int rv = query_min_r(tree, 2*node+1, mid+1, hi, l, r);
    return lv < rv ? lv : rv;
}

int segtree_query_min(SegTree *st, int l, int r) {
    return query_min_r(st->tree, 1, 0, st->n-1, l, r);
}

/* ── Interval tree (augmented BST) ─────────────────────────────────────── */
/*
 * Each node stores [lo,hi] and the max hi in its subtree.
 * Augmented max lets us prune: if left->max_hi < p, no left subtree
 * interval can contain p — skip entire left subtree.
 *
 * Domain: VMA overlap detection (Linux mm), firewall port-range rules,
 *         MVCC version interval lookup, CUDA stream overlap scheduling
 */

static ITreeNode *itnode_new(int lo, int hi, int val) {
    ITreeNode *n = malloc(sizeof *n);
    n->lo=lo; n->hi=hi; n->max_hi=hi; n->val=val;
    n->left=n->right=NULL;
    return n;
}

IntervalTree *itree_create(void) {
    IntervalTree *t = malloc(sizeof *t);
    t->root = NULL;
    return t;
}

static void itree_free_r(ITreeNode *n) {
    if (!n) return;
    itree_free_r(n->left);
    itree_free_r(n->right);
    free(n);
}

void itree_destroy(IntervalTree *t) { itree_free_r(t->root); free(t); }

static ITreeNode *itree_insert_r(ITreeNode *n, int lo, int hi, int val) {
    if (!n) return itnode_new(lo, hi, val);
    if (lo < n->lo) n->left  = itree_insert_r(n->left,  lo, hi, val);
    else            n->right = itree_insert_r(n->right, lo, hi, val);
    if (hi > n->max_hi) n->max_hi = hi;
    return n;
}

void itree_insert(IntervalTree *t, int lo, int hi, int val) {
    t->root = itree_insert_r(t->root, lo, hi, val);
}

ITreeNode *itree_stab(IntervalTree *t, int p) {
    ITreeNode *n = t->root;
    while (n) {
        if (p >= n->lo && p <= n->hi) return n;          /* hit */
        if (n->left && n->left->max_hi >= p) n = n->left; /* prune right */
        else n = n->right;
    }
    return NULL;
}

static void itree_overlap_r(ITreeNode *n, int lo, int hi,
                             void (*visit)(ITreeNode *)) {
    if (!n || n->max_hi < lo) return;
    if (n->lo <= hi && n->hi >= lo) visit(n);
    itree_overlap_r(n->left,  lo, hi, visit);
    itree_overlap_r(n->right, lo, hi, visit);
}

void itree_overlap(IntervalTree *t, int lo, int hi,
                   void (*visit)(ITreeNode *)) {
    itree_overlap_r(t->root, lo, hi, visit);
}
