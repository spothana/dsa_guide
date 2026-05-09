/*
 * array.h — Dynamic array (buffer)
 *
 * Algorithms: binary search, two-pointer, sliding window max, prefix sum
 * Domain use: ring buffers (NIC, eBPF), tensor storage, column store pages
 */
#pragma once
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    int    *data;
    size_t  size;
    size_t  capacity;
} Array;

/* lifecycle */
Array  *array_create(size_t initial_capacity);
void    array_destroy(Array *a);
bool    array_push(Array *a, int val);
int     array_pop(Array *a);
int     array_get(Array *a, size_t idx);
void    array_set(Array *a, size_t idx, int val);

/* algorithms */

/* Binary search — O(log n), array must be sorted */
int  array_binary_search(const Array *a, int target);

/* Two-pointer: find pair summing to target — O(n), sorted array */
bool array_two_pointer_pair(const Array *a, int target, int *i_out, int *j_out);

/* Sliding window maximum — O(n) using monotonic deque */
Array *array_sliding_window_max(const Array *a, int k);

/* Prefix sum build + range query — O(n) build, O(1) query */
Array *array_prefix_sum_build(const Array *a);
int    array_prefix_sum_query(const Array *prefix, int l, int r);
