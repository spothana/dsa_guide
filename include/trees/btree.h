/*
 * btree.h — B-tree (order T: each node has T-1..2T-1 keys)
 *
 * Algorithms: O(log n) disk-friendly ops, fan-out optimisation, bulk load
 * Domain use: ext4/xfs/btrfs/NTFS index, InnoDB/Postgres primary index,
 *             FIB prefix table, APFS
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>

#define BTREE_T 3   /* minimum degree — each node has T-1..2T-1 keys */

typedef struct BTreeNode {
    int              keys[2*BTREE_T - 1];
    struct BTreeNode *children[2*BTREE_T];
    int              n;       /* current number of keys */
    bool             leaf;
} BTreeNode;

typedef struct {
    BTreeNode *root;
    size_t     size;
} BTree;

BTree     *bt_create(void);
void       bt_destroy(BTree *t);
void       bt_insert(BTree *t, int key);
bool       bt_search(BTree *t, int key);
void       bt_delete(BTree *t, int key);
void       bt_print(const BTree *t);          /* pretty tree dump */
bool       bt_range_scan(BTree *t, int lo, int hi,
                         void (*visit)(int key)); /* in-order range */
