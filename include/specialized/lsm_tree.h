/*
 * lsm_tree.h — Log-Structured Merge tree (simplified 2-level)
 *
 * Algorithms: leveled compaction, tiered compaction, bloom filter per level
 * Domain use: RocksDB, LevelDB, Cassandra SSTable, write-optimised KV store,
 *             eBPF map persistence, audit log write path
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>

#define LSM_MEM_MAX    8      /* memtable max entries before flush */
#define LSM_LEVEL_MAX  4      /* number of levels */

typedef struct {
    int key;
    int val;
    bool deleted;   /* tombstone */
} LSMEntry;

typedef struct {
    LSMEntry *entries;
    size_t    count;
    size_t    capacity;
    bool      sorted;
} SSTable;   /* Sorted String Table */

typedef struct {
    LSMEntry  mem[LSM_MEM_MAX];   /* in-memory write buffer (memtable) */
    size_t    mem_count;
    SSTable  *levels[LSM_LEVEL_MAX];
    size_t    level_count[LSM_LEVEL_MAX];
} LSMTree;

LSMTree *lsm_create(void);
void     lsm_destroy(LSMTree *t);

/* write: always goes to memtable — O(1) amortised */
void     lsm_put(LSMTree *t, int key, int val);
void     lsm_delete(LSMTree *t, int key);        /* inserts tombstone */

/* read: memtable → level 0 → level 1 → … — O(log n) per level */
bool     lsm_get(LSMTree *t, int key, int *val_out);

/* compaction: merge level i into level i+1 — O(n log n) */
void     lsm_compact(LSMTree *t, int level);

void     lsm_print(const LSMTree *t);
