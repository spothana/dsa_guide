/*
 * cuckoo_hash.c — Cuckoo hash: O(1) worst-case lookup
 */
#include "hash/cuckoo_hash.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static unsigned int h1(int key, size_t cap) {
    unsigned k=(unsigned)key; k^=k>>16; k*=0x45d9f3b; return k%cap;
}
static unsigned int h2(int key, size_t cap) {
    unsigned k=(unsigned)key; k^=k>>13; k*=0x9e3779b9; k^=k>>11; return k%cap;
}

CuckooHash *cuckoo_create(size_t cap) {
    CuckooHash *ch = malloc(sizeof *ch);
    ch->keys1 = malloc(cap*sizeof(int)); ch->vals1 = malloc(cap*sizeof(int));
    ch->keys2 = malloc(cap*sizeof(int)); ch->vals2 = malloc(cap*sizeof(int));
    ch->capacity = cap; ch->size = 0;
    for (size_t i=0;i<cap;i++) { ch->keys1[i]=CUCKOO_EMPTY; ch->keys2[i]=CUCKOO_EMPTY; }
    return ch;
}

void cuckoo_destroy(CuckooHash *ch) {
    free(ch->keys1);free(ch->vals1);free(ch->keys2);free(ch->vals2);free(ch);
}

bool cuckoo_get(CuckooHash *ch, int key, int *val_out) {
    unsigned s1=h1(key,ch->capacity), s2=h2(key,ch->capacity);
    if (ch->keys1[s1]==key) { *val_out=ch->vals1[s1]; return true; }
    if (ch->keys2[s2]==key) { *val_out=ch->vals2[s2]; return true; }
    return false;
}

bool cuckoo_insert(CuckooHash *ch, int key, int val) {
    int cur_key=key, cur_val=val;
    for (int kick=0; kick<CUCKOO_MAX_KICKS; kick++) {
        unsigned s1=h1(cur_key,ch->capacity);
        if (ch->keys1[s1]==CUCKOO_EMPTY || ch->keys1[s1]==cur_key) {
            ch->keys1[s1]=cur_key; ch->vals1[s1]=cur_val; ch->size++; return true;
        }
        /* displace occupant to table 2 */
        int displaced_k=ch->keys1[s1], displaced_v=ch->vals1[s1];
        ch->keys1[s1]=cur_key; ch->vals1[s1]=cur_val;
        cur_key=displaced_k; cur_val=displaced_v;

        unsigned s2=h2(cur_key,ch->capacity);
        if (ch->keys2[s2]==CUCKOO_EMPTY || ch->keys2[s2]==cur_key) {
            ch->keys2[s2]=cur_key; ch->vals2[s2]=cur_val; ch->size++; return true;
        }
        displaced_k=ch->keys2[s2]; displaced_v=ch->vals2[s2];
        ch->keys2[s2]=cur_key; ch->vals2[s2]=cur_val;
        cur_key=displaced_k; cur_val=displaced_v;
    }
    return false; /* rehash needed */
}

bool cuckoo_delete(CuckooHash *ch, int key) {
    unsigned s1=h1(key,ch->capacity), s2=h2(key,ch->capacity);
    if (ch->keys1[s1]==key) { ch->keys1[s1]=CUCKOO_EMPTY; ch->size--; return true; }
    if (ch->keys2[s2]==key) { ch->keys2[s2]=CUCKOO_EMPTY; ch->size--; return true; }
    return false;
}

void cuckoo_print(const CuckooHash *ch) {
    printf("CuckooHash (size=%zu cap=%zu):\n", ch->size, ch->capacity);
    for (size_t i=0;i<ch->capacity;i++) {
        if (ch->keys1[i]!=CUCKOO_EMPTY) printf("  T1[%zu]: %d→%d\n",i,ch->keys1[i],ch->vals1[i]);
        if (ch->keys2[i]!=CUCKOO_EMPTY) printf("  T2[%zu]: %d→%d\n",i,ch->keys2[i],ch->vals2[i]);
    }
}
