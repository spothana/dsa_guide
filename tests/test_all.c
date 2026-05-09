/*
 * test_all.c — Unit tests for every data structure
 * Each test prints PASS or FAIL with the assertion that failed.
 */
#include "dsa.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int tests_run=0, tests_passed=0;

#define TEST(name, cond) do { \
    tests_run++; \
    if (cond) { tests_passed++; printf("  PASS  %s\n", name); } \
    else       { printf("  FAIL  %s  (line %d)\n", name, __LINE__); } \
} while(0)

/* ── Array ────────────────────────────────────────────────────────────────── */
static void test_array(void) {
    printf("\n[Array]\n");
    Array *a = array_create(4);
    for (int i=0;i<6;i++) array_push(a,i*2);          /* 0,2,4,6,8,10 */
    TEST("binary search found",   array_binary_search(a,6) == 3);
    TEST("binary search missing", array_binary_search(a,5) == -1);

    int pi, pj;
    TEST("two-pointer pair",      array_two_pointer_pair(a,10,&pi,&pj));

    int sw[]={2,1,5,3,6,4,8,7};
    Array *swa=array_create(8); for(int i=0;i<8;i++) array_push(swa,sw[i]);
    Array *wm=array_sliding_window_max(swa,3);
    int expected[]={5,5,6,6,8,8};
    bool ok=true;
    for(size_t i=0;i<wm->size;i++) if(wm->data[i]!=expected[i]) ok=false;
    TEST("sliding window max", ok);

    int ps[]={1,2,3,4,5};
    Array *pa=array_create(5); for(int i=0;i<5;i++) array_push(pa,ps[i]);
    Array *pf=array_prefix_sum_build(pa);
    TEST("prefix sum [1..3]", array_prefix_sum_query(pf,1,3)==9);
    TEST("prefix sum [0..4]", array_prefix_sum_query(pf,0,4)==15);

    array_destroy(a); array_destroy(swa); array_destroy(wm);
    array_destroy(pa); array_destroy(pf);
}

/* ── Linked list ──────────────────────────────────────────────────────────── */
static void test_linked_list(void) {
    printf("\n[Linked List]\n");
    LinkedList *l=ll_create();
    int v[]={5,3,8,1,9};
    for(int i=0;i<5;i++) ll_push_back(l,v[i]);
    TEST("contains 8",      ll_contains(l,8));
    TEST("not contains 99", !ll_contains(l,99));
    ll_merge_sort(l);
    TEST("sorted head=1",   l->head->val==1);
    TEST("sorted tail=9",   l->head->next->next->next->next->val==9);
    ll_reverse(l);
    TEST("reversed head=9", l->head->val==9);
    TEST("no cycle",        !ll_has_cycle(l));
    ll_destroy(l);
}

/* ── Stack ────────────────────────────────────────────────────────────────── */
static void test_stack(void) {
    printf("\n[Stack]\n");
    TEST("postfix 3 4 + 2 *", stack_eval_postfix("3 4 + 2 *")==14);
    TEST("postfix 5 1 2 + 4 * + 3 -", stack_eval_postfix("5 1 2 + 4 * + 3 -")==14);
    TEST("brackets {[()]}",   stack_balanced_brackets("{[()]}"));
    TEST("brackets {[(])} no",!stack_balanced_brackets("{[(])}"));
    TEST("brackets empty",    stack_balanced_brackets(""));
}

/* ── Queue / Deque ────────────────────────────────────────────────────────── */
static void test_queue(void) {
    printf("\n[Queue / Deque]\n");
    Queue *q=queue_create(8);
    queue_enqueue(q,1); queue_enqueue(q,2); queue_enqueue(q,3);
    TEST("dequeue FIFO", queue_dequeue(q)==1);
    TEST("peek",         queue_peek(q)==2);
    queue_destroy(q);

    int sw[]={2,1,5,3,6,4,8,7}; int len;
    int *wm=deque_sliding_window_max(sw,8,3,&len);
    int exp[]={5,5,6,6,8,8};
    bool ok=true; for(int i=0;i<len;i++) if(wm[i]!=exp[i]) ok=false;
    TEST("deque sliding window max", ok);
    free(wm);
}

