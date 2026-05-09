/*
 * skip_list.c — Probabilistic skip list
 * O(log n) expected for all operations via probabilistic balancing
 */
#include "probabilistic/skip_list.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <limits.h>

#ifndef INT32_MIN
#define INT32_MIN (-2147483647-1)
#endif

static int random_level(void) {
    int level = 1;
    while ((double)rand()/RAND_MAX < SKIP_P && level < SKIP_MAX_LEVEL) level++;
    return level;
}

static SLNode *slnode_create(int key, int val, int level) {
    SLNode *n = malloc(sizeof(SLNode) + level * sizeof(SLNode*));
    n->key = key; n->val = val; n->level = level;
    for (int i=0;i<level;i++) n->forward[i] = NULL;
    return n;
}

SkipList *sl_create(void) {
    srand((unsigned)time(NULL));
    SkipList *sl = malloc(sizeof *sl);
    sl->header = slnode_create(INT32_MIN, 0, SKIP_MAX_LEVEL);
    sl->max_level = SKIP_MAX_LEVEL;
    sl->cur_level = 1;
    sl->size = 0;
    return sl;
}

void sl_destroy(SkipList *sl) {
    SLNode *cur = sl->header->forward[0];
    while (cur) { SLNode *nx=cur->forward[0]; free(cur); cur=nx; }
    free(sl->header); free(sl);
}

/*
 * Insert — O(log n) expected
 *
 * Algorithm:
 *  1. Find predecessor at each level (update[])
 *  2. Generate random level for new node
 *  3. Splice new node into each level's chain
 *
 * Domain: RocksDB memtable (before flush to SSTable),
 *         Redis sorted set internal structure
 */
void sl_insert(SkipList *sl, int key, int val) {
    SLNode *update[SKIP_MAX_LEVEL];
    SLNode *cur = sl->header;

    for (int i=sl->cur_level-1; i>=0; i--) {
        while (cur->forward[i] && cur->forward[i]->key < key)
            cur = cur->forward[i];
        update[i] = cur;
    }

    cur = cur->forward[0];
    if (cur && cur->key == key) { cur->val = val; return; }

    int new_level = random_level();
    if (new_level > sl->cur_level) {
        for (int i=sl->cur_level; i<new_level; i++) update[i]=sl->header;
        sl->cur_level = new_level;
    }

    SLNode *n = slnode_create(key, val, new_level);
    for (int i=0; i<new_level; i++) {
        n->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = n;
    }
    sl->size++;
}

bool sl_search(SkipList *sl, int key, int *val_out) {
    SLNode *cur = sl->header;
    for (int i=sl->cur_level-1; i>=0; i--)
        while (cur->forward[i] && cur->forward[i]->key < key)
            cur = cur->forward[i];
    cur = cur->forward[0];
    if (cur && cur->key==key) { if(val_out)*val_out=cur->val; return true; }
    return false;
}

bool sl_delete(SkipList *sl, int key) {
    SLNode *update[SKIP_MAX_LEVEL], *cur = sl->header;
    for (int i=sl->cur_level-1; i>=0; i--) {
        while (cur->forward[i] && cur->forward[i]->key < key) cur=cur->forward[i];
        update[i] = cur;
    }
    cur = cur->forward[0];
    if (!cur || cur->key != key) return false;
    for (int i=0; i<sl->cur_level; i++) {
        if (update[i]->forward[i] != cur) break;
        update[i]->forward[i] = cur->forward[i];
    }
    free(cur); sl->size--;
    while (sl->cur_level>1 && !sl->header->forward[sl->cur_level-1]) sl->cur_level--;
    return true;
}

void sl_range(SkipList *sl, int lo, int hi, void (*visit)(int k, int v)) {
    SLNode *cur = sl->header;
    for (int i=sl->cur_level-1; i>=0; i--)
        while (cur->forward[i] && cur->forward[i]->key < lo) cur=cur->forward[i];
    for (cur=cur->forward[0]; cur && cur->key<=hi; cur=cur->forward[0])
        visit(cur->key, cur->val);
}

void sl_print(const SkipList *sl) {
    printf("SkipList (size=%zu levels=%d):\n", sl->size, sl->cur_level);
    for (SLNode *c=sl->header->forward[0]; c; c=c->forward[0])
        printf("  key=%d val=%d lvl=%d\n", c->key, c->val, c->level);
}
