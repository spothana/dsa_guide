/*
 * hash_table.h — Chaining hash table with RCU-style resize
 *
 * Algorithms: O(1) avg lookup, open addressing (linear probe), chaining
 * Domain use: PID table, dentry cache, inode cache, connection tracking,
 *             hash join, password hash store, embedding lookup
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef struct HTEntry {
    int            key;
    int            val;
    struct HTEntry *next;   /* chaining */
} HTEntry;

typedef struct {
    HTEntry **buckets;
    size_t    capacity;
    size_t    size;
    double    load_factor_max;  /* resize threshold */
} HashTable;

HashTable *ht_create(size_t initial_capacity);
void       ht_destroy(HashTable *ht);

bool       ht_insert(HashTable *ht, int key, int val);
bool       ht_get(HashTable *ht, int key, int *val_out);
bool       ht_delete(HashTable *ht, int key);
void       ht_print(const HashTable *ht);

/* Open-addressing (linear probe) variant */
typedef struct {
    int   *keys;
    int   *vals;
    bool  *used;
    bool  *deleted;
    size_t capacity;
    size_t size;
} OAHashTable;

OAHashTable *oaht_create(size_t capacity);
void         oaht_destroy(OAHashTable *ht);
bool         oaht_insert(OAHashTable *ht, int key, int val);
bool         oaht_get(OAHashTable *ht, int key, int *val_out);
bool         oaht_delete(OAHashTable *ht, int key);
