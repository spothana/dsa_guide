/*
 * hash_table.c — Chaining hash table with dynamic resizing + open-addressing
 */
#include "hash/hash_table.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* FNV-1a inspired integer hash — good avalanche, low collision */
static size_t hash_int(int key, size_t cap) {
    unsigned int k = (unsigned int)key;
    k ^= k >> 16; k *= 0x45d9f3b; k ^= k >> 16;
    return k % cap;
}

HashTable *ht_create(size_t cap) {
    HashTable *ht = malloc(sizeof *ht);
    ht->buckets = calloc(cap, sizeof(HTEntry*));
    ht->capacity = cap; ht->size = 0;
    ht->load_factor_max = 0.75;
    return ht;
}

void ht_destroy(HashTable *ht) {
    for (size_t i=0;i<ht->capacity;i++) {
        HTEntry *e = ht->buckets[i];
        while (e) { HTEntry *nx=e->next; free(e); e=nx; }
    }
    free(ht->buckets); free(ht);
}

static void ht_resize(HashTable *ht) {
    size_t new_cap = ht->capacity * 2;
    HTEntry **new_buckets = calloc(new_cap, sizeof(HTEntry*));
    for (size_t i=0;i<ht->capacity;i++) {
        HTEntry *e = ht->buckets[i];
        while (e) {
            HTEntry *nx = e->next;
            size_t nb = hash_int(e->key, new_cap);
            e->next = new_buckets[nb];
            new_buckets[nb] = e;
            e = nx;
        }
    }
    free(ht->buckets);
    ht->buckets = new_buckets;
    ht->capacity = new_cap;
}

bool ht_insert(HashTable *ht, int key, int val) {
    if ((double)ht->size / ht->capacity >= ht->load_factor_max)
        ht_resize(ht);
    size_t idx = hash_int(key, ht->capacity);
    for (HTEntry *e=ht->buckets[idx]; e; e=e->next)
        if (e->key==key) { e->val=val; return true; }
    HTEntry *ne = malloc(sizeof *ne);
    ne->key=key; ne->val=val;
    ne->next = ht->buckets[idx];
    ht->buckets[idx] = ne;
    ht->size++;
    return true;
}

bool ht_get(HashTable *ht, int key, int *val_out) {
    size_t idx = hash_int(key, ht->capacity);
    for (HTEntry *e=ht->buckets[idx]; e; e=e->next)
        if (e->key==key) { *val_out=e->val; return true; }
    return false;
}

bool ht_delete(HashTable *ht, int key) {
    size_t idx = hash_int(key, ht->capacity);
    HTEntry **cur = &ht->buckets[idx];
    while (*cur) {
        if ((*cur)->key==key) {
            HTEntry *del = *cur; *cur = del->next; free(del); ht->size--; return true;
        }
        cur = &(*cur)->next;
    }
    return false;
}

void ht_print(const HashTable *ht) {
    printf("HashTable (size=%zu cap=%zu):\n", ht->size, ht->capacity);
    for (size_t i=0;i<ht->capacity;i++) {
        if (!ht->buckets[i]) continue;
        printf("  [%zu]:", i);
        for (HTEntry *e=ht->buckets[i];e;e=e->next) printf(" %d→%d",e->key,e->val);
        printf("\n");
    }
}

/* ── Open-addressing (linear probe) ─────────────────────────────────────── */

OAHashTable *oaht_create(size_t cap) {
    OAHashTable *ht = malloc(sizeof *ht);
    ht->keys    = malloc(cap*sizeof(int));
    ht->vals    = malloc(cap*sizeof(int));
    ht->used    = calloc(cap, sizeof(bool));
    ht->deleted = calloc(cap, sizeof(bool));
    ht->capacity = cap; ht->size = 0;
    return ht;
}

void oaht_destroy(OAHashTable *ht) {
    free(ht->keys); free(ht->vals); free(ht->used); free(ht->deleted); free(ht);
}

bool oaht_insert(OAHashTable *ht, int key, int val) {
    size_t idx = hash_int(key, ht->capacity);
    for (size_t i=0; i<ht->capacity; i++) {
        size_t slot = (idx+i) % ht->capacity;
        if (!ht->used[slot] || ht->deleted[slot]) {
            ht->keys[slot]=key; ht->vals[slot]=val;
            ht->used[slot]=true; ht->deleted[slot]=false;
            ht->size++; return true;
        }
        if (ht->used[slot] && !ht->deleted[slot] && ht->keys[slot]==key) {
            ht->vals[slot]=val; return true;
        }
    }
    return false;
}

bool oaht_get(OAHashTable *ht, int key, int *val_out) {
    size_t idx = hash_int(key, ht->capacity);
    for (size_t i=0; i<ht->capacity; i++) {
        size_t slot = (idx+i) % ht->capacity;
        if (!ht->used[slot]) return false;
        if (!ht->deleted[slot] && ht->keys[slot]==key) {
            *val_out=ht->vals[slot]; return true;
        }
    }
    return false;
}

bool oaht_delete(OAHashTable *ht, int key) {
    size_t idx = hash_int(key, ht->capacity);
    for (size_t i=0; i<ht->capacity; i++) {
        size_t slot = (idx+i) % ht->capacity;
        if (!ht->used[slot]) return false;
        if (!ht->deleted[slot] && ht->keys[slot]==key) {
            ht->deleted[slot]=true; ht->size--; return true;
        }
    }
    return false;
}