/* ── Ring buffer ──────────────────────────────────────────────────────────── */
static void test_ring_buffer(void) {
    printf("\n[Ring Buffer]\n");
    RingBuffer *rb=rb_create(4);
    TEST("produce 3", rb_produce(rb,10)&&rb_produce(rb,20)&&rb_produce(rb,30));
    int v; rb_consume(rb,&v);
    TEST("consume FIFO", v==10);
    TEST("size 2", rb_size(rb)==2);
    rb_produce(rb,40); rb_produce(rb,50);
    TEST("full", rb_is_full(rb));
    TEST("full reject", !rb_produce(rb,99));
    rb_destroy(rb);
}

/* ── Red-black tree ───────────────────────────────────────────────────────── */
static void test_rbt(void) {
    printf("\n[Red-Black Tree]\n");
    RBTree *t=rbt_create();
    int keys[]={10,20,30,15,25,5,35,1,8,12};
    for(int i=0;i<10;i++) rbt_insert(t,keys[i],keys[i]);
    TEST("valid after inserts", rbt_is_valid(t));
    TEST("search 15",           rbt_search(t,15)!=NULL);
    TEST("search 99 missing",   rbt_search(t,99)==NULL);
    TEST("min=1 (O(1))",        rbt_minimum(t)->key==1);
    rbt_delete(t,10);
    TEST("valid after delete",  rbt_is_valid(t));
    TEST("10 deleted",          rbt_search(t,10)==NULL);
    rbt_delete(t,1);
    TEST("new min after del 1", rbt_minimum(t)->key==5);
    rbt_destroy(t);
}

/* ── B-tree ───────────────────────────────────────────────────────────────── */
static void test_btree(void) {
    printf("\n[B-tree]\n");
    BTree *t=bt_create();
    for(int i=1;i<=30;i++) bt_insert(t,i);
    TEST("search 15",   bt_search(t,15));
    TEST("search 31 no",!bt_search(t,31));
    bt_delete(t,15);
    TEST("15 deleted",  !bt_search(t,15));
    TEST("14 still ok", bt_search(t,14));
    bt_destroy(t);
}

/* ── Trie ─────────────────────────────────────────────────────────────────── */
static void test_trie(void) {
    printf("\n[Trie]\n");
    Trie *t=trie_create();
    trie_insert(t,"hello",1); trie_insert(t,"help",2); trie_insert(t,"world",3);
    TEST("search hello",   trie_search(t,"hello",NULL));
    TEST("search hell no", !trie_search(t,"hell",NULL));
    TEST("prefix hel",     trie_starts_with(t,"hel"));
    int lpm=trie_longest_prefix_match(t,"helpdesk");
    TEST("LPM helpdesk=4", lpm==4);
    trie_delete(t,"hello");
    TEST("hello deleted",  !trie_search(t,"hello",NULL));
    TEST("help still ok",  trie_search(t,"help",NULL));
    trie_destroy(t);
}

/* ── Segment tree ─────────────────────────────────────────────────────────── */
static void test_segtree(void) {
    printf("\n[Segment Tree]\n");
    int a[]={1,2,3,4,5,6,7,8};
    SegTree *st=segtree_create(a,8);
    TEST("sum [0..7]=36",  segtree_query_sum(st,0,7)==36);
    TEST("sum [2..5]=18",  segtree_query_sum(st,2,5)==18);
    segtree_range_update(st,2,5,10);
    TEST("sum [2..5] after +10=58", segtree_query_sum(st,2,5)==58);
    TEST("sum [0..1] unaffected=3", segtree_query_sum(st,0,1)==3);
    segtree_destroy(st);
}

