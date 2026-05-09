/*
 * count_min.h — Count-Min sketch
 *
 * Algorithms: sub-linear frequency estimation, heavy hitters, mergeable sketches
 * Domain use: DDoS traffic profiling, hot block tracking, cardinality estimation,
 *             eBPF heavy-hitter detection, network anomaly detection
 */
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t **table;   /* d rows × w columns */
    int        d;       /* number of hash functions */
    int        w;       /* width (number of buckets) */
    uint64_t   total;   /* total items inserted */
} CountMin;

/*
 * Create with error ε (0<ε<1) and confidence δ (0<δ<1).
 * w = ceil(e/ε), d = ceil(ln(1/δ))
 */
CountMin *cm_create(double epsilon, double delta);
void      cm_destroy(CountMin *cm);

void      cm_add(CountMin *cm, const void *data, size_t len, uint32_t count);
uint32_t  cm_query(const CountMin *cm, const void *data, size_t len);

/* Heavy hitters: return keys with estimated count ≥ threshold.
 * (Simplified version using integer keys.) */
void      cm_add_int(CountMin *cm, int key);
uint32_t  cm_query_int(const CountMin *cm, int key);

/* Merge two sketches of equal dimensions */
bool      cm_merge(CountMin *dst, const CountMin *src);

/*
 * hyperloglog.h — HyperLogLog cardinality estimator
 *
 * Algorithms: O(1) space cardinality estimation, stochastic averaging,
 *             LogLog correction
 * Domain use: COUNT DISTINCT, unique IP counting, eBPF unique flow counter,
 *             dataset cardinality profiling
 */
typedef struct {
    uint8_t *registers;   /* M = 2^b registers */
    int      b;           /* b bits of precision (4..16) */
    int      m;           /* m = 2^b registers */
    double   alpha;       /* bias correction constant */
} HyperLogLog;

/*
 * Create with b bits of precision.
 * Error ≈ 1.04/sqrt(2^b). b=12 → ~1.6% error, 4KB space.
 */
HyperLogLog *hll_create(int b);
void         hll_destroy(HyperLogLog *hll);

void         hll_add(HyperLogLog *hll, const void *data, size_t len);
uint64_t     hll_estimate(const HyperLogLog *hll);   /* cardinality estimate */
void         hll_merge(HyperLogLog *dst, const HyperLogLog *src);
void         hll_reset(HyperLogLog *hll);
