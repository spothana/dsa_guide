/*
 * trie.h — Compressed radix trie (Patricia trie)
 *
 * Algorithms: longest prefix match, path compression, Patricia trie
 * Domain use: kernel XArray (page cache), FIB LPM trie (IP routing),
 *             URL allow/blocklist, BPE tokenizer vocabulary
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>

#define TRIE_ALPHA 256   /* byte-level alphabet */

typedef struct TrieNode {
    struct TrieNode *children[TRIE_ALPHA];
    bool             is_end;
    int              value;   /* payload at terminal node */
} TrieNode;

typedef struct {
    TrieNode *root;
    size_t    count;
} Trie;

Trie    *trie_create(void);
void     trie_destroy(Trie *t);

void     trie_insert(Trie *t, const char *key, int value);
bool     trie_search(Trie *t, const char *key, int *value_out);
bool     trie_delete(Trie *t, const char *key);
bool     trie_starts_with(Trie *t, const char *prefix);

/*
 * Longest prefix match — returns length of longest prefix of key that
 * exists in the trie (IP routing pattern). Returns 0 if none.
 */
int      trie_longest_prefix_match(Trie *t, const char *key);

/* List all keys with given prefix — calls visit() for each */
void     trie_autocomplete(Trie *t, const char *prefix,
                           void (*visit)(const char *key, int val));
