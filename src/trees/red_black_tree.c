/*
 * red_black_tree.c — Left-leaning red-black BST
 *
 * Properties maintained after every insert/delete:
 *   1. Every node is RED or BLACK
 *   2. Root is BLACK
 *   3. Every NIL leaf is BLACK
 *   4. RED node's children are BLACK (no two consecutive reds)
 *   5. All paths from any node to NIL leaves have same BLACK-height
 *
 * These guarantee height ≤ 2·log₂(n+1) → O(log n) all operations
 *
 * CFS pattern: leftmost pointer caches minimum for O(1) pick-next-task
 */
#include "trees/red_black_tree.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define IS_RED(t,n)   ((n) != (t)->nil && (n)->color == RB_RED)
#define IS_BLACK(t,n) ((n) == (t)->nil || (n)->color == RB_BLACK)

RBTree *rbt_create(void) {
    RBTree *t = malloc(sizeof *t);
    t->nil = malloc(sizeof(RBNode));
    t->nil->color = RB_BLACK;
    t->nil->left = t->nil->right = t->nil->parent = t->nil;
    t->nil->key = t->nil->val = 0;
    t->root = t->nil;
    t->leftmost = t->nil;
    t->size = 0;
    return t;
}

static void free_nodes(RBTree *t, RBNode *n) {
    if (n == t->nil) return;
    free_nodes(t, n->left);
    free_nodes(t, n->right);
    free(n);
}

void rbt_destroy(RBTree *t) {
    free_nodes(t, t->root);
    free(t->nil);
    free(t);
}

/* ── Rotations ───────────────────────────────────────────────────────────── */
/*
 * Left rotation: pivot x up, x's right child y becomes parent
 *        x                 y
 *       / \               / \
 *      A   y    →        x   C
 *         / \           / \
 *        B   C         A   B
 */
static void rotate_left(RBTree *t, RBNode *x) {
    RBNode *y = x->right;
    x->right = y->left;
    if (y->left != t->nil) y->left->parent = x;
    y->parent = x->parent;
    if (x->parent == t->nil)       t->root = y;
    else if (x == x->parent->left) x->parent->left = y;
    else                           x->parent->right = y;
    y->left = x;
    x->parent = y;
}

static void rotate_right(RBTree *t, RBNode *x) {
    RBNode *y = x->left;
    x->left = y->right;
    if (y->right != t->nil) y->right->parent = x;
    y->parent = x->parent;
    if (x->parent == t->nil)        t->root = y;
    else if (x == x->parent->right) x->parent->right = y;
    else                            x->parent->left = y;
    y->right = x;
    x->parent = y;
}

/* ── Insert fixup ────────────────────────────────────────────────────────── */
/*
 * After BST insert (new node is RED), fix red-red violations.
 * Three cases depending on uncle's colour:
 *   Case 1: Uncle RED   → recolour parent + uncle BLACK, grandparent RED
 *   Case 2: Uncle BLACK, node is right child → rotate left on parent
 *   Case 3: Uncle BLACK, node is left child  → rotate right on grandparent
 */
static void insert_fixup(RBTree *t, RBNode *z) {
    while (IS_RED(t, z->parent)) {
        if (z->parent == z->parent->parent->left) {
            RBNode *y = z->parent->parent->right;  /* uncle */
            if (IS_RED(t, y)) {                    /* case 1 */
                z->parent->color = RB_BLACK;
                y->color = RB_BLACK;
                z->parent->parent->color = RB_RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {       /* case 2 */
                    z = z->parent;
                    rotate_left(t, z);
                }
                z->parent->color = RB_BLACK;       /* case 3 */
                z->parent->parent->color = RB_RED;
                rotate_right(t, z->parent->parent);
            }
        } else {  /* mirror */
            RBNode *y = z->parent->parent->left;
            if (IS_RED(t, y)) {
                z->parent->color = RB_BLACK;
                y->color = RB_BLACK;
                z->parent->parent->color = RB_RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    rotate_right(t, z);
                }
                z->parent->color = RB_BLACK;
                z->parent->parent->color = RB_RED;
                rotate_left(t, z->parent->parent);
            }
        }
    }
    t->root->color = RB_BLACK;
}

void rbt_insert(RBTree *t, int key, int val) {
    RBNode *z = malloc(sizeof *z);
    z->key = key; z->val = val; z->color = RB_RED;
    z->left = z->right = z->parent = t->nil;

    RBNode *y = t->nil, *x = t->root;
    while (x != t->nil) {
        y = x;
        if (key < x->key)  x = x->left;
        else if (key > x->key) x = x->right;
        else { x->val = val; free(z); return; }  /* update existing */
    }
    z->parent = y;
    if (y == t->nil)       t->root = z;
    else if (key < y->key) y->left = z;
    else                   y->right = z;

    t->size++;
    insert_fixup(t, z);

    /* update leftmost cache (CFS pattern) */
    if (t->leftmost == t->nil || key < t->leftmost->key)
        t->leftmost = z;
}

RBNode *rbt_search(RBTree *t, int key) {
    RBNode *x = t->root;
    while (x != t->nil) {
        if      (key < x->key) x = x->left;
        else if (key > x->key) x = x->right;
        else return x;
    }
    return NULL;
}

/* O(1) — leftmost cache mirrors CFS's cached leftmost rb entry */
RBNode *rbt_minimum(RBTree *t) {
    return t->leftmost == t->nil ? NULL : t->leftmost;
}

