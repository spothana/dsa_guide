/*
 * count_min.c — Count-Min sketch + HyperLogLog
 */
#include "probabilistic/count_min.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

/* ── Count-Min sketch ────────────────────────────────────────────────────── */

static uint32_t cm_hash(const void *data, size_t len, int seed) {
    uint32_t h = (uint32_t)(seed * 2654435761U);
    const uint8_t *p = data;
    for (size_t i=0;i<len;i++) h = (h ^ p[i]) * 2654435761U;
    return h;
}

/*
 * Create with error bound ε and confidence 1-δ:
 *   w = ceil(e/ε)  (width)
 *   d = ceil(ln(1/δ)) (depth = number of hash functions)
 *
 * Query always returns ≥ true count (never underestimates).
 * With probability ≥ 1-δ, overestimate ≤ ε * total_count.
 *
 * Domain: DDoS heavy-hitter detection, eBPF top-flow tracking,
 *         Postgres cardinality estimation, network traffic profiling
 */
CountMin *cm_create(double epsilon, double delta) {
    int w = (int)ceil(2.71828/epsilon);
    int d = (int)ceil(log(1.0/delta));
    CountMin *cm = malloc(sizeof *cm);
    cm->w=w; cm->d=d; cm->total=0;
    cm->table = malloc(d * sizeof(uint32_t*));
    for (int i=0;i<d;i++) cm->table[i] = calloc(w, sizeof(uint32_t));
    return cm;
}

void cm_destroy(CountMin *cm) {
    for (int i=0;i<cm->d;i++) free(cm->table[i]);
    free(cm->table); free(cm);
}

void cm_add(CountMin *cm, const void *data, size_t len, uint32_t count) {
    for (int i=0;i<cm->d;i++) {
        uint32_t h = cm_hash(data, len, i) % (uint32_t)cm->w;
        cm->table[i][h] += count;
    }
    cm->total += count;
}

uint32_t cm_query(const CountMin *cm, const void *data, size_t len) {
    uint32_t min = UINT32_MAX;
    for (int i=0;i<cm->d;i++) {
        uint32_t h = cm_hash(data, len, i) % (uint32_t)cm->w;
        if (cm->table[i][h] < min) min = cm->table[i][h];
    }
    return min;
}

void cm_add_int(CountMin *cm, int key) { cm_add(cm, &key, sizeof key, 1); }
uint32_t cm_query_int(const CountMin *cm, int key) { return cm_query(cm, &key, sizeof key); }

bool cm_merge(CountMin *dst, const CountMin *src) {
    if (dst->w!=src->w || dst->d!=src->d) return false;
    for (int i=0;i<dst->d;i++)
        for (int j=0;j<dst->w;j++) dst->table[i][j]+=src->table[i][j];
    dst->total+=src->total;
    return true;
}

/* ── HyperLogLog ─────────────────────────────────────────────────────────── */

static uint64_t hll_hash(const void *data, size_t len) {
    /* MurmurHash3 finaliser mix — stronger avalanche than FNV */
    const uint8_t *p = data;
    uint64_t h = 0xcbf29ce484222325ULL ^ (uint64_t)len;
    for (size_t i = 0; i < len; i++) {
        h ^= (uint64_t)p[i];
        h *= 0x100000001b3ULL;
    }
    /* final mix */
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53ULL;
    h ^= h >> 33;
    return h;
}

static int clz64(uint64_t x) {
    if (x==0) return 64;
#ifdef __GNUC__
    return __builtin_clzll(x);
#else
    int n=0; while (!(x&(1ULL<<63))) { n++; x<<=1; } return n;
#endif
}

/*
 * HyperLogLog cardinality estimator — O(1) space per estimate
 *
 * Algorithm:
 *   1. Hash each element to 64 bits
 *   2. Use top b bits as register index
 *   3. Store max number of leading zeros in that register
 *   4. Combine registers with harmonic mean → cardinality estimate
 *
 * Error ≈ 1.04/sqrt(m) where m = 2^b registers
 * b=12 → ~4096 registers, ~4KB memory, ~1.6% error
 *
 * Domain: COUNT DISTINCT in Postgres/ClickHouse, unique IP counting,
 *         eBPF unique flow counter, dataset size estimation in ML pipelines
 */
HyperLogLog *hll_create(int b) {
    HyperLogLog *hll = malloc(sizeof *hll);
    hll->b = b; hll->m = 1<<b;
    hll->registers = calloc(hll->m, 1);
    /* alpha bias correction constants */
    if      (b==4) hll->alpha = 0.673;
    else if (b==5) hll->alpha = 0.697;
    else if (b==6) hll->alpha = 0.709;
    else            hll->alpha = 0.7213 / (1.0 + 1.079/hll->m);
    return hll;
}

void hll_destroy(HyperLogLog *hll) { free(hll->registers); free(hll); }

void hll_add(HyperLogLog *hll, const void *data, size_t len) {
    uint64_t h = hll_hash(data, len);
    int reg = (int)(h >> (64 - hll->b));          /* top b bits = register index */
    uint64_t w = h << hll->b;                      /* remaining bits */
    uint8_t rank = (uint8_t)(clz64(w) + 1);        /* position of leftmost 1-bit */
    if (rank > hll->registers[reg]) hll->registers[reg] = rank;
}

uint64_t hll_estimate(const HyperLogLog *hll) {
    double Z = 0.0;
    int V = 0;
    for (int i=0;i<hll->m;i++) {
        Z += 1.0 / (double)(1ULL << hll->registers[i]);
        if (hll->registers[i]==0) V++;
    }
    double E = hll->alpha * (double)hll->m * (double)hll->m / Z;
    /* small-range correction: use linear counting when many registers are empty */
    if (E <= 2.5 * hll->m && V > 0)
        E = (double)hll->m * log((double)hll->m / (double)V);
    /* large-range correction */
    else if (E > (1ULL << 32) / 30.0) {
        double two32 = 4294967296.0;
        E = -two32 * log(1.0 - E / two32);
    }
    return (uint64_t)E;
}

void hll_merge(HyperLogLog *dst, const HyperLogLog *src) {
    for (int i=0;i<dst->m;i++)
        if (src->registers[i]>dst->registers[i]) dst->registers[i]=src->registers[i];
}

void hll_reset(HyperLogLog *hll) { memset(hll->registers, 0, hll->m); }
