/*
 * heap.h — Binary min-heap / priority queue
 *
 * Algorithms: O(log n) insert/extract, heapsort, Dijkstra's relaxation
 * Domain use: RT priority queue (Linux), timer heap, Dijkstra routing,
 *             LSM compaction, top-K query, beam search
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    int   *keys;
    int   *vals;    /* satellite data (e.g. node index for Dijkstra) */
    size_t size;
    size_t capacity;
} MinHeap;

MinHeap *heap_create(size_t capacity);
void     heap_destroy(MinHeap *h);

void     heap_push(MinHeap *h, int key, int val);   /* O(log n) */
int      heap_pop_key(MinHeap *h);                  /* O(log n) */
int      heap_pop_val(MinHeap *h);
int      heap_peek_key(const MinHeap *h);           /* O(1) */
bool     heap_is_empty(const MinHeap *h);

/* Heapsort — in-place, O(n log n) */
void     heapsort(int *arr, int n);

/* Dijkstra's shortest path — returns dist[] array (caller frees)
 * adj[u] = list of {v, weight} pairs, nadj[u] = count for vertex u */
typedef struct { int v, w; } Edge;
int     *dijkstra(Edge **adj, int *nadj, int n, int src);

/* Bellman-Ford — handles negative weights, detects negative cycles */
int     *bellman_ford(Edge *edges, int num_edges, int n, int src,
                      bool *has_neg_cycle);
