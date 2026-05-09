/*
 * circular_list.c — Circular doubly-linked list with LRU eviction
 *
 * Mirrors Linux kernel list_head design: a sentinel node whose
 * next/prev always point into the list. O(1) insert, delete, splice.
 */
#include "linear/circular_list.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

static CLNode *clnode_new(int val) {
    CLNode *n = malloc(sizeof *n);
    n->val = val; n->next = n->prev = NULL;
    return n;
}

CircularList *clist_create(void) {
    CircularList *l = malloc(sizeof *l);
    l->sentinel = clnode_new(0);           /* dummy sentinel */
    l->sentinel->next = l->sentinel;
    l->sentinel->prev = l->sentinel;
    l->size = 0;
    return l;
}

void clist_destroy(CircularList *l) {
    CLNode *cur = l->sentinel->next;
    while (cur != l->sentinel) {
        CLNode *nx = cur->next;
        free(cur); cur = nx;
    }
    free(l->sentinel);
    free(l);
}

/* Insert node between prev and next — O(1) core primitive */
static void _insert_between(CLNode *node, CLNode *prev, CLNode *next) {
    next->prev = node;
    node->next = next;
    node->prev = prev;
    prev->next = node;
}

/* Unlink node from list — O(1) */
static void _unlink(CLNode *node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

void clist_push_front(CircularList *l, int val) {
    CLNode *n = clnode_new(val);
    _insert_between(n, l->sentinel, l->sentinel->next);
    l->size++;
}

void clist_push_back(CircularList *l, int val) {
    CLNode *n = clnode_new(val);
    _insert_between(n, l->sentinel->prev, l->sentinel);
    l->size++;
}

int clist_pop_front(CircularList *l) {
    assert(l->size > 0);
    CLNode *n = l->sentinel->next;
    int val = n->val;
    _unlink(n); free(n); l->size--;
    return val;
}

int clist_pop_back(CircularList *l) {
    assert(l->size > 0);
    CLNode *n = l->sentinel->prev;
    int val = n->val;
    _unlink(n); free(n); l->size--;
    return val;
}

void clist_delete(CircularList *l, CLNode *node) {
    _unlink(node); free(node); l->size--;
}

/*
 * Move node to front — O(1)
 *
 * This is the heart of LRU: accessing an element promotes it to MRU.
 * The sentinel design means the operation is always just:
 *   unlink + re-insert at front — no edge cases for head/tail.
 *
 * Domain: Linux page cache LRU, dentry cache MRU, buffer pool promotion
 */
void clist_move_to_front(CircularList *l, CLNode *node) {
    _unlink(node);
    _insert_between(node, l->sentinel, l->sentinel->next);
}

/*
 * LRU cache access — O(n) scan for existing, O(1) promote/evict
 *
 * Real LRU implementations add a hash table for O(1) lookup.
 * Here we show the list mechanics clearly.
 *
 * Domain: dentry cache eviction, OS page replacement, DB buffer pool
 */
int clist_lru_access(CircularList *l, int val, size_t max_size) {
    /* search for existing */
    for (CLNode *c = l->sentinel->next; c != l->sentinel; c = c->next) {
        if (c->val == val) {
            clist_move_to_front(l, c);  /* cache hit — promote */
            return -1;
        }
    }
    /* cache miss — insert at front */
    clist_push_front(l, val);
    int evicted = -1;
    if (l->size > max_size) {
        evicted = clist_pop_back(l);  /* evict LRU (back of list) */
    }
    return evicted;
}

void clist_print(const CircularList *l) {
    printf("[");
    for (CLNode *c = l->sentinel->next; c != l->sentinel; c = c->next)
        printf("%d%s", c->val, c->next != l->sentinel ? " <-> " : "");
    printf("]\n");
}
