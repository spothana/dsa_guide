/*
 * btree.c — B-tree of minimum degree T
 *
 * Disk-friendly: each node holds up to 2T-1 keys, so a tree of height h
 * touches only h+1 nodes (pages) — minimises disk I/O.
 * All leaves are at the same level (balanced by construction).
 */
#include "trees/btree.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static BTreeNode *btnode_new(bool leaf) {
    BTreeNode *n = calloc(1, sizeof *n);
    n->leaf = leaf; n->n = 0;
    return n;
}

BTree *bt_create(void) {
    BTree *t = malloc(sizeof *t);
    t->root = btnode_new(true);
    t->size = 0;
    return t;
}

static void btnode_free(BTreeNode *n) {
    if (!n) return;
    if (!n->leaf) for (int i=0;i<=n->n;i++) btnode_free(n->children[i]);
    free(n);
}

void bt_destroy(BTree *t) { btnode_free(t->root); free(t); }

bool bt_search(BTree *t, int key) {
    BTreeNode *cur = t->root;
    while (cur) {
        int i = 0;
        while (i < cur->n && key > cur->keys[i]) i++;
        if (i < cur->n && cur->keys[i] == key) return true;
        if (cur->leaf) return false;
        cur = cur->children[i];
    }
    return false;
}

/* Split child[i] of parent node x (child is full: 2T-1 keys) */
static void split_child(BTreeNode *x, int i) {
    BTreeNode *y = x->children[i];   /* full child */
    BTreeNode *z = btnode_new(y->leaf);
    z->n = BTREE_T - 1;

    /* copy right half of y into z */
    for (int j=0; j<BTREE_T-1; j++) z->keys[j] = y->keys[j+BTREE_T];
    if (!y->leaf)
        for (int j=0; j<BTREE_T; j++) z->children[j] = y->children[j+BTREE_T];
    y->n = BTREE_T - 1;

    /* insert z as sibling of y in parent x */
    for (int j=x->n; j>i; j--) x->children[j+1] = x->children[j];
    x->children[i+1] = z;
    for (int j=x->n-1; j>=i; j--) x->keys[j+1] = x->keys[j];
    x->keys[i] = y->keys[BTREE_T-1];   /* median moves up */
    x->n++;
}

/*
 * Insert into a non-full node (proactively split full nodes on the way down)
 * This single-pass top-down approach avoids a second pass upward.
 * Domain: ext4 extent tree insert, InnoDB page split on insert
 */
static void insert_nonfull(BTreeNode *x, int key) {
    int i = x->n - 1;
    if (x->leaf) {
        while (i >= 0 && key < x->keys[i]) { x->keys[i+1]=x->keys[i]; i--; }
        x->keys[i+1] = key; x->n++;
    } else {
        while (i >= 0 && key < x->keys[i]) i--;
        i++;
        if (x->children[i]->n == 2*BTREE_T-1) {
            split_child(x, i);
            if (key > x->keys[i]) i++;
        }
        insert_nonfull(x->children[i], key);
    }
}

void bt_insert(BTree *t, int key) {
    BTreeNode *r = t->root;
    if (r->n == 2*BTREE_T-1) {
        /* root is full — create new root, split old root */
        BTreeNode *s = btnode_new(false);
        s->children[0] = r;
        split_child(s, 0);
        t->root = s;
        insert_nonfull(s, key);
    } else {
        insert_nonfull(r, key);
    }
    t->size++;
}

static int btnode_min(BTreeNode *n) {
    while (!n->leaf) n = n->children[0];
    return n->keys[0];
}

static void delete_r(BTreeNode *n, int key);

