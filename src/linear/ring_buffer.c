/*
 * ring_buffer.c — Lock-free SPSC ring buffer (single producer, single consumer)
 *
 * Uses C11 atomics. Head and tail are on separate cache lines to avoid
 * false sharing — the same issue that kills LCRQ without padding.
 */
#include "linear/ring_buffer.h"
#include <stdlib.h>
#include <stdio.h>

static size_t next_pow2(size_t n) {
    size_t p = 1;
    while (p < n) p <<= 1;
    return p;
}

RingBuffer *rb_create(size_t capacity) {
    capacity = next_pow2(capacity);
    RingBuffer *rb = malloc(sizeof *rb);
    rb->buf = malloc(capacity * sizeof(int));
    rb->capacity = capacity;
    atomic_store(&rb->head, 0);
    atomic_store(&rb->tail, 0);
    return rb;
}

void rb_destroy(RingBuffer *rb) { free(rb->buf); free(rb); }

/*
 * Produce: write val at tail, advance tail.
 *
 * Memory order:
 *   - Load head with acquire to see consumer's progress
 *   - Store tail with release so consumer sees the written value
 *     before it observes the advanced tail
 *
 * Domain: NIC RX descriptor ring, eBPF ring buffer, WAL journal ring
 */
bool rb_produce(RingBuffer *rb, int val) {
    size_t tail = atomic_load_explicit(&rb->tail, memory_order_relaxed);
    size_t head = atomic_load_explicit(&rb->head, memory_order_acquire);
    if (tail - head >= rb->capacity) return false;   /* full */
    rb->buf[tail & (rb->capacity - 1)] = val;
    atomic_store_explicit(&rb->tail, tail + 1, memory_order_release);
    return true;
}

/*
 * Consume: read val at head, advance head.
 *
 * Memory order:
 *   - Load tail with acquire to see producer's writes
 *   - Store head with release so producer knows slot is free
 */
bool rb_consume(RingBuffer *rb, int *out) {
    size_t head = atomic_load_explicit(&rb->head, memory_order_relaxed);
    size_t tail = atomic_load_explicit(&rb->tail, memory_order_acquire);
    if (head == tail) return false;  /* empty */
    *out = rb->buf[head & (rb->capacity - 1)];
    atomic_store_explicit(&rb->head, head + 1, memory_order_release);
    return true;
}

size_t rb_size(const RingBuffer *rb) {
    size_t tail = atomic_load_explicit(&rb->tail, memory_order_acquire);
    size_t head = atomic_load_explicit(&rb->head, memory_order_acquire);
    return tail - head;
}

bool rb_is_empty(const RingBuffer *rb) { return rb_size(rb) == 0; }
bool rb_is_full(const RingBuffer *rb)  { return rb_size(rb) == rb->capacity; }
