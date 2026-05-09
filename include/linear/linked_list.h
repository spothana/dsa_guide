/*
 * linked_list.h — Singly linked list
 *
 * Algorithms: O(1) head insert, list reversal, merge sort
 * Domain use: task_struct lists, sk_buff queues, inode chains
 */
#pragma once
#include <stddef.h>
#include <stdbool.h>

typedef struct LLNode {
    int            val;
    struct LLNode *next;
} LLNode;

typedef struct {
    LLNode *head;
    size_t  size;
} LinkedList;

/* lifecycle */
LinkedList *ll_create(void);
void        ll_destroy(LinkedList *l);

/* operations — all O(1) unless noted */
void    ll_push_front(LinkedList *l, int val);   /* O(1) */
void    ll_push_back(LinkedList *l, int val);    /* O(n) */
int     ll_pop_front(LinkedList *l);
bool    ll_contains(LinkedList *l, int val);     /* O(n) */
void    ll_delete(LinkedList *l, int val);       /* O(n) */
void    ll_print(const LinkedList *l);

/* algorithms */
void        ll_reverse(LinkedList *l);            /* O(n) in-place */
LinkedList *ll_merge_sort(LinkedList *l);         /* O(n log n) */
LLNode     *ll_merge_sorted(LLNode *a, LLNode *b);
bool        ll_has_cycle(const LinkedList *l);    /* Floyd's tortoise & hare */
LLNode     *ll_find_middle(const LinkedList *l);  /* slow/fast pointer */
