/*
 * cuckoo_hash.h — Cuckoo hash table (two tables, O(1) worst-case lookup)
 *
 * Algorithms: O(1) worst-case lookup, two-table displacement, parallel cuckoo
 * Domain use: eBPF maps, flow table fast lookup, GPU hash table,
 *             fast IP/MAC lookup in firewall
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>

#define CUCKOO_EMPTY  (-1)
#define CUCKOO_MAX_KICKS 128   /* max displacements before rehash */

typedef struct {
    int *keys1, *vals1;   /* table 1 */
    int *keys2, *vals2;   /* table 2 */
    size_t capacity;      /* per-table capacity */
    size_t size;
} CuckooHash;

CuckooHash *cuckoo_create(size_t capacity);
void        cuckoo_destroy(CuckooHash *ch);

/*
 * insert: O(1) amortised — may trigger rehash
 * Returns false if rehash fails (caller should grow and retry)
 */
bool cuckoo_insert(CuckooHash *ch, int key, int val);

/* lookup: O(1) worst case — checks exactly 2 locations */
bool cuckoo_get(CuckooHash *ch, int key, int *val_out);

/* delete: O(1) */
bool cuckoo_delete(CuckooHash *ch, int key);

void cuckoo_print(const CuckooHash *ch);
