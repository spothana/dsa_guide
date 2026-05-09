/*
 * queue.h — Circular-array queue + deque
 *
 * Algorithms: BFS, sliding window (monotonic deque), work-stealing
 * Domain use: run queues, Qdisc, I/O request queues, data loader
 */
#pragma once
#include <stddef.h>
#include <stdbool.h>

/* ── Basic FIFO queue ────────────────────────────────────────────────────── */
typedef struct {
    int   *data;
    int    head, tail;
    size_t size, capacity;
} Queue;

Queue *queue_create(size_t cap);
void   queue_destroy(Queue *q);
bool   queue_enqueue(Queue *q, int val);
int    queue_dequeue(Queue *q);
int    queue_peek(const Queue *q);
bool   queue_is_empty(const Queue *q);

/* BFS: returns visited order array (caller frees), size written to *out_n */
int   *queue_bfs(int **adj, int n, int src, int *out_n);

/* ── Deque (double-ended) ────────────────────────────────────────────────── */
typedef struct {
    int   *data;
    int    front, back;
    size_t size, capacity;
} Deque;

Deque *deque_create(size_t cap);
void   deque_destroy(Deque *d);
bool   deque_push_front(Deque *d, int val);
bool   deque_push_back(Deque *d, int val);
int    deque_pop_front(Deque *d);
int    deque_pop_back(Deque *d);
int    deque_front(const Deque *d);
int    deque_back(const Deque *d);
bool   deque_is_empty(const Deque *d);

/*
 * Monotonic deque sliding window max — O(n) total
 * Returns array of window maximums (caller frees), length = n - k + 1
 */
int   *deque_sliding_window_max(const int *arr, int n, int k, int *out_len);
