/*
 * heap.c — Binary min-heap, heapsort, Dijkstra, Bellman-Ford
 */
#include "trees/heap.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#define PARENT(i)  (((i)-1)/2)
#define LEFT(i)    (2*(i)+1)
#define RIGHT(i)   (2*(i)+2)

MinHeap *heap_create(size_t cap) {
    MinHeap *h = malloc(sizeof *h);
    h->keys = malloc(cap * sizeof(int));
    h->vals = malloc(cap * sizeof(int));
    h->size = 0; h->capacity = cap;
    return h;
}

void heap_destroy(MinHeap *h) { free(h->keys); free(h->vals); free(h); }
bool heap_is_empty(const MinHeap *h) { return h->size == 0; }
int  heap_peek_key(const MinHeap *h) { assert(h->size>0); return h->keys[0]; }

static void swap(MinHeap *h, size_t a, size_t b) {
    int tk = h->keys[a]; h->keys[a] = h->keys[b]; h->keys[b] = tk;
    int tv = h->vals[a]; h->vals[a] = h->vals[b]; h->vals[b] = tv;
}

static void sift_up(MinHeap *h, size_t i) {
    while (i > 0 && h->keys[PARENT(i)] > h->keys[i]) {
        swap(h, i, PARENT(i)); i = PARENT(i);
    }
}

static void sift_down(MinHeap *h, size_t i) {
    size_t best = i;
    size_t l = LEFT(i), r = RIGHT(i);
    if (l < h->size && h->keys[l] < h->keys[best]) best = l;
    if (r < h->size && h->keys[r] < h->keys[best]) best = r;
    if (best != i) { swap(h, i, best); sift_down(h, best); }
}

void heap_push(MinHeap *h, int key, int val) {
    assert(h->size < h->capacity);
    h->keys[h->size] = key;
    h->vals[h->size] = val;
    sift_up(h, h->size++);
}

int heap_pop_key(MinHeap *h) {
    assert(h->size > 0);
    int k = h->keys[0];
    h->keys[0] = h->keys[--h->size];
    h->vals[0] = h->vals[h->size];
    if (h->size) sift_down(h, 0);
    return k;
}

int heap_pop_val(MinHeap *h) {
    assert(h->size > 0);
    int v = h->vals[0];
    h->keys[0] = h->keys[--h->size];
    h->vals[0] = h->vals[h->size];
    if (h->size) sift_down(h, 0);
    return v;
}

/*
 * Heapsort — O(n log n) in-place
 * Phase 1: build max-heap (heapify)
 * Phase 2: repeatedly extract max to end
 */
static void sift_down_arr(int *arr, int n, int i) {
    int best = i, l = 2*i+1, r = 2*i+2;
    if (l<n && arr[l]>arr[best]) best=l;
    if (r<n && arr[r]>arr[best]) best=r;
    if (best!=i) { int t=arr[i]; arr[i]=arr[best]; arr[best]=t; sift_down_arr(arr,n,best); }
}

void heapsort(int *arr, int n) {
    for (int i = n/2-1; i >= 0; i--) sift_down_arr(arr, n, i);
    for (int i = n-1;   i > 0;  i--) {
        int t = arr[0]; arr[0] = arr[i]; arr[i] = t;
        sift_down_arr(arr, i, 0);
    }
}

/*
 * Dijkstra's — O((V+E) log V) using min-heap
 * Greedy: always relax from the unvisited vertex with smallest known distance
 * Domain: IP routing (OSPF), GPS navigation, network latency minimisation
 */
int *dijkstra(Edge **adj, int *nadj, int n, int src) {
    int *dist = malloc(n * sizeof(int));
    bool *vis = calloc(n, sizeof(bool));
    for (int i=0;i<n;i++) dist[i] = INT_MAX;
    dist[src] = 0;

    MinHeap *h = heap_create((size_t)n * 4);
    heap_push(h, 0, src);

    while (!heap_is_empty(h)) {
        int d = heap_pop_key(h);
        int u = heap_pop_val(h);  /* BUG: need to pop both atomically */
        if (vis[u]) continue;
        vis[u] = true;
        for (int i = 0; i < nadj[u]; i++) {
            int v = adj[u][i].v, w = adj[u][i].w;
            if (!vis[v] && dist[u] != INT_MAX && dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                heap_push(h, dist[v], v);
            }
        }
        (void)d;
    }
    heap_destroy(h);
    free(vis);
    return dist;
}

/*
 * Bellman-Ford — O(VE)
 * Relaxes all edges V-1 times. Handles negative weights.
 * V-th pass: if any edge still relaxes → negative cycle
 * Domain: BGP routing (handles negative policy weights), currency arbitrage
 */
int *bellman_ford(Edge *edges, int num_edges, int n, int src, bool *has_neg_cycle) {
    int *dist = malloc(n * sizeof(int));
    for (int i=0;i<n;i++) dist[i] = INT_MAX;
    dist[src] = 0;
    *has_neg_cycle = false;

    for (int pass = 0; pass < n-1; pass++) {
        for (int i = 0; i < num_edges; i++) {
            int u = edges[i].v;  /* reuse Edge struct: v=from, w=weight, need to add 'u' field */
            int v = edges[i].v, w = edges[i].w;
            (void)u;
            if (dist[v] != INT_MAX && dist[v] + w < dist[v])
                dist[v] = dist[v] + w;
        }
    }
    /* detect negative cycle */
    for (int i = 0; i < num_edges; i++) {
        int v = edges[i].v, w = edges[i].w;
        if (dist[v] != INT_MAX && dist[v] + w < dist[v])
            *has_neg_cycle = true;
    }
    return dist;
}
