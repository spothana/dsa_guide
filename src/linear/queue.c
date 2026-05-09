/*
 * queue.c — FIFO queue (circular array) + Deque with monotonic sliding window
 */
#include "linear/queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

/* ── Basic Queue ─────────────────────────────────────────────────────────── */

Queue *queue_create(size_t cap) {
    Queue *q = malloc(sizeof *q);
    q->data = malloc(cap * sizeof(int));
    q->head = q->tail = 0; q->size = 0; q->capacity = cap;
    return q;
}

void queue_destroy(Queue *q) { free(q->data); free(q); }

bool queue_enqueue(Queue *q, int val) {
    if (q->size == q->capacity) return false;
    q->data[q->tail] = val;
    q->tail = (q->tail + 1) % (int)q->capacity;
    q->size++;
    return true;
}

int queue_dequeue(Queue *q) {
    assert(q->size > 0);
    int val = q->data[q->head];
    q->head = (q->head + 1) % (int)q->capacity;
    q->size--;
    return val;
}

int  queue_peek(const Queue *q)     { assert(q->size>0); return q->data[q->head]; }
bool queue_is_empty(const Queue *q) { return q->size == 0; }

/*
 * BFS — Breadth-first search using queue
 * Algorithm: enqueue source; while queue non-empty, dequeue vertex,
 *   visit unvisited neighbours, enqueue them
 * Complexity: O(V + E)
 * Domain: network shortest-hop routing, OS module dependency resolution,
 *         GPU task dispatch level-by-level
 */
int *queue_bfs(int **adj, int n, int src, int *out_n) {
    int *order   = malloc(n * sizeof(int));
    bool *visited = calloc(n, sizeof(bool));
    Queue *q     = queue_create((size_t)n);
    int cnt = 0;

    visited[src] = true;
    queue_enqueue(q, src);

    while (!queue_is_empty(q)) {
        int v = queue_dequeue(q);
        order[cnt++] = v;
        /* adj[v][0] = degree, adj[v][1..] = neighbours */
        for (int i = 1; i <= adj[v][0]; i++) {
            int nb = adj[v][i];
            if (!visited[nb]) {
                visited[nb] = true;
                queue_enqueue(q, nb);
            }
        }
    }
    *out_n = cnt;
    queue_destroy(q);
    free(visited);
    return order;
}

/* ── Deque ───────────────────────────────────────────────────────────────── */

Deque *deque_create(size_t cap) {
    Deque *d = malloc(sizeof *d);
    d->data = malloc(cap * sizeof(int));
    d->front = 0; d->back = -1; d->size = 0; d->capacity = cap;
    return d;
}

void deque_destroy(Deque *d) { free(d->data); free(d); }

bool deque_push_back(Deque *d, int val) {
    if (d->size == d->capacity) return false;
    d->back = (d->back + 1) % (int)d->capacity;
    d->data[d->back] = val; d->size++;
    return true;
}

bool deque_push_front(Deque *d, int val) {
    if (d->size == d->capacity) return false;
    d->front = (d->front - 1 + (int)d->capacity) % (int)d->capacity;
    d->data[d->front] = val; d->size++;
    return true;
}

int deque_pop_front(Deque *d) {
    assert(d->size > 0);
    int val = d->data[d->front];
    d->front = (d->front + 1) % (int)d->capacity;
    d->size--; return val;
}

int deque_pop_back(Deque *d) {
    assert(d->size > 0);
    int val = d->data[d->back];
    d->back = (d->back - 1 + (int)d->capacity) % (int)d->capacity;
    d->size--; return val;
}

int  deque_front(const Deque *d)     { assert(d->size>0); return d->data[d->front]; }
int  deque_back(const Deque *d)      { assert(d->size>0); return d->data[d->back]; }
bool deque_is_empty(const Deque *d)  { return d->size == 0; }

/*
 * Monotonic deque sliding window maximum — O(n) total
 *
 * Invariant: deque stores INDICES in decreasing order of their values.
 *   - Front of deque = index of window maximum (always)
 *   - Before adding index i:
 *       1. Pop front indices outside [i-k+1, i]  (expired)
 *       2. Pop back indices with value ≤ arr[i]   (dominated — can never be max)
 *   - Each index is pushed and popped at most once → O(n) total
 *
 * Domain: fq_codel RTT min tracking, GPU max-pool layer, SQL OVER(ROWS...)
 */
int *deque_sliding_window_max(const int *arr, int n, int k, int *out_len) {
    if (n == 0 || k <= 0 || k > n) { *out_len = 0; return NULL; }
    int len = n - k + 1;
    int *result = malloc(len * sizeof(int));
    int *dq     = malloc(n  * sizeof(int));  /* stores indices */
    int front = 0, back = -1, ri = 0;

    for (int i = 0; i < n; i++) {
        /* step 1: expire indices outside window */
        while (front <= back && dq[front] < i - k + 1) front++;

        /* step 2: pop dominated back elements */
        while (front <= back && arr[dq[back]] <= arr[i]) back--;

        dq[++back] = i;

        /* record max once window is full */
        if (i >= k - 1) result[ri++] = arr[dq[front]];
    }
    *out_len = len;
    free(dq);
    return result;
}
