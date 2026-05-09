/*
 * circular_list.h — Circular doubly-linked list (Linux list_head style)
 *
 * Algorithms: O(1) splice, LRU clock-hand, wait-queue rotation
 * Domain use: list_head (most-used Linux kernel struct), LRU page list,
 *             socket wait queues, buffer pool LRU
 */
#pragma once
#include <stddef.h>
#include <stdbool.h>

typedef struct CLNode {
    int            val;
    struct CLNode *next;
    struct CLNode *prev;
} CLNode;

typedef struct {
    CLNode *sentinel;   /* dummy head; sentinel->next = first real node */
    size_t  size;
} CircularList;

CircularList *clist_create(void);
void          clist_destroy(CircularList *l);

void    clist_push_front(CircularList *l, int val);
void    clist_push_back(CircularList *l, int val);
int     clist_pop_front(CircularList *l);
int     clist_pop_back(CircularList *l);
void    clist_delete(CircularList *l, CLNode *node);

/* O(1) splice: moves node to front (MRU) — core of LRU eviction */
void    clist_move_to_front(CircularList *l, CLNode *node);

/* LRU cache simulation: returns evicted value (-1 if nothing evicted) */
int     clist_lru_access(CircularList *l, int val, size_t max_size);

void    clist_print(const CircularList *l);