/* Ensure child[i] has at least T keys before descending */
static void fix_child(BTreeNode *n, int i) {
    if (i > 0 && n->children[i-1]->n >= BTREE_T) {
        /* borrow from left sibling */
        BTreeNode *c = n->children[i], *sib = n->children[i-1];
        for (int j=c->n-1;j>=0;j--) c->keys[j+1]=c->keys[j];
        if (!c->leaf) for (int j=c->n;j>=0;j--) c->children[j+1]=c->children[j];
        c->keys[0]=n->keys[i-1]; n->keys[i-1]=sib->keys[sib->n-1];
        if (!sib->leaf) c->children[0]=sib->children[sib->n];
        c->n++; sib->n--;
    } else if (i < n->n && n->children[i+1]->n >= BTREE_T) {
        /* borrow from right sibling */
        BTreeNode *c = n->children[i], *sib = n->children[i+1];
        c->keys[c->n]=n->keys[i]; n->keys[i]=sib->keys[0];
        if (!c->leaf) c->children[c->n+1]=sib->children[0];
        for (int j=0;j<sib->n-1;j++) sib->keys[j]=sib->keys[j+1];
        if (!sib->leaf) for (int j=0;j<sib->n;j++) sib->children[j]=sib->children[j+1];
        c->n++; sib->n--;
    } else {
        /* merge */
        int mi = (i < n->n) ? i : i-1;
        BTreeNode *left = n->children[mi], *right = n->children[mi+1];
        left->keys[left->n] = n->keys[mi];
        for (int j=0;j<right->n;j++) left->keys[left->n+1+j]=right->keys[j];
        if (!left->leaf) for (int j=0;j<=right->n;j++) left->children[left->n+1+j]=right->children[j];
        left->n += right->n + 1;
        for (int j=mi;j<n->n-1;j++) { n->keys[j]=n->keys[j+1]; n->children[j+1]=n->children[j+2]; }
        n->n--;
        free(right);
    }
}

static void delete_r(BTreeNode *n, int key) {
    int i=0;
    while (i < n->n && key > n->keys[i]) i++;
    if (i < n->n && n->keys[i] == key) {
        if (n->leaf) {
            for (int j=i;j<n->n-1;j++) n->keys[j]=n->keys[j+1];
            n->n--;
        } else if (n->children[i]->n >= BTREE_T) {
            /* replace with predecessor */
            BTreeNode *pred=n->children[i];
            while (!pred->leaf) pred=pred->children[pred->n];
            n->keys[i]=pred->keys[pred->n-1];
            delete_r(n->children[i], n->keys[i]);
        } else if (n->children[i+1]->n >= BTREE_T) {
            n->keys[i] = btnode_min(n->children[i+1]);
            delete_r(n->children[i+1], n->keys[i]);
        } else {
            fix_child(n, i); delete_r(n->children[i], key);
        }
    } else if (!n->leaf) {
        if (n->children[i]->n < BTREE_T) fix_child(n, i);
        int di = (i > n->n) ? i-1 : i;
        delete_r(n->children[di], key);
    }
}

void bt_delete(BTree *t, int key) {
    if (!bt_search(t, key)) return;
    delete_r(t->root, key);
    if (t->root->n == 0 && !t->root->leaf) {
        BTreeNode *old = t->root;
        t->root = old->children[0];
        free(old);
    }
    t->size--;
}

static void print_r(BTreeNode *n, int depth) {
    for (int d=0;d<depth;d++) printf("  ");
    printf("[");
    for (int i=0;i<n->n;i++) printf("%d%s", n->keys[i], i<n->n-1?",":"");
    printf("]\n");
    if (!n->leaf) for (int i=0;i<=n->n;i++) print_r(n->children[i], depth+1);
}

void bt_print(const BTree *t) {
    printf("--- B-tree (T=%d, size=%zu) ---\n", BTREE_T, t->size);
    print_r(t->root, 0);
}

static void range_r(BTreeNode *n, int lo, int hi, void (*visit)(int)) {
    for (int i=0; i<=n->n; i++) {
        if (!n->leaf && (i==0 || n->keys[i-1] < hi))
            range_r(n->children[i], lo, hi, visit);
        if (i < n->n && n->keys[i] >= lo && n->keys[i] <= hi)
            visit(n->keys[i]);
    }
}

bool bt_range_scan(BTree *t, int lo, int hi, void (*visit)(int key)) {
    range_r(t->root, lo, hi, visit);
    return true;
}