/* ── Heap ─────────────────────────────────────────────────────────────────── */
static void test_heap(void) {
    printf("\n[Heap]\n");
    MinHeap *h=heap_create(16);
    int keys[]={7,3,9,1,5,8,2,6,4};
    for(int i=0;i<9;i++) heap_push(h,keys[i],i);
    int prev=heap_pop_key(h), ok=1;
    while(!heap_is_empty(h)) { int cur=heap_pop_key(h); if(cur<prev) ok=0; prev=cur; }
    TEST("min-heap order", ok);

    int arr[]={5,3,8,1,9,2,7,4,6};
    heapsort(arr,9);
    ok=1; for(int i=0;i<8;i++) if(arr[i]>arr[i+1]) ok=0;
    TEST("heapsort sorted", ok);
    heap_destroy(h);
}

/* ── Hash table ───────────────────────────────────────────────────────────── */
static void test_hash_table(void) {
    printf("\n[Hash Table]\n");
    HashTable *ht=ht_create(8);
    for(int i=0;i<20;i++) ht_insert(ht,i,i*10);
    int v;
    TEST("get 15=150",    ht_get(ht,15,&v)&&v==150);
    TEST("get 0=0",       ht_get(ht,0,&v)&&v==0);
    ht_delete(ht,15);
    TEST("15 deleted",    !ht_get(ht,15,&v));
    ht_destroy(ht);

    CuckooHash *ch=cuckoo_create(16);
    for(int i=0;i<8;i++) cuckoo_insert(ch,i,i*5);
    TEST("cuckoo get 3=15",   cuckoo_get(ch,3,&v)&&v==15);
    cuckoo_delete(ch,3);
    TEST("cuckoo 3 deleted",  !cuckoo_get(ch,3,&v));
    cuckoo_destroy(ch);
}

/* ── Graph ────────────────────────────────────────────────────────────────── */
static void test_graph(void) {
    printf("\n[Graph]\n");
    Graph *g=graph_create(6,true);
    graph_add_edge(g,5,2,1); graph_add_edge(g,5,0,1);
    graph_add_edge(g,4,0,1); graph_add_edge(g,4,1,1);
    graph_add_edge(g,2,3,1); graph_add_edge(g,3,1,1);
    bool is_dag; int *topo=graph_topological_sort(g,&is_dag);
    TEST("is DAG", is_dag);
    TEST("no cycle", !graph_has_cycle(g));
    free(topo); graph_destroy(g);

    /* Dijkstra */
    Graph *g2=graph_create(5,true);
    graph_add_edge(g2,0,1,4); graph_add_edge(g2,0,2,1);
    graph_add_edge(g2,2,1,2); graph_add_edge(g2,1,3,1);
    graph_add_edge(g2,3,4,3);
    int *dist=graph_dijkstra(g2,0);
    TEST("dijkstra d[1]=3", dist[1]==3);
    TEST("dijkstra d[4]=7", dist[4]==7);
    free(dist); graph_destroy(g2);
}

/* ── Union-Find ───────────────────────────────────────────────────────────── */
static void test_union_find(void) {
    printf("\n[Union-Find]\n");
    UnionFind *uf=uf_create(6);
    uf_union(uf,0,1); uf_union(uf,1,2); uf_union(uf,3,4);
    TEST("0-2 connected",   uf_connected(uf,0,2));
    TEST("0-3 not connected",!uf_connected(uf,0,3));
    TEST("components=3",    uf_num_components(uf)==3);
    uf_union(uf,2,3);
    TEST("0-4 connected after union", uf_connected(uf,0,4));
    TEST("components=2",    uf_num_components(uf)==2);
    uf_destroy(uf);
}

/* ── Bloom filter ─────────────────────────────────────────────────────────── */
static void test_bloom(void) {
    printf("\n[Bloom Filter]\n");
    BloomFilter *bf=bloom_create(1000,0.01);
    bloom_add(bf,"alpha",5); bloom_add(bf,"beta",4); bloom_add(bf,"gamma",5);
    TEST("alpha in set",  bloom_test(bf,"alpha",5));
    TEST("beta in set",   bloom_test(bf,"beta",4));
    TEST("delta not in",  !bloom_test(bf,"delta",5));  /* may have FP */
    bloom_destroy(bf);
}

