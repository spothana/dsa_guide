/*
 * array.c — Dynamic array with binary search, two-pointer,
 *            sliding window max (monotonic deque), prefix sum
 */
#include "linear/array.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* ── Lifecycle ──────────────────────────────────────────────────────────── */

Array *array_create(size_t cap) {
    Array *a = malloc(sizeof *a);
    a->data = malloc(cap * sizeof(int));
    a->size = 0;
    a->capacity = cap;
    return a;
}

void array_destroy(Array *a) {
    if (!a) return;
    free(a->data);
    free(a);
}

bool array_push(Array *a, int val) {
    if (a->size == a->capacity) {
        a->capacity *= 2;
        a->data = realloc(a->data, a->capacity * sizeof(int));
        if (!a->data) return false;
    }
    a->data[a->size++] = val;
    return true;
}

int array_pop(Array *a) {
    assert(a->size > 0);
    return a->data[--a->size];
}

int array_get(Array *a, size_t idx) {
    assert(idx < a->size);
    return a->data[idx];
}

void array_set(Array *a, size_t idx, int val) {
    assert(idx < a->size);
    a->data[idx] = val;
}

/* ── Binary search ──────────────────────────────────────────────────────── */
/*
 * Algorithm: classic binary search
 * Invariant: answer is always in [lo, hi]
 * Complexity: O(log n) time, O(1) space
 *
 * Domain: page cache XArray lookup, FIB sorted table, sorted column scan
 */
int array_binary_search(const Array *a, int target) {
    int lo = 0, hi = (int)a->size - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;   /* avoids integer overflow */
        if (a->data[mid] == target) return mid;
        if (a->data[mid] <  target) lo = mid + 1;
        else                         hi = mid - 1;
    }
    return -1;  /* not found */
}

/* ── Two-pointer pair sum ────────────────────────────────────────────────── */
/*
 * Algorithm: two-pointer
 * Idea: sorted array → left pointer moves right when sum too small,
 *       right pointer moves left when sum too large
 * Complexity: O(n) time, O(1) space (vs O(n²) brute force)
 *
 * Domain: network packet pair matching, container-with-most-water
 */
bool array_two_pointer_pair(const Array *a, int target,
                             int *i_out, int *j_out) {
    int lo = 0, hi = (int)a->size - 1;
    while (lo < hi) {
        int sum = a->data[lo] + a->data[hi];
        if      (sum == target) { *i_out = lo; *j_out = hi; return true; }
        else if (sum <  target)  lo++;
        else                     hi--;
    }
    return false;
}

/* ── Sliding window maximum (monotonic deque) ───────────────────────────── */
/*
 * Algorithm: monotonic deque
 *
 * Key insight: maintain a deque of INDICES such that:
 *   - values are monotonically decreasing (front = max of window)
 *   - indices outside the window are popped from the front
 *   - smaller values are popped from the back when a larger arrives
 *     (they can never be the window max while the larger element exists)
 *
 * Each element is pushed and popped at most once → O(n) total
 *
 * Domain: fq_codel min-RTT tracking, GPU max-pooling, windowed MAX() SQL
 */
Array *array_sliding_window_max(const Array *a, int k) {
    int n = (int)a->size;
    if (n == 0 || k <= 0 || k > n) return NULL;

    Array *result = array_create(n - k + 1);
    int *deque = malloc(n * sizeof(int));  /* stores indices */
    int front = 0, back = -1;

    for (int i = 0; i < n; i++) {
        /* pop indices outside window from front */
        while (front <= back && deque[front] < i - k + 1)
            front++;

        /* pop indices whose values are smaller than current from back
         * (they can never be max while a[i] is in the window) */
        while (front <= back && a->data[deque[back]] < a->data[i])
            back--;

        deque[++back] = i;

        /* window is full — record max (front of deque) */
        if (i >= k - 1)
            array_push(result, a->data[deque[front]]);
    }

    free(deque);
    return result;
}

/* ── Prefix sum ─────────────────────────────────────────────────────────── */
/*
 * Algorithm: prefix sum array (also called cumulative sum / scan)
 * Build: prefix[i] = a[0] + a[1] + ... + a[i-1]  (prefix[0] = 0)
 * Query: sum(l,r) = prefix[r+1] - prefix[l]       O(1)
 *
 * Domain: GPU parallel reduction prefix scan, database column aggregation,
 *         network traffic byte counting over time windows
 */
Array *array_prefix_sum_build(const Array *a) {
    Array *prefix = array_create(a->size + 1);
    array_push(prefix, 0);
    for (size_t i = 0; i < a->size; i++)
        array_push(prefix, prefix->data[i] + a->data[i]);
    return prefix;
}

int array_prefix_sum_query(const Array *prefix, int l, int r) {
    assert(l >= 0 && r < (int)prefix->size - 1 && l <= r);
    return prefix->data[r + 1] - prefix->data[l];
}
