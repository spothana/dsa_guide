/*
 * ring_buffer.h — Lock-free SPSC ring buffer
 *
 * Algorithms: lock-free SPSC, memory-mapped I/O pattern, NAPI polling
 * Domain use: eBPF ring, NIC RX/TX descriptor ring, WAL journal ring
 */
#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>

typedef struct {
    int           *buf;
    size_t         capacity;        /* must be power of 2 */
    atomic_size_t  head;            /* consumer reads here */
    atomic_size_t  tail;            /* producer writes here */
} RingBuffer;

RingBuffer *rb_create(size_t capacity);   /* capacity rounded to next pow2 */
void        rb_destroy(RingBuffer *rb);
bool        rb_produce(RingBuffer *rb, int val);   /* single producer */
bool        rb_consume(RingBuffer *rb, int *out);  /* single consumer */
size_t      rb_size(const RingBuffer *rb);
bool        rb_is_empty(const RingBuffer *rb);
bool        rb_is_full(const RingBuffer *rb);
