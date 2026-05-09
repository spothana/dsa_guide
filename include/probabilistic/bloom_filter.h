/*
 * bloom_filter.h — Bloom filter + counting bloom filter
 *
 * Algorithms: k-hash probabilistic membership, false-positive tuning,
 *             counting bloom
 * Domain use: eBPF fast negative lookup, CDN cache miss pre-check,
 *             LSM level skip (RocksDB/Cassandra), malware URL screening
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t *bits;
    size_t   bit_count;
    int      num_hashes;    /* k */
    size_t   count;         /* items inserted (approx) */
} BloomFilter;

/*
 * Create a bloom filter for expected n items and target false-positive rate p.
 * Automatically computes optimal bit_count and num_hashes.
 */
BloomFilter *bloom_create(size_t n, double p);
void         bloom_destroy(BloomFilter *bf);

void         bloom_add(BloomFilter *bf, const void *data, size_t len);
bool         bloom_test(const BloomFilter *bf, const void *data, size_t len);
double       bloom_fp_rate(const BloomFilter *bf);   /* current estimated FP rate */

/* Counting bloom filter (supports deletion) */
typedef struct {
    uint8_t *counts;    /* 4-bit counters packed 2-per-byte */
    size_t   slots;
    int      num_hashes;
} CountingBloom;

CountingBloom *cbloom_create(size_t n, double p);
void           cbloom_destroy(CountingBloom *cb);
void           cbloom_add(CountingBloom *cb, const void *data, size_t len);
bool           cbloom_remove(CountingBloom *cb, const void *data, size_t len);
bool           cbloom_test(const CountingBloom *cb, const void *data, size_t len);
