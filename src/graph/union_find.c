/*
 * union_find.c — Union-Find with path compression and union by rank
 */
#include "graph/union_find.h"
#include <stdlib.h>
#include <stdio.h>

UnionFind *uf_create(size_t n) {
    UnionFind *uf = malloc(sizeof *uf);
    uf->parent = malloc(n * sizeof(int));
    uf->rank   = calloc(n, sizeof(int));
    uf->n = n; uf->num_components = n;
    for (size_t i=0;i<n;i++) uf->parent[i]=(int)i;
    return uf;
}

void uf_destroy(UnionFind *uf) { free(uf->parent); free(uf->rank); free(uf); }

/*
 * Find with path compression — O(α) amortised
 * After find, every node on the path points directly to root.
 * Domain: network partition detection, fsck component labeling
 */
int uf_find(UnionFind *uf, int x) {
    if (uf->parent[x] != x)
        uf->parent[x] = uf_find(uf, uf->parent[x]);  /* path compression */
    return uf->parent[x];
}

/*
 * Union by rank — O(α) amortised
 * Always attach smaller-rank tree under larger-rank root.
 * Domain: cluster merging, Kruskal's MST, social network union
 */
bool uf_union(UnionFind *uf, int x, int y) {
    int rx = uf_find(uf, x), ry = uf_find(uf, y);
    if (rx == ry) return false;
    if (uf->rank[rx] < uf->rank[ry]) { int t=rx; rx=ry; ry=t; }
    uf->parent[ry] = rx;
    if (uf->rank[rx] == uf->rank[ry]) uf->rank[rx]++;
    uf->num_components--;
    return true;
}

bool   uf_connected(UnionFind *uf, int x, int y) { return uf_find(uf,x)==uf_find(uf,y); }
size_t uf_num_components(const UnionFind *uf)     { return uf->num_components; }

UnionFind *uf_from_edges(int n, int (*edges)[2], int num_edges) {
    UnionFind *uf = uf_create((size_t)n);
    for (int i=0;i<num_edges;i++) uf_union(uf, edges[i][0], edges[i][1]);
    return uf;
}

void uf_print(const UnionFind *uf) {
    printf("UnionFind (n=%zu, components=%zu):\n", uf->n, uf->num_components);
    for (size_t i=0;i<uf->n;i++) printf("  %zu→root %d\n", i, uf_find(uf,(int)i));
}
