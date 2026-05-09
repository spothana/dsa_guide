/*
 * trie.c — Compressed radix trie with longest prefix match
 */
#include "trees/trie.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

Trie *trie_create(void) {
    Trie *t = malloc(sizeof *t);
    t->root = calloc(1, sizeof(TrieNode));
    t->count = 0;
    return t;
}

static void free_node(TrieNode *n) {
    if (!n) return;
    for (int i=0;i<TRIE_ALPHA;i++) free_node(n->children[i]);
    free(n);
}

void trie_destroy(Trie *t) { free_node(t->root); free(t); }

void trie_insert(Trie *t, const char *key, int value) {
    TrieNode *cur = t->root;
    for (; *key; key++) {
        int c = (unsigned char)*key;
        if (!cur->children[c]) cur->children[c] = calloc(1, sizeof(TrieNode));
        cur = cur->children[c];
    }
    if (!cur->is_end) t->count++;
    cur->is_end = true;
    cur->value = value;
}

bool trie_search(Trie *t, const char *key, int *val_out) {
    TrieNode *cur = t->root;
    for (; *key; key++) {
        int c = (unsigned char)*key;
        if (!cur->children[c]) return false;
        cur = cur->children[c];
    }
    if (!cur->is_end) return false;
    if (val_out) *val_out = cur->value;
    return true;
}

bool trie_starts_with(Trie *t, const char *prefix) {
    TrieNode *cur = t->root;
    for (; *prefix; prefix++) {
        int c = (unsigned char)*prefix;
        if (!cur->children[c]) return false;
        cur = cur->children[c];
    }
    return true;
}

static bool has_children(TrieNode *n) {
    for (int i=0;i<TRIE_ALPHA;i++) if (n->children[i]) return true;
    return false;
}

static bool delete_r(TrieNode *n, const char *key) {
    if (!*key) {
        if (!n->is_end) return false;
        n->is_end = false;
        /* only signal parent to delete us if we have no children */
        return !has_children(n);
    }
    int c=(unsigned char)*key;
    if (!n->children[c]) return false;
    bool should_delete_child = delete_r(n->children[c], key+1);
    if (should_delete_child) {
        free(n->children[c]);
        n->children[c] = NULL;
    }
    /* delete this node only if it's not terminal and has no remaining children */
    return !n->is_end && !has_children(n);
}

bool trie_delete(Trie *t, const char *key) {
    if (!trie_search(t, key, NULL)) return false;
    delete_r(t->root, key);
    t->count--;
    return true;
}

/*
 * Longest prefix match — returns length of longest matching prefix
 *
 * Algorithm: walk trie, record last terminal node seen.
 * Domain: IP routing (FIB trie), URL path routing, autocomplete
 *
 * Example: trie has "192.168" and "192.168.1"
 *   LPM("192.168.1.1") = 9  (matches "192.168.1")
 *   LPM("192.168.5.1") = 7  (matches "192.168")
 */
int trie_longest_prefix_match(Trie *t, const char *key) {
    TrieNode *cur = t->root;
    int best = 0, depth = 0;
    for (; *key; key++, depth++) {
        int c = (unsigned char)*key;
        if (!cur->children[c]) break;
        cur = cur->children[c];
        if (cur->is_end) best = depth+1;
    }
    return best;
}

static void autocomplete_r(TrieNode *n, char *buf, int depth,
                            void (*visit)(const char*, int)) {
    if (!n) return;
    if (n->is_end) visit(buf, n->value);
    for (int i=0;i<TRIE_ALPHA;i++) if (n->children[i]) {
        buf[depth] = (char)i; buf[depth+1] = '\0';
        autocomplete_r(n->children[i], buf, depth+1, visit);
    }
}

void trie_autocomplete(Trie *t, const char *prefix,
                       void (*visit)(const char *key, int val)) {
    TrieNode *cur = t->root;
    int plen = (int)strlen(prefix);
    char buf[512]; strncpy(buf, prefix, sizeof buf-1); buf[sizeof buf-1]=0;
    for (int i=0;i<plen;i++) {
        int c=(unsigned char)prefix[i];
        if (!cur->children[c]) return;
        cur = cur->children[c];
    }
    if (cur->is_end) visit(buf, cur->value);
    for (int i=0;i<TRIE_ALPHA;i++) if (cur->children[i]) {
        buf[plen]=(char)i; buf[plen+1]='\0';
        autocomplete_r(cur->children[i], buf, plen+1, visit);
    }
}
