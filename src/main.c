/*
 * main.c - DSA Guide demo runner
 *
 * Exercises every data structure and algorithm with labelled output
 * so students can trace what each operation does and why.
 *
 * Run:  ./dsa_demo
 */
#include "dsa.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SECTION(name) \
    printf("\n????????????????????????????????????????????????????\n"); \
    printf("?  %-48s?\n", name); \
    printf("????????????????????????????????????????????????????\n");

#define DEMO(name) printf("\n?? %s ??\n", name);

/* ?? helpers ???????????????????????????????????????????????????????????????? */
static void print_int_arr(const char *label, const int *arr, int n) {
    printf("%s: [", label);
    for (int i=0;i<n;i++) printf("%d%s", arr[i], i<n-1?", ":"");
    printf("]\n");
}

static void visit_rbt(RBNode *n) { printf("  %d(%s)\n", n->key, n->color==RB_RED?"R":"B"); }
static void visit_range(int k, int v) { printf("  range hit: key=%d val=%d\n", k, v); }
static void visit_auto(const char *key, int val) { printf("  autocomplete: %s ? %d\n", key, val); }
static void visit_kd(KDNode *n) { printf("  in-range: (%.1f, %.1f) val=%d\n", n->point[0], n->point[1], n->val); }
static void visit_interval(ITreeNode *n) { printf("  overlap: [%d,%d]\n", n->lo, n->hi); }
static void visit_rtree(int id, MBR mbr) { printf("  rtree hit: id=%d MBR=(%.1f,%.1f)-(%.1f,%.1f)\n", id, mbr.x1,mbr.y1,mbr.x2,mbr.y2); }
static void visit_btree(int k) { printf("  %d ", k); }

