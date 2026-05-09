/*
 * red_black_tree.h — Left-leaning red-black BST
 *
 * Algorithms: O(log n) insert/delete/search, leftmost cache, in-order traversal
 * Domain use: CFS run queue (Linux scheduler), VMA intervals, InnoDB index,
 *             extent tree (btrfs), decision tree nodes
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef enum { RB_RED, RB_BLACK } RBColor;

typedef struct RBNode {
    int            key;
    int            val;
    RBColor        color;
    struct RBNode *left, *right, *parent;
} RBNode;

typedef struct {
    RBNode *root;
    RBNode *nil;        /* sentinel nil node (black) */
    RBNode *leftmost;   /* cached min — O(1) min lookup (CFS pattern) */
    size_t  size;
} RBTree;

/* lifecycle */
RBTree *rbt_create(void);
void    rbt_destroy(RBTree *t);

/* core operations — all O(log n) */
void    rbt_insert(RBTree *t, int key, int val);
void    rbt_delete(RBTree *t, int key);
RBNode *rbt_search(RBTree *t, int key);
RBNode *rbt_minimum(RBTree *t);       /* O(1) via leftmost cache */
RBNode *rbt_maximum(RBTree *t);       /* O(log n) */
RBNode *rbt_successor(RBTree *t, RBNode *node);

/* traversal */
void    rbt_inorder(const RBTree *t, void (*visit)(RBNode *));

/* validation (teaching/testing) */
bool    rbt_is_valid(const RBTree *t);
void    rbt_print(const RBTree *t);