RBNode *rbt_maximum(RBTree *t) {
    RBNode *x = t->root;
    if (x == t->nil) return NULL;
    while (x->right != t->nil) x = x->right;
    return x;
}

RBNode *rbt_successor(RBTree *t, RBNode *node) {
    if (node->right != t->nil) {
        RBNode *x = node->right;
        while (x->left != t->nil) x = x->left;
        return x;
    }
    RBNode *y = node->parent;
    while (y != t->nil && node == y->right) { node = y; y = y->parent; }
    return y == t->nil ? NULL : y;
}

static void transplant(RBTree *t, RBNode *u, RBNode *v) {
    if (u->parent == t->nil)        t->root = v;
    else if (u == u->parent->left)  u->parent->left = v;
    else                            u->parent->right = v;
    v->parent = u->parent;
}

static void delete_fixup(RBTree *t, RBNode *x);

void rbt_delete(RBTree *t, int key) {
    RBNode *z = rbt_search(t, key);
    if (!z) return;

    RBNode *y = z, *x;
    RBColor y_orig = y->color;

    if (z->left == t->nil) {
        x = z->right; transplant(t, z, z->right);
    } else if (z->right == t->nil) {
        x = z->left; transplant(t, z, z->left);
    } else {
        y = z->right;
        while (y->left != t->nil) y = y->left;
        y_orig = y->color;
        x = y->right;
        if (y->parent == z) x->parent = y;
        else {
            transplant(t, y, y->right);
            y->right = z->right;
            y->right->parent = y;
        }
        transplant(t, z, y);
        y->left = z->left;
        y->left->parent = y;
        y->color = z->color;
    }
    free(z); t->size--;

    if (y_orig == RB_BLACK) delete_fixup(t, x);

    /* rebuild leftmost if needed */
    if (t->root == t->nil) t->leftmost = t->nil;
    else {
        RBNode *m = t->root;
        while (m->left != t->nil) m = m->left;
        t->leftmost = m;
    }
}

static void delete_fixup(RBTree *t, RBNode *x) {
    while (x != t->root && IS_BLACK(t, x)) {
        if (x == x->parent->left) {
            RBNode *w = x->parent->right;
            if (IS_RED(t, w)) {
                w->color = RB_BLACK; x->parent->color = RB_RED;
                rotate_left(t, x->parent); w = x->parent->right;
            }
            if (IS_BLACK(t, w->left) && IS_BLACK(t, w->right)) {
                w->color = RB_RED; x = x->parent;
            } else {
                if (IS_BLACK(t, w->right)) {
                    w->left->color = RB_BLACK; w->color = RB_RED;
                    rotate_right(t, w); w = x->parent->right;
                }
                w->color = x->parent->color;
                x->parent->color = RB_BLACK; w->right->color = RB_BLACK;
                rotate_left(t, x->parent); x = t->root;
            }
        } else {  /* mirror */
            RBNode *w = x->parent->left;
            if (IS_RED(t, w)) {
                w->color = RB_BLACK; x->parent->color = RB_RED;
                rotate_right(t, x->parent); w = x->parent->left;
            }
            if (IS_BLACK(t, w->right) && IS_BLACK(t, w->left)) {
                w->color = RB_RED; x = x->parent;
            } else {
                if (IS_BLACK(t, w->left)) {
                    w->right->color = RB_BLACK; w->color = RB_RED;
                    rotate_left(t, w); w = x->parent->left;
                }
                w->color = x->parent->color;
                x->parent->color = RB_BLACK; w->left->color = RB_BLACK;
                rotate_right(t, x->parent); x = t->root;
            }
        }
    }
    x->color = RB_BLACK;
}

static void inorder_r(RBTree *t, RBNode *n, void (*visit)(RBNode *)) {
    if (n == t->nil) return;
    inorder_r(t, n->left, visit);
    visit(n);
    inorder_r(t, n->right, visit);
}

void rbt_inorder(const RBTree *t, void (*visit)(RBNode *)) {
    inorder_r((RBTree*)t, t->root, visit);
}

static int check_black_height(RBTree *t, RBNode *n) {
    if (n == t->nil) return 1;
    int lh = check_black_height(t, n->left);
    int rh = check_black_height(t, n->right);
    if (lh < 0 || rh < 0 || lh != rh) return -1;
    if (IS_RED(t, n) && (IS_RED(t, n->left) || IS_RED(t, n->right))) return -1;
    return lh + (n->color == RB_BLACK ? 1 : 0);
}

bool rbt_is_valid(const RBTree *t) {
    if (t->root == t->nil) return true;
    if (IS_RED(t, t->root)) return false;
    return check_black_height((RBTree*)t, t->root) > 0;
}

static void print_r(RBTree *t, RBNode *n, int depth) {
    if (n == t->nil) return;
    print_r(t, n->right, depth+1);
    for (int i=0;i<depth;i++) printf("    ");
    printf("%d(%s)\n", n->key, n->color==RB_RED?"R":"B");
    print_r(t, n->left, depth+1);
}

void rbt_print(const RBTree *t) {
    printf("--- Red-Black Tree ---\n");
    print_r((RBTree*)t, t->root, 0);
    printf("size=%zu valid=%s\n", t->size, rbt_is_valid(t)?"yes":"NO");
}