/* ??????????????????????????????????????????????????????????????????????????? */
int main(void) {
    printf("DSA Guide - C Implementation Demo\n");
    printf("=========================================\n");

    /* ?? 1. ARRAY ??????????????????????????????????????????????????????????? */
    SECTION("1. Array / Buffer")

    DEMO("Binary search")
    Array *arr = array_create(10);
    int sorted[] = {1,3,5,7,9,11,13,15,17,19};
    for (int i=0;i<10;i++) array_push(arr, sorted[i]);
    printf("Array: "); for(size_t i=0;i<arr->size;i++) printf("%d ",arr->data[i]); printf("\n");
    printf("Binary search 11 ? index %d (expect 5)\n", array_binary_search(arr, 11));
    printf("Binary search  4 ? index %d (expect -1)\n", array_binary_search(arr, 4));

    DEMO("Sliding window max (k=3)")
    int sw[] = {2,1,5,3,6,4,8,7};
    Array *swa = array_create(8);
    for (int i=0;i<8;i++) array_push(swa, sw[i]);
    Array *result = array_sliding_window_max(swa, 3);
    print_int_arr("Input  ", sw, 8);
    printf("Window max (k=3): [");
    for (size_t i=0;i<result->size;i++) printf("%d%s",result->data[i],i<result->size-1?", ":"");
    printf("] (expect [5,5,6,6,8,8])\n");

    DEMO("Prefix sum")
    int ps[] = {1,2,3,4,5};
    Array *pa = array_create(5);
    for (int i=0;i<5;i++) array_push(pa, ps[i]);
    Array *prefix = array_prefix_sum_build(pa);
    printf("Sum [1..3] = %d (expect 9)\n", array_prefix_sum_query(prefix, 1, 3));
    printf("Sum [0..4] = %d (expect 15)\n", array_prefix_sum_query(prefix, 0, 4));

    array_destroy(arr); array_destroy(swa); array_destroy(result);
    array_destroy(pa);  array_destroy(prefix);

    /* ?? 2. LINKED LIST ????????????????????????????????????????????????????? */
    SECTION("2. Linked List")

    DEMO("Push/reversal/merge sort")
    LinkedList *ll = ll_create();
    int vals[] = {5,3,8,1,9,2,7,4,6};
    for (int i=0;i<9;i++) ll_push_back(ll, vals[i]);
    printf("Before sort: "); ll_print(ll);
    ll_merge_sort(ll);
    printf("After  sort: "); ll_print(ll);
    ll_reverse(ll);
    printf("Reversed:    "); ll_print(ll);

    DEMO("Cycle detection")
    printf("Has cycle: %s (expect false)\n", ll_has_cycle(ll)?"true":"false");
    ll_destroy(ll);

    /* ?? 3. STACK ??????????????????????????????????????????????????????????? */
    SECTION("3. Stack")

    DEMO("Postfix expression eval")
    printf("\"3 4 + 2 *\" = %d (expect 14)\n", stack_eval_postfix("3 4 + 2 *"));
    printf("\"5 1 2 + 4 * + 3 -\" = %d (expect 14)\n",
           stack_eval_postfix("5 1 2 + 4 * + 3 -"));

    DEMO("Balanced brackets")
    printf("{[()]} balanced: %s\n", stack_balanced_brackets("{[()]}")?"yes":"no");
    printf("{[(])} balanced: %s\n", stack_balanced_brackets("{[(])}")?"yes":"no");

    /* ?? 4. QUEUE + DEQUE ??????????????????????????????????????????????????? */
    SECTION("4. Queue / Deque")

    DEMO("BFS on simple graph")
    /* Graph: 0?1,2  1?3  2?3  3?4 */
    int adj0[]={2,1,2}, adj1[]={1,3}, adj2[]={1,3}, adj3[]={1,4}, adj4[]={0};
    int *adj[]={(int*)adj0,(int*)adj1,(int*)adj2,(int*)adj3,(int*)adj4};
    int bfs_n;
    int *bfs_order = queue_bfs(adj, 5, 0, &bfs_n);
    print_int_arr("BFS from 0", bfs_order, bfs_n);
    free(bfs_order);

    DEMO("Monotonic deque sliding window max")
    int sw2[]={2,1,5,3,6,4,8,7}; int swlen;
    int *swmax = deque_sliding_window_max(sw2, 8, 3, &swlen);
    print_int_arr("Window max", swmax, swlen);
    free(swmax);

    /* ?? 5. RING BUFFER ????????????????????????????????????????????????????? */
    SECTION("5. Ring Buffer (Lock-free SPSC)")

    RingBuffer *rb = rb_create(8);
    for (int i=0;i<5;i++) rb_produce(rb, i*10);
    printf("Size after 5 produces: %zu\n", rb_size(rb));
    int v; rb_consume(rb, &v); printf("Consumed: %d\n", v);
    rb_consume(rb, &v); printf("Consumed: %d\n", v);
    printf("Size now: %zu\n", rb_size(rb));
    rb_destroy(rb);

    /* ?? 6. CIRCULAR LIST (LRU) ????????????????????????????????????????????? */
    SECTION("6. Circular / Doubly-Linked List - LRU Cache")

    CircularList *cl = clist_create();
    printf("LRU cache (max=4). Accessing: 1,2,3,4,1,5\n");
    int accesses[] = {1,2,3,4,1,5};
    for (int i=0;i<6;i++) {
        int evicted = clist_lru_access(cl, accesses[i], 4);
        printf("  access(%d) ? ", accesses[i]);
        if (evicted != -1) printf("evicted %d  ", evicted);
        clist_print(cl);
    }
    clist_destroy(cl);

    /* ?? 7. RED-BLACK TREE ?????????????????????????????????????????????????? */
    SECTION("7. Red-Black Tree (CFS scheduler pattern)")

    RBTree *rbt = rbt_create();
    int rbt_keys[] = {10,20,30,15,25,5,35,1,8};
    for (int i=0;i<9;i++) rbt_insert(rbt, rbt_keys[i], rbt_keys[i]*10);
    printf("In-order traversal (should be sorted):\n");
    rbt_inorder(rbt, visit_rbt);
    printf("Valid RB tree: %s\n", rbt_is_valid(rbt)?"yes":"NO!");
    printf("Min (CFS pick-next O(1)): key=%d\n", rbt_minimum(rbt)->key);
    rbt_delete(rbt, 20);
    printf("After delete(20), min=%d, valid=%s\n",
           rbt_minimum(rbt)->key, rbt_is_valid(rbt)?"yes":"NO!");
    rbt_destroy(rbt);

    /* ?? 8. B-TREE ?????????????????????????????????????????????????????????? */
    SECTION("8. B-tree (filesystem/DB index)")

    BTree *bt = bt_create();
    for (int i=1;i<=20;i++) bt_insert(bt, i*3);
    printf("Search 15: %s\n", bt_search(bt,15)?"found":"not found");
    printf("Search 14: %s\n", bt_search(bt,14)?"found":"not found");
    printf("Range scan [10..30]: ");
    bt_range_scan(bt, 10, 30, visit_btree); printf("\n");
    bt_delete(bt, 15);
    printf("After delete(15), search 15: %s\n", bt_search(bt,15)?"found":"not found");
    bt_destroy(bt);

    /* ?? 9. TRIE ???????????????????????????????????????????????????????????? */
    SECTION("9. Trie / Radix Tree (IP routing LPM)")

    Trie *trie = trie_create();
    trie_insert(trie, "192.168",   1);
    trie_insert(trie, "192.168.1", 2);
    trie_insert(trie, "10.0",      3);
    trie_insert(trie, "10.0.0.1",  4);
    printf("Search '192.168':   %s\n", trie_search(trie,"192.168",NULL)?"found":"not found");
    printf("Search '192.169':   %s\n", trie_search(trie,"192.169",NULL)?"found":"not found");
    printf("Starts with '10.':  %s\n", trie_starts_with(trie,"10.")?"yes":"no");
    printf("LPM '192.168.1.55': %d chars matched\n",
           trie_longest_prefix_match(trie, "192.168.1.55"));
    printf("LPM '10.0.5.1':     %d chars matched\n",
           trie_longest_prefix_match(trie, "10.0.5.1"));
    printf("Autocomplete '192.':\n");
    trie_autocomplete(trie, "192.", visit_auto);
    trie_destroy(trie);

    /* ?? 10. SEGMENT TREE ??????????????????????????????????????????????????? */
    SECTION("10. Segment / Interval Tree")

    DEMO("Segment tree range sum + lazy update")
    int st_arr[] = {1,2,3,4,5,6,7,8};
    SegTree *st = segtree_create(st_arr, 8);
    printf("Sum [2..5] = %d (expect 18)\n", segtree_query_sum(st, 2, 5));
    segtree_range_update(st, 2, 5, 10);   /* add 10 to positions 2..5 */
    printf("After +10 to [2..5], sum [2..5] = %d (expect 58)\n",
           segtree_query_sum(st, 2, 5));
    segtree_destroy(st);

    DEMO("Interval tree stabbing + overlap")
    IntervalTree *it = itree_create();
    itree_insert(it, 1,  5,  10);
    itree_insert(it, 3,  9,  20);
    itree_insert(it, 6,  12, 30);
    itree_insert(it, 14, 18, 40);
    ITreeNode *hit = itree_stab(it, 7);
    printf("Stab point=7: [%d,%d]\n", hit?hit->lo:0, hit?hit->hi:0);
    printf("Overlaps with [4,10]:\n");
    itree_overlap(it, 4, 10, visit_interval);
    itree_destroy(it);

    /* ?? 11. HEAP ??????????????????????????????????????????????????????????? */
    SECTION("11. Heap / Priority Queue + Heapsort + Dijkstra")

    DEMO("Min-heap and heapsort")
    MinHeap *h = heap_create(16);
    int hkeys[] = {7,3,9,1,5,8,2,6,4};
    for (int i=0;i<9;i++) heap_push(h, hkeys[i], i);
    printf("Extract-min sequence: ");
    while (!heap_is_empty(h)) printf("%d ", heap_pop_key(h));
    printf("\n");

    int hs[] = {5,3,8,1,9,2,7,4,6};
    heapsort(hs, 9);
    print_int_arr("Heapsort result", hs, 9);
    heap_destroy(h);

    DEMO("Dijkstra shortest path")
    Graph *g = graph_create(5, true);
    graph_add_edge(g, 0, 1, 4);
    graph_add_edge(g, 0, 2, 1);
    graph_add_edge(g, 2, 1, 2);
    graph_add_edge(g, 1, 3, 1);
    graph_add_edge(g, 2, 3, 5);
    graph_add_edge(g, 3, 4, 3);
    int *dist = graph_dijkstra(g, 0);
    printf("Shortest from 0: ");
    for (int i=0;i<5;i++) printf("[%d]=%d ", i, dist[i]);
    printf("\n(expect 0?0, 1?3, 2?1, 3?4, 4?7)\n");
    free(dist); graph_destroy(g);

    /* ?? 12. HASH TABLE ????????????????????????????????????????????????????? */
    SECTION("12. Hash Table (chaining + open addressing)")

    HashTable *ht = ht_create(8);
    for (int i=0;i<10;i++) ht_insert(ht, i, i*100);
    int got;
    printf("get(5) = %d (expect 500)\n", ht_get(ht,5,&got)?got:-1);
    ht_delete(ht, 5);
    printf("get(5) after delete = %d (expect -1)\n", ht_get(ht,5,&got)?got:-1);
    ht_destroy(ht);

    DEMO("Cuckoo hash - O(1) worst-case lookup")
    CuckooHash *ch = cuckoo_create(16);
    for (int i=0;i<8;i++) cuckoo_insert(ch, i, i*10);
    printf("cuckoo get(3) = %d (expect 30)\n", cuckoo_get(ch,3,&got)?got:-1);
    cuckoo_delete(ch, 3);
    printf("cuckoo get(3) after delete = %d\n", cuckoo_get(ch,3,&got)?got:-1);
    cuckoo_destroy(ch);

    /* ?? 13. GRAPH ?????????????????????????????????????????????????????????? */
    SECTION("13. Graph - BFS / DFS / Topo-sort")

    Graph *g2 = graph_create(6, true);
    graph_add_edge(g2, 5, 2, 1); graph_add_edge(g2, 5, 0, 1);
    graph_add_edge(g2, 4, 0, 1); graph_add_edge(g2, 4, 1, 1);
    graph_add_edge(g2, 2, 3, 1); graph_add_edge(g2, 3, 1, 1);

    int topo_n; bool is_dag;
    int *topo = graph_topological_sort(g2, &is_dag);
    printf("Is DAG: %s\n", is_dag?"yes":"no");
    print_int_arr("Topological order", topo, topo_n = g2->n);
    free(topo);
    printf("Has cycle: %s (expect false)\n", graph_has_cycle(g2)?"true":"false");
    graph_destroy(g2);

    /* ?? 14. DAG ???????????????????????????????????????????????????????????? */
    SECTION("14. DAG - Critical Path")

    DAG *dag = dag_create(5);
    dag_add_edge(dag,0,1,3); dag_add_edge(dag,0,2,2);
    dag_add_edge(dag,1,3,4); dag_add_edge(dag,2,3,1);
    dag_add_edge(dag,3,4,2);
    int cp_len; int *cp = dag_critical_path(dag, &cp_len);
    printf("Critical path length: %d\n", cp_len);
    printf("Distances from source: ");
    for (int i=0;i<5;i++) printf("[%d]=%d ",i,cp[i]);
    printf("\n");
    int tn; int *tk = dag_topo_kahn(dag, &tn);
    print_int_arr("Kahn's topo order", tk, tn);
    free(cp); free(tk); dag_destroy(dag);

    /* ?? 15. UNION-FIND ????????????????????????????????????????????????????? */
    SECTION("15. Union-Find (DSU)")

    UnionFind *uf = uf_create(7);
    uf_union(uf,0,1); uf_union(uf,1,2);
    uf_union(uf,3,4); uf_union(uf,5,6);
    printf("Components: %zu (expect 3)\n", uf_num_components(uf));
    printf("0 and 2 connected: %s\n", uf_connected(uf,0,2)?"yes":"no");
    printf("0 and 3 connected: %s\n", uf_connected(uf,0,3)?"yes":"no");
    uf_union(uf,2,3);
    printf("After union(2,3), 0 and 4 connected: %s\n", uf_connected(uf,0,4)?"yes":"no");
    printf("Components now: %zu (expect 2)\n", uf_num_components(uf));
    uf_destroy(uf);

    /* ?? 16. BLOOM FILTER ??????????????????????????????????????????????????? */
    SECTION("16. Bloom Filter")

    BloomFilter *bf = bloom_create(1000, 0.01);
    printf("Created bloom filter: %d hashes\n", bf->num_hashes);
    bloom_add(bf, "192.168.1.1", 11);
    bloom_add(bf, "10.0.0.1", 8);
    printf("Test '192.168.1.1': %s (expect yes)\n",
           bloom_test(bf,"192.168.1.1",11)?"yes - in set":"no");
    printf("Test '8.8.8.8':     %s\n",
           bloom_test(bf,"8.8.8.8",7)?"possible hit (FP)":"no - definitely not in set");
    printf("Estimated FP rate: %.4f\n", bloom_fp_rate(bf));
    bloom_destroy(bf);

    /* ?? 17. SKIP LIST ?????????????????????????????????????????????????????? */
    SECTION("17. Skip List (Redis sorted set / RocksDB memtable)")

    SkipList *sl = sl_create();
    for (int i=0;i<8;i++) sl_insert(sl, (i*7)%13, i);
    int sv;
    printf("Search key 6: %s\n", sl_search(sl,6,&sv)?"found":"not found");
    printf("Range [2..8]: "); sl_range(sl,2,8,visit_range);
    sl_delete(sl, 6);
    printf("After delete(6), search: %s\n", sl_search(sl,6,&sv)?"found":"not found");
    sl_destroy(sl);

    /* ?? 18. COUNT-MIN + HYPERLOGLOG ??????????????????????????????????????? */
    SECTION("18. Count-Min Sketch + HyperLogLog")

    DEMO("Count-Min: frequency estimation")
    CountMin *cm = cm_create(0.01, 0.01);
    int stream[] = {1,2,1,3,1,2,4,1,5,1,2,3,1};
    for (int i=0;i<13;i++) cm_add_int(cm, stream[i]);
    printf("Freq estimate of 1: %u (true=6)\n", cm_query_int(cm, 1));
    printf("Freq estimate of 2: %u (true=3)\n", cm_query_int(cm, 2));
    printf("Freq estimate of 5: %u (true=1)\n", cm_query_int(cm, 5));
    cm_destroy(cm);

    DEMO("HyperLogLog: cardinality estimation")
    HyperLogLog *hll = hll_create(12);  /* b=12: ~1.6% error */
    char buf[32];
    for (int i=0;i<10000;i++) { snprintf(buf,sizeof buf,"item_%d",i); hll_add(hll,buf,strlen(buf)); }
    uint64_t est = hll_estimate(hll);
    printf("True count=10000, HLL estimate=%llu (error=%.2f%%)\n",
           (unsigned long long)est, 100.0*((double)est-10000)/10000);
    hll_destroy(hll);

    /* ?? 19. k-d TREE ??????????????????????????????????????????????????????? */
    SECTION("19. k-d Tree (spatial nearest-neighbour)")

    KDTree *kd = kd_create();
    double points[][2] = {{2,3},{5,4},{9,6},{4,7},{8,1},{7,2}};
    for (int i=0;i<6;i++) kd_insert(kd, points[i][0], points[i][1], i);
    KDNode *nn = kd_nearest(kd, 9.0, 2.0);
    printf("Nearest to (9,2): (%.0f,%.0f) val=%d (expect (8,1))\n",
           nn->point[0], nn->point[1], nn->val);
    printf("Points within r=3 of (5,5):\n");
    kd_range_search(kd, 5.0, 5.0, 3.0, visit_kd);
    kd_destroy(kd);

    /* ?? 20. R-TREE ????????????????????????????????????????????????????????? */
    SECTION("20. R-tree (bounding-box spatial index)")

    RTree *rt = rtree_create();
    rtree_insert(rt, 1,1,3,3, 1);
    rtree_insert(rt, 2,2,4,4, 2);
    rtree_insert(rt, 5,5,7,7, 3);
    rtree_insert(rt, 6,1,8,3, 4);
    printf("Search MBR (1.5,1.5)-(3.5,3.5):\n");
    rtree_search(rt, 1.5,1.5,3.5,3.5, visit_rtree);
    rtree_destroy(rt);

    /* ?? 21. LSM TREE ??????????????????????????????????????????????????????? */
    SECTION("21. LSM Tree (RocksDB / write-optimised KV)")

    LSMTree *lsm = lsm_create();
    for (int i=0;i<12;i++) lsm_put(lsm, i, i*100);  /* triggers flush at 8 */
    int lv; bool found = lsm_get(lsm, 5, &lv);
    printf("get(5) = %d (expect 500)\n", found?lv:-1);
    lsm_delete(lsm, 5);
    found = lsm_get(lsm, 5, &lv);
    printf("get(5) after delete = %s\n", found?"found":"not found (tombstone)");
    lsm_compact(lsm, 0);
    printf("After compaction, get(5) = %s\n", lsm_get(lsm,5,&lv)?"found":"not found");
    lsm_destroy(lsm);

    /* ?? 22. BITMAP ????????????????????????????????????????????????????????? */
    SECTION("22. Bitmap / Bitset (RT scheduler, CPU masks)")

    Bitmap *bm = bm_create(64);
    int bits_to_set[] = {0,3,7,15,31,63};
    for (int i=0;i<6;i++) bm_set(bm, (size_t)bits_to_set[i]);
    bm_print(bm);
    printf("find_first_set: %d (expect 0)\n", bm_find_first_set(bm));
    bm_clear(bm, 0);
    printf("After clear(0), find_first_set: %d (expect 3)\n", bm_find_first_set(bm));
    printf("popcount: %zu (expect 5)\n", bm_popcount(bm));
    bm_destroy(bm);

    /* ?? 23. WAVELET TREE ??????????????????????????????????????????????????? */
    SECTION("23. Wavelet Tree (rank/select/quantile)")

    int wseq[] = {3,1,4,1,5,9,2,6,5,3};
    WaveletTree *wt = wt_build(wseq, 10, 10);
    printf("rank(1, 5) = %d (count of 1s in [0..4], expect 2)\n", wt_rank(wt,1,5));
    printf("rank(5, 8) = %d (count of 5s in [0..7], expect 1)\n", wt_rank(wt,5,8));
    printf("select(1,2) = pos %d (2nd occurrence of 1, expect 3)\n", wt_select(wt,1,2));
    printf("quantile(0,9,5) = %d (5th smallest in full array, expect 4)\n",
           wt_quantile(wt,0,9,5));
    wt_destroy(wt);

    /* ?? 24. BIT-OPERATION ALGORITHMS ?????????????????????????????????????? */
    SECTION("24. Bit-Operation Algorithms")

    DEMO("Fundamentals: field extraction and mutation")
    uint32_t reg = 0b10110100u;           /* imagine a hardware register      */
    bits_demo_print_u32("register",             reg);
    bits_demo_print_u32("set bit 0",            bit_set(reg, 0));
    bits_demo_print_u32("clear bit 2",          bit_clear(reg, 2));
    bits_demo_print_u32("toggle bit 7",         bit_toggle(reg, 7));
    printf("bit_field(reg, 2, 5)   = 0x%X  (bits 5..2, expect 0xD)\n",
           bit_field(reg, 2, 5));
    printf("bit_field_set(..., 4)  = 0x%X\n",
           bit_field_set(reg, 2, 5, 4));

    DEMO("Arithmetic without +/-/*//")
    printf("bit_add(17, 25)        = %d  (expect 42)\n",  bit_add(17, 25));
    printf("bit_add(-10, 4)        = %d  (expect -6)\n",  bit_add(-10, 4));
    printf("bit_sub(100, 58)       = %d  (expect 42)\n",  bit_sub(100, 58));
    printf("bit_mul(6, 7)          = %d  (expect 42)\n",  bit_mul(6, 7));
    printf("bit_mul(-3, 14)        = %d  (expect -42)\n", bit_mul(-3, 14));
    printf("bit_div(100, 7)        = %u  (expect 14)\n",  bit_div(100, 7));
    printf("bit_abs(-42)           = %d  (expect 42)\n",  bit_abs(-42));

    DEMO("Counting: popcount, parity, CTZ, CLZ, floor_log2")
    uint32_t bv = 0b10110110u;
    printf("value                  = 0b10110110\n");
    printf("bit_popcount32         = %d  (expect 5)\n",  bit_popcount32(bv));
    printf("bit_parity             = %d  (expect 1 - odd)\n", bit_parity(bv));
    printf("bit_ctz (lowest set)   = %d  (expect 1)\n", bit_ctz(bv));
    printf("bit_clz (leading zeros)= %d  (expect 24)\n", bit_clz(bv));
    printf("bit_floor_log2         = %d  (expect 7)\n",  bit_floor_log2(bv));

    DEMO("Rounding: next/prev power-of-two, alignment")
    uint32_t sizes[] = {1, 5, 8, 9, 255, 256, 1000};
    for (int i = 0; i < 7; i++)
        printf("next_pow2(%4u) = %4u   prev_pow2(%4u) = %4u\n",
               sizes[i], bit_next_pow2(sizes[i]),
               sizes[i], bit_prev_pow2(sizes[i]));
    printf("align_up(13, 8)        = %u  (expect 16)\n", bit_align_up(13, 8));
    printf("align_down(13, 8)      = %u  (expect 8)\n",  bit_align_down(13, 8));
    printf("is_aligned(16, 8)      = %s\n", bit_is_aligned(16,8)?"yes":"no");
    printf("is_aligned(13, 8)      = %s\n", bit_is_aligned(13,8)?"yes":"no");

    DEMO("Isolation: LSB, MSB, bit_reverse")
    uint32_t w = 0b10110100u;
    bits_demo_print_u32("original",              w);
    bits_demo_print_u32("isolate LSB",           bit_lsb(w));
    bits_demo_print_u32("clear LSB",             bit_clear_lsb(w));
    bits_demo_print_u32("isolate MSB",           bit_msb(w));
    bits_demo_print_u32("bit_reverse",           bit_reverse(w));
    printf("bit_reverse(bit_reverse(x)) == x: %s\n",
           bit_reverse(bit_reverse(w)) == w ? "yes" : "NO");

    DEMO("Branchless predicates: is_pow2, same_sign, min, max, xor_swap")
    printf("is_pow2(64)            = %s\n", bit_is_pow2(64)?"yes":"no");
    printf("is_pow2(63)            = %s\n", bit_is_pow2(63)?"yes":"no");
    printf("is_pow2(0)             = %s\n", bit_is_pow2(0)?"yes":"no");
    printf("same_sign(-1, -99)     = %s\n", bit_same_sign(-1,-99)?"yes":"no");
    printf("same_sign(-1,  99)     = %s\n", bit_same_sign(-1, 99)?"yes":"no");
    printf("bit_min(17, 42)        = %d  (expect 17)\n", bit_min(17, 42));
    printf("bit_max(17, 42)        = %d  (expect 42)\n", bit_max(17, 42));
    int32_t p = 7, q = 13;
    bit_xor_swap(&p, &q);
    printf("xor_swap(7,13)         ? p=%d q=%d  (expect 13 7)\n", p, q);

    DEMO("Integer coding: Gray code, zigzag, bswap, Morton")
    for (uint32_t n = 0; n < 8; n++) {
        uint32_t g = bit_to_gray(n);
        printf("  binary=%u  gray=0b%u%u%u  decode=%u\n",
               n, (g>>2)&1, (g>>1)&1, g&1, bit_from_gray(g));
    }
    printf("zigzag_encode(-3)      = %u  (expect 5)\n",  bit_zigzag_encode(-3));
    printf("zigzag_decode(5)       = %d  (expect -3)\n", bit_zigzag_decode(5));
    printf("bswap32(0x11223344)    = 0x%08X  (expect 0x44332211)\n",
           bit_bswap32(0x11223344u));
    uint32_t morton = bit_morton_encode(3, 5);   /* x=3=0b011, y=5=0b101 */
    uint16_t mx, my;
    bit_morton_decode(morton, &mx, &my);
    printf("morton_encode(3,5)     = 0x%X  decode ? x=%u y=%u\n",
           morton, mx, my);

    DEMO("Classic puzzles: single number, missing number, two singles")
    int32_t arr1[] = {4, 1, 2, 1, 2};
    printf("single_number([4,1,2,1,2])   = %d  (expect 4)\n",
           bit_single_number(arr1, 5));

    int32_t arr2[] = {0, 1, 3, 4};          /* missing 2 from [0..4] */
    printf("missing_number([0,1,3,4])    = %d  (expect 2)\n",
           bit_missing_number(arr2, 4));

    int32_t arr3[] = {1, 2, 1, 3, 2, 5};    /* singles are 3 and 5  */
    int32_t sa, sb;
    bit_two_singles(arr3, 6, &sa, &sb);
    printf("two_singles([1,2,1,3,2,5])   = %d and %d  (expect 3 and 5)\n",
           sa, sb);

    DEMO("Subset enumeration (bitmask = 0b0111, items {0,1,2})")
    {
        int subset_count = 0;
        void count_and_print(uint32_t s, void *ctx) {
            (void)ctx;
            printf("  subset 0b%u%u%u\n", (s>>2)&1, (s>>1)&1, s&1);
            subset_count++;   /* note: nested fn for demo only; use closure in prod */
        }
        bit_each_subset(0b0111u, count_and_print, NULL);
        printf("total subsets = %d  (expect 7 = 2^3 - 1)\n", subset_count);
    }

    DEMO("Packed byte ops: has_zero_byte, byte_eq_mask")
    uint32_t no_zero  = 0x41424344u;   /* "ABCD" */
    uint32_t has_zero = 0x41004344u;   /* "A\0CD" */
    printf("has_zero_byte(0x41424344) = %s  (expect no)\n",
           bit_has_zero_byte(no_zero) ? "yes" : "no");
    printf("has_zero_byte(0x41004344) = %s  (expect yes)\n",
           bit_has_zero_byte(has_zero) ? "yes" : "no");
    uint32_t eq_mask = bit_byte_eq_mask(0x41424344u, 0x41004399u);
    printf("byte_eq_mask(ABCD, A?C?) = 0x%08X  (bytes 0,2 match ? expect 0x00FF00FF)\n",
           eq_mask);

    printf("\n????????????????????????????????????????????????????\n");
    printf("?  All demos complete.                             ?\n");
    printf("?  Build with: mkdir build && cd build            ?\n");
    printf("?              cmake .. && make && ./dsa_demo      ?\n");
    printf("????????????????????????????????????????????????????\n");
    return 0;
}
