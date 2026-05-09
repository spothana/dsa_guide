/*
 * union_find.h — Union-Find (Disjoint Set Union)
 *
 * Algorithms: path compression, union by rank, O(α) amortised
 * Domain use: namespace merging, network partition detection,
 *             fsck consistency check, CUDA connected components,
 *             cluster merging, graph neural nets
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    int   *parent;
    int   *rank;
    size_t n;
    size_t num_components;
} UnionFind;

UnionFind *uf_create(size_t n);
void       uf_destroy(UnionFind *uf);

/* find with path compression — O(α) amortised */
int        uf_find(UnionFind *uf, int x);

/* union by rank — O(α) amortised */
bool       uf_union(UnionFind *uf, int x, int y);  /* returns false if same set */

bool       uf_connected(UnionFind *uf, int x, int y);
size_t     uf_num_components(const UnionFind *uf);

/* Build from edge list — O(E·α) */
UnionFind *uf_from_edges(int n, int (*edges)[2], int num_edges);

void       uf_print(const UnionFind *uf);
