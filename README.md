# DSA Guide — C Implementation

A comprehensive reference implementation of **25 data structures** and
**60+ algorithms** organised by category, with real-world domain annotations
for every structure.

## Project layout

```
dsa_study_guide/
├── CMakeLists.txt
├── include/
│   ├── dsa.h                   ← single master include
│   ├── linear/                 ← array, linked_list, stack, queue,
│   │                              ring_buffer, circular_list
│   ├── trees/                  ← red_black_tree, btree, trie,
│   │                              segment_tree, heap
│   ├── hash/                   ← hash_table, cuckoo_hash
│   ├── graph/                  ← graph, dag, union_find
│   ├── probabilistic/          ← bloom_filter, skip_list,
│   │                              count_min, hyperloglog
│   ├── spatial/                ← kd_tree, rtree
│   └── specialized/            ← lsm_tree, bitmap, wavelet_tree
├── src/                        ← implementations mirror include/
├── tests/
│   └── test_all.c              ← ~80 unit tests
└── ALGORITHMS.md               ← complexity cheat-sheet
└── README.md                   ← README
```

## Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# Run the annotated demo
./dsa_demo

# Run unit tests
ctest --output-on-failure
# or directly:
./test_all
```

## Data structures covered

| Category | Structures |
|---|---|
| **Linear** | Array, Linked list, Stack, Queue+Deque, Ring buffer, Circular list |
| **Trees** | Red-black tree, B-tree, Trie/Radix, Segment+Interval tree, Heap |
| **Hash** | Chaining hash table, Open-address hash, Cuckoo hash |
| **Graph** | Adjacency-list graph, DAG, Union-Find (DSU) |
| **Probabilistic** | Bloom filter, Counting bloom, Skip list, Count-Min sketch, HyperLogLog |
| **Spatial** | k-d tree, R-tree |
| **Specialized** | LSM tree, Bitmap/bitset, Wavelet tree |

## Algorithms covered

| Algorithm | Complexity | Where used |
|---|---|---|
| Binary search | O(log n) | Sorted arrays, page cache lookup |
| Two-pointer | O(n) | Pair sum, container problems |
| Sliding window max | O(n) | GPU max-pool, SQL OVER() |
| Merge sort (lists) | O(n log n) | sk_buff chain sort |
| Floyd cycle detect | O(n) | Kernel list integrity |
| DFS (iterative) | O(V+E) | Module dependency |
| BFS | O(V+E) | Shortest hop routing |
| Dijkstra | O((V+E) log V) | OSPF, GPS routing |
| Bellman-Ford | O(VE) | BGP (negative weights) |
| Kahn's topo-sort | O(V+E) | Build systems, CUDA scheduling |
| RB-tree rotations | O(log n) | CFS scheduler, VMA lookup |
| B-tree split/merge | O(log n) | Filesystem/DB index |
| Longest prefix match | O(L) | IP routing FIB trie |
| Segment tree lazy | O(log n) | Range update/query |
| Interval stabbing | O(log n) | VMA lookup, firewall rules |
| Heapsort | O(n log n) | In-place sort |
| Path compression | O(α) | Union-Find |
| Bloom k-hash | O(k) | RocksDB level skip |
| Count-Min sketch | O(k) | DDoS heavy hitters |
| HyperLogLog | O(1) space | COUNT DISTINCT |
| k-d NN search | O(log n) avg | k-NN, ray tracing |
| LSM compaction | O(n log n) | RocksDB, Cassandra |
| Wavelet rank/select | O(log σ) | Succinct column store |
| Lock-free SPSC ring | O(1) | NIC RX/TX, eBPF ring |
| Cuckoo displacement | O(1) worst | Firewall flow table |

## Domain annotations

Every header file documents which real systems use that structure:

- **OS / kernel**: CFS scheduler (RB-tree), page cache (XArray/trie),
  RT priority bitmap, ring buffer (eBPF, kfifo), VMA interval tree,
  list_head (circular doubly-linked list)
- **Networking**: FIB trie (LPM), conntrack (hash), sk_buff (linked list),
  NIC RX/TX ring, Qdisc (queue), Count-Min for DDoS detection
- **Storage / FS**: B-tree (ext4/btrfs/xfs), LSM (RocksDB/Cassandra),
  Bloom filter (level skip), WAL ring buffer, extent interval tree
- **GPU / compute**: CUDA task queue, ring buffer command queue,
  k-d tree (BVH ray tracing), R-tree (BVH), warp lane bitmap
- **Databases**: Hash join, B+ index scan, HyperLogLog COUNT DISTINCT,
  Skip list (Redis sorted set), Wavelet tree (DuckDB column store)
- **Security**: ACL bitmap, cert chain (linked list), Bloom URL screening,
  attack graph (graph/DSU), shadow stack (stack)
- **ML / AI**: Tensor (array), BPE trie, embedding hash table,
  HLL dataset sizing, k-d tree k-NN, autograd DAG, beam search heap

## Key design decisions

**Why C11?**  
Atomics (`<stdatomic.h>`) are needed for the lock-free ring buffer.
Everything else uses standard C99 constructs.

**Why no generics?**  
Clarity over flexibility. Each structure operates on `int` keys/values
so students can read and trace without `void*` casting noise.
Extending to generic types is left as an exercise.

**Why leftmost cache in RB-tree?**  
Mirrors Linux CFS: `rbt_minimum()` is O(1) because `t->leftmost` is
updated on every insert/delete. This is how the scheduler picks the
next task in O(1) while maintaining O(log n) for all other operations.

**Why counting Bloom supports deletion?**  
The standard Bloom filter uses single bits — no deletion.
The counting variant uses 4-bit counters so `remove()` is safe,
at the cost of 4× space. Same trade-off as kernel RCU vs spinlock.

**Why LSM compaction removes tombstones?**  
A tombstone (delete marker) must survive until compaction reaches the
level that holds the original key. At that point both are merged and
removed together — otherwise the delete is invisible after compaction.