/* ── Skip list ────────────────────────────────────────────────────────────── */
static void test_skip_list(void) {
    printf("\n[Skip List]\n");
    SkipList *sl=sl_create();
    for(int i=0;i<10;i++) sl_insert(sl,i,i*10);
    int v;
    TEST("search 5=50",   sl_search(sl,5,&v)&&v==50);
    TEST("search 99 no",  !sl_search(sl,99,&v));
    sl_delete(sl,5);
    TEST("5 deleted",     !sl_search(sl,5,&v));
    TEST("6 still ok",    sl_search(sl,6,&v)&&v==60);
    sl_destroy(sl);
}

/* ── HyperLogLog ──────────────────────────────────────────────────────────── */
static void test_hll(void) {
    printf("\n[HyperLogLog]\n");
    HyperLogLog *hll=hll_create(12);
    char buf[32];
    for(int i=0;i<1000;i++) { snprintf(buf,sizeof buf,"item_%d",i); hll_add(hll,buf,strlen(buf)); }
    uint64_t est=hll_estimate(hll);
    double err=100.0*((double)(long long)(est-1000))/1000.0;
    TEST("HLL 1000 items within 5%", err>-5.0&&err<5.0);
    hll_destroy(hll);
}

/* ── Bitmap ───────────────────────────────────────────────────────────────── */
static void test_bitmap(void) {
    printf("\n[Bitmap]\n");
    Bitmap *bm=bm_create(64);
    bm_set(bm,0); bm_set(bm,7); bm_set(bm,63);
    TEST("bit 0 set",     bm_test(bm,0));
    TEST("bit 63 set",    bm_test(bm,63));
    TEST("bit 1 clear",   !bm_test(bm,1));
    TEST("popcount=3",    bm_popcount(bm)==3);
    TEST("find_first=0",  bm_find_first_set(bm)==0);
    bm_clear(bm,0);
    TEST("find_first=7",  bm_find_first_set(bm)==7);
    bm_destroy(bm);
}

/* ── LSM tree ─────────────────────────────────────────────────────────────── */
static void test_lsm(void) {
    printf("\n[LSM Tree]\n");
    LSMTree *lsm=lsm_create();
    for(int i=0;i<12;i++) lsm_put(lsm,i,i*100);
    int v; bool found=lsm_get(lsm,7,&v);
    TEST("get 7=700",    found&&v==700);
    lsm_delete(lsm,7);
    TEST("7 tombstoned", !lsm_get(lsm,7,&v));
    lsm_compact(lsm,0);
    TEST("compact: 7 gone", !lsm_get(lsm,7,&v));
    TEST("compact: 8 ok",    lsm_get(lsm,8,&v)&&v==800);
    lsm_destroy(lsm);
}

/* ── kd-tree ──────────────────────────────────────────────────────────────── */
static void test_kd(void) {
    printf("\n[k-d Tree]\n");
    KDTree *kd=kd_create();
    double pts[][2]={{2,3},{5,4},{9,6},{4,7},{8,1},{7,2}};
    for(int i=0;i<6;i++) kd_insert(kd,pts[i][0],pts[i][1],i);
    KDNode *nn=kd_nearest(kd,9.0,2.0);
    TEST("nearest to (9,2) is (8,1)", nn&&nn->point[0]==8.0&&nn->point[1]==1.0);
    kd_destroy(kd);
}

int main(void) {
    printf("╔══════════════════════════════════════════════════╗\n");
    printf("║         DSA Guide — Test Suite             ║\n");
    printf("╚══════════════════════════════════════════════════╝\n");

    test_array();
    test_linked_list();
    test_stack();
    test_queue();
    test_ring_buffer();
    test_rbt();
    test_btree();
    test_trie();
    test_segtree();
    test_heap();
    test_hash_table();
    test_graph();
    test_union_find();
    test_bloom();
    test_skip_list();
    test_hll();
    test_bitmap();
    test_lsm();
    test_kd();

    printf("\n══════════════════════════════════════════════════\n");
    printf("Results: %d / %d passed\n", tests_passed, tests_run);
    printf("══════════════════════════════════════════════════\n");
    return (tests_passed == tests_run) ? 0 : 1;
}
