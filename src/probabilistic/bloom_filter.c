/*
 * bloom_filter.c — Bloom filter and counting bloom filter
 */
#include "probabilistic/bloom_filter.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* MurmurHash3 finaliser — fast, good distribution */
static uint64_t murmur_mix(uint64_t h) {
    h ^= h >> 33; h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33; h *= 0xc4ceb9fe1a85ec53ULL;
    h ^= h >> 33; return h;
}

/* k-th hash of data using double hashing: h_k = h1 + k*h2 */
static size_t bloom_hash(const void *data, size_t len, int k, size_t bits) {
    uint64_t h1=0, h2=0;
    const uint8_t *p = data;
    for (size_t i=0;i<len;i++) { h1 ^= (uint64_t)p[i]*(i+1); h2 ^= (uint64_t)p[i]; }
    h1 = murmur_mix(h1); h2 = murmur_mix(h2);
    return (size_t)((h1 + (uint64_t)k * h2) % bits);
}

/*
 * Optimal parameters given n items and false-positive rate p:
 *   m (bits)      = -n * ln(p) / (ln 2)²
 *   k (hashes)    = (m/n) * ln 2
 *
 * Domain: RocksDB level skip (avoids expensive SSTable lookup),
 *         CDN pre-check before DB query, eBPF flow fast-reject
 */
BloomFilter *bloom_create(size_t n, double p) {
    size_t m = (size_t)ceil(-((double)n * log(p)) / (log(2)*log(2)));
    int    k = (int)ceil((double)m / n * log(2));
    BloomFilter *bf = malloc(sizeof *bf);
    bf->bit_count = m;
    bf->num_hashes = k;
    bf->count = 0;
    bf->bits = calloc((m+7)/8, 1);
    return bf;
}

void bloom_destroy(BloomFilter *bf) { free(bf->bits); free(bf); }

static void bit_set(uint8_t *bits, size_t i)        { bits[i/8] |=  (1<<(i%8)); }
static bool bit_get(const uint8_t *bits, size_t i)  { return !!(bits[i/8]&(1<<(i%8))); }

void bloom_add(BloomFilter *bf, const void *data, size_t len) {
    for (int k=0; k<bf->num_hashes; k++)
        bit_set(bf->bits, bloom_hash(data, len, k, bf->bit_count));
    bf->count++;
}

bool bloom_test(const BloomFilter *bf, const void *data, size_t len) {
    for (int k=0; k<bf->num_hashes; k++)
        if (!bit_get(bf->bits, bloom_hash(data, len, k, bf->bit_count))) return false;
    return true;  /* may be false positive */
}

double bloom_fp_rate(const BloomFilter *bf) {
    return pow(1.0 - exp(-(double)bf->num_hashes * bf->count / bf->bit_count),
               bf->num_hashes);
}

/* Counting bloom filter — supports deletion */
CountingBloom *cbloom_create(size_t n, double p) {
    size_t m = (size_t)ceil(-((double)n*log(p))/(log(2)*log(2)));
    int k = (int)ceil((double)m/n*log(2));
    CountingBloom *cb = malloc(sizeof *cb);
    cb->slots = m; cb->num_hashes = k;
    cb->counts = calloc((m+1)/2, 1);
    return cb;
}

void cbloom_destroy(CountingBloom *cb) { free(cb->counts); free(cb); }

static int cbloom_get(CountingBloom *cb, size_t i) {
    return (cb->counts[i/2] >> ((i%2)*4)) & 0xF;
}
static void cbloom_inc(CountingBloom *cb, size_t i) {
    int v = cbloom_get(cb, i);
    if (v < 15) { cb->counts[i/2] += (uint8_t)(1 << ((i%2)*4)); }
}
static void cbloom_dec(CountingBloom *cb, size_t i) {
    int v = cbloom_get(cb, i);
    if (v > 0)  { cb->counts[i/2] -= (uint8_t)(1 << ((i%2)*4)); }
}

void cbloom_add(CountingBloom *cb, const void *data, size_t len) {
    for (int k=0;k<cb->num_hashes;k++) cbloom_inc(cb, bloom_hash(data,len,k,cb->slots));
}
bool cbloom_remove(CountingBloom *cb, const void *data, size_t len) {
    if (!cbloom_test(cb, data, len)) return false;
    for (int k=0;k<cb->num_hashes;k++) cbloom_dec(cb, bloom_hash(data,len,k,cb->slots));
    return true;
}
bool cbloom_test(const CountingBloom *cb, const void *data, size_t len) {
    for (int k=0;k<cb->num_hashes;k++)
        if (cbloom_get((CountingBloom*)cb, bloom_hash(data,len,k,cb->slots))==0) return false;
    return true;
}
