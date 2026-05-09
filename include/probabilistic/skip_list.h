/*
 * skip_list.h — Probabilistic skip list
 *
 * Algorithms: O(log n) expected insert/search/delete, probabilistic balancing
 * Domain use: LevelDB/RocksDB memtable, Redis sorted sets, ordered audit log
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>

#define SKIP_MAX_LEVEL 16
#define SKIP_P         0.5   /* level promotion probability */

typedef struct SLNode {
    int            key;
    int            val;
    int            level;
    struct SLNode *forward[];   /* flexible array of forward pointers */
} SLNode;

typedef struct {
    SLNode *header;
    int     max_level;
    int     cur_level;
    size_t  size;
} SkipList;

SkipList *sl_create(void);
void      sl_destroy(SkipList *sl);

void      sl_insert(SkipList *sl, int key, int val);   /* O(log n) expected */
bool      sl_search(SkipList *sl, int key, int *val_out);
bool      sl_delete(SkipList *sl, int key);

/* Range scan [lo, hi] — calls visit for each key in range */
void      sl_range(SkipList *sl, int lo, int hi, void (*visit)(int k, int v));
void      sl_print(const SkipList *sl);
