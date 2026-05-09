/*
 * linked_list.c — Singly linked list: O(1) insert, reversal, merge sort,
 *                 cycle detection, middle-finding
 */
#include "linear/linked_list.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

static LLNode *node_new(int val) {
    LLNode *n = malloc(sizeof *n);
    n->val = val; n->next = NULL;
    return n;
}

LinkedList *ll_create(void) {
    LinkedList *l = malloc(sizeof *l);
    l->head = NULL; l->size = 0;
    return l;
}

void ll_destroy(LinkedList *l) {
    LLNode *cur = l->head;
    while (cur) { LLNode *nx = cur->next; free(cur); cur = nx; }
    free(l);
}

/* O(1) — always prepend */
void ll_push_front(LinkedList *l, int val) {
    LLNode *n = node_new(val);
    n->next = l->head;
    l->head = n;
    l->size++;
}

/* O(n) — walk to tail */
void ll_push_back(LinkedList *l, int val) {
    LLNode *n = node_new(val);
    if (!l->head) { l->head = n; }
    else {
        LLNode *cur = l->head;
        while (cur->next) cur = cur->next;
        cur->next = n;
    }
    l->size++;
}

int ll_pop_front(LinkedList *l) {
    assert(l->head);
    LLNode *old = l->head;
    int val = old->val;
    l->head = old->next;
    free(old);
    l->size--;
    return val;
}

bool ll_contains(LinkedList *l, int val) {
    for (LLNode *c = l->head; c; c = c->next)
        if (c->val == val) return true;
    return false;
}

void ll_delete(LinkedList *l, int val) {
    LLNode **cur = &l->head;
    while (*cur) {
        if ((*cur)->val == val) {
            LLNode *del = *cur;
            *cur = del->next;
            free(del);
            l->size--;
            return;
        }
        cur = &(*cur)->next;
    }
}

void ll_print(const LinkedList *l) {
    for (LLNode *c = l->head; c; c = c->next)
        printf("%d -> ", c->val);
    printf("NULL\n");
}

/* ── Reversal — O(n) in-place three-pointer ─────────────────────────────── */
/*
 * Algorithm: iterate with prev/cur/next pointers, flip each next pointer
 * Domain: kernel list_del + re-add, protocol packet reorder
 */
void ll_reverse(LinkedList *l) {
    LLNode *prev = NULL, *cur = l->head;
    while (cur) {
        LLNode *next = cur->next;
        cur->next = prev;
        prev = cur;
        cur = next;
    }
    l->head = prev;
}

/* ── Merge two sorted lists — O(n+m) ────────────────────────────────────── */
LLNode *ll_merge_sorted(LLNode *a, LLNode *b) {
    LLNode dummy = {0}; LLNode *tail = &dummy;
    while (a && b) {
        if (a->val <= b->val) { tail->next = a; a = a->next; }
        else                  { tail->next = b; b = b->next; }
        tail = tail->next;
    }
    tail->next = a ? a : b;
    return dummy.next;
}

/* ── Merge sort — O(n log n) ────────────────────────────────────────────── */
/*
 * Algorithm: split at midpoint, recurse, merge
 * Why lists: no random access needed — split is O(n) but merge is O(1) link swap
 * Domain: sorting sk_buff chains, inode list compaction
 */
static LLNode *merge_sort_node(LLNode *head) {
    if (!head || !head->next) return head;

    /* find midpoint */
    LLNode *slow = head, *fast = head->next;
    while (fast && fast->next) { slow = slow->next; fast = fast->next->next; }

    LLNode *second = slow->next;
    slow->next = NULL;  /* split */

    return ll_merge_sorted(merge_sort_node(head),
                           merge_sort_node(second));
}

LinkedList *ll_merge_sort(LinkedList *l) {
    l->head = merge_sort_node(l->head);
    return l;
}

/* ── Cycle detection — Floyd's tortoise & hare ───────────────────────────── */
/*
 * Algorithm: two pointers — slow moves 1 step, fast moves 2 steps.
 * If they meet, a cycle exists. O(n) time, O(1) space.
 * Domain: detecting corrupted kernel linked lists, OS resource leak checks
 */
bool ll_has_cycle(const LinkedList *l) {
    LLNode *slow = l->head, *fast = l->head;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) return true;
    }
    return false;
}

/* ── Find middle — slow/fast pointer ────────────────────────────────────── */
LLNode *ll_find_middle(const LinkedList *l) {
    LLNode *slow = l->head, *fast = l->head;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
    }
    return slow;
}
