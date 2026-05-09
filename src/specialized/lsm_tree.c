/*
 * lsm_tree.c — Log-Structured Merge tree (2-level for clarity)
 */
#include "specialized/lsm_tree.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int entry_cmp(const void *a, const void *b) {
    return ((LSMEntry*)a)->key - ((LSMEntry*)b)->key;
}

LSMTree *lsm_create(void) {
    LSMTree *t = calloc(1, sizeof *t);
    for (int i=0;i<LSM_LEVEL_MAX;i++) {
        t->levels[i] = calloc(1, sizeof(SSTable));
        t->levels[i]->capacity = 64<<(i*2);
        t->levels[i]->entries = malloc(t->levels[i]->capacity*sizeof(LSMEntry));
        t->levels[i]->count = 0;
    }
    return t;
}

void lsm_destroy(LSMTree *t) {
    for (int i=0;i<LSM_LEVEL_MAX;i++) { free(t->levels[i]->entries); free(t->levels[i]); }
    free(t);
}

/*
 * Write path: always append to memtable — O(1)
 * When memtable is full, flush to level 0 SSTable (sorted).
 *
 * This is why LSM trees are write-optimised: sequential append
 * outperforms random B-tree updates by avoiding random I/O.
 * Domain: RocksDB, LevelDB, Cassandra, eBPF audit log
 */
static void flush_memtable(LSMTree *t) {
    SSTable *l0 = t->levels[0];
    if (l0->count + t->mem_count > l0->capacity) {
        l0->capacity *= 2;
        l0->entries = realloc(l0->entries, l0->capacity*sizeof(LSMEntry));
    }
    memcpy(l0->entries + l0->count, t->mem, t->mem_count*sizeof(LSMEntry));
    l0->count += t->mem_count;
    /* sort level 0 after each flush */
    qsort(l0->entries, l0->count, sizeof(LSMEntry), entry_cmp);
    l0->sorted = true;
    t->mem_count = 0;
}

void lsm_put(LSMTree *t, int key, int val) {
    if (t->mem_count == LSM_MEM_MAX) flush_memtable(t);
    t->mem[t->mem_count++] = (LSMEntry){.key=key,.val=val,.deleted=false};
}

void lsm_delete(LSMTree *t, int key) {
    if (t->mem_count == LSM_MEM_MAX) flush_memtable(t);
    t->mem[t->mem_count++] = (LSMEntry){.key=key,.val=0,.deleted=true};
}

/*
 * Read path: memtable → level 0 → level 1 → …
 * Most recent write wins. Tombstones mask older entries.
 * This is the read amplification cost of LSM trees.
 */
bool lsm_get(LSMTree *t, int key, int *val_out) {
    /* check memtable (newest) first — linear scan */
    for (int i=(int)t->mem_count-1; i>=0; i--) {
        if (t->mem[i].key==key) {
            if (t->mem[i].deleted) return false;
            *val_out = t->mem[i].val; return true;
        }
    }
    /* check each level — binary search since sorted */
    for (int lvl=0; lvl<LSM_LEVEL_MAX; lvl++) {
        SSTable *ss = t->levels[lvl];
        if (!ss->sorted || ss->count==0) continue;
        int lo=0, hi=(int)ss->count-1;
        while (lo<=hi) {
            int mid=(lo+hi)/2;
            if      (ss->entries[mid].key==key) {
                if (ss->entries[mid].deleted) return false;
                *val_out=ss->entries[mid].val; return true;
            }
            else if (ss->entries[mid].key<key) lo=mid+1;
            else hi=mid-1;
        }
    }
    return false;
}

/*
 * Compaction: merge level i into level i+1 — O(n log n)
 * Eliminates duplicate keys (keep newest) and removes tombstones.
 * Prevents read amplification from growing unboundedly.
 */
void lsm_compact(LSMTree *t, int level) {
    if (level >= LSM_LEVEL_MAX-1) return;
    SSTable *src = t->levels[level], *dst = t->levels[level+1];
    if (src->count == 0) return;
    /* merge (src is sorted, dst is sorted) */
    size_t new_cap = src->count + dst->count;
    LSMEntry *merged = malloc(new_cap * sizeof(LSMEntry));
    size_t i=0, j=0, k=0;
    while (i<src->count && j<dst->count) {
        if (src->entries[i].key <= dst->entries[j].key) {
            if (i>0 && merged[k-1].key==src->entries[i].key) { i++; continue; }
            merged[k++] = src->entries[i++];
        } else merged[k++] = dst->entries[j++];
    }
    while (i<src->count) merged[k++] = src->entries[i++];
    while (j<dst->count) merged[k++] = dst->entries[j++];
    /* remove tombstones at compaction time */
    size_t clean=0;
    for (size_t m=0;m<k;m++) if (!merged[m].deleted) merged[clean++]=merged[m];
    if (dst->capacity < clean) {
        dst->capacity = clean*2;
        dst->entries = realloc(dst->entries, dst->capacity*sizeof(LSMEntry));
    }
    memcpy(dst->entries, merged, clean*sizeof(LSMEntry));
    dst->count = clean; dst->sorted = true;
    src->count = 0;
    free(merged);
}

void lsm_print(const LSMTree *t) {
    printf("LSMTree:\n  memtable (%zu entries):\n", t->mem_count);
    for (size_t i=0;i<t->mem_count;i++)
        printf("    k=%d v=%d%s\n", t->mem[i].key, t->mem[i].val, t->mem[i].deleted?" [DEL]":"");
    for (int lvl=0;lvl<LSM_LEVEL_MAX;lvl++) {
        printf("  level%d (%zu entries):\n", lvl, t->levels[lvl]->count);
        for (size_t i=0;i<t->levels[lvl]->count&&i<8;i++)
            printf("    k=%d v=%d%s\n", t->levels[lvl]->entries[i].key,
                   t->levels[lvl]->entries[i].val, t->levels[lvl]->entries[i].deleted?" [DEL]":"");
    }
}
