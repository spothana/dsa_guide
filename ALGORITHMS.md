# DSA Guide — Algorithms Reference

Complete complexity cheat-sheet, invariants, domain applications,
and "why it works" notes for every algorithm in this project.

---

## Table of Contents

1. [Linear — Array algorithms](#1-linear--array-algorithms)
2. [Linear — List algorithms](#2-linear--list-algorithms)
3. [Linear — Stack algorithms](#3-linear--stack-algorithms)
4. [Linear — Queue / Deque algorithms](#4-linear--queue--deque-algorithms)
5. [Linear — Ring buffer (lock-free SPSC)](#5-linear--ring-buffer-lock-free-spsc)
6. [Linear — Circular list / LRU](#6-linear--circular-list--lru)
7. [Trees — Red-black tree](#7-trees--red-black-tree)
8. [Trees — B-tree](#8-trees--b-tree)
9. [Trees — Trie / Radix tree](#9-trees--trie--radix-tree)
10. [Trees — Segment tree + lazy propagation](#10-trees--segment-tree--lazy-propagation)
11. [Trees — Interval tree](#11-trees--interval-tree)
12. [Trees — Heap / Priority queue](#12-trees--heap--priority-queue)
13. [Hash — Chaining hash table](#13-hash--chaining-hash-table)
14. [Hash — Open-addressing (linear probe)](#14-hash--open-addressing-linear-probe)
15. [Hash — Cuckoo hashing](#15-hash--cuckoo-hashing)
16. [Graph — BFS](#16-graph--bfs)
17. [Graph — DFS](#17-graph--dfs)
18. [Graph — Dijkstra's shortest path](#18-graph--dijkstras-shortest-path)
19. [Graph — Bellman-Ford](#19-graph--bellman-ford)
20. [Graph — Topological sort (Kahn's)](#20-graph--topological-sort-kahns)
21. [Graph — Topological sort (DFS)](#21-graph--topological-sort-dfs)
22. [Graph — Critical path](#22-graph--critical-path)
23. [Graph — Union-Find (DSU)](#23-graph--union-find-dsu)
24. [Probabilistic — Bloom filter](#24-probabilistic--bloom-filter)
25. [Probabilistic — Count-Min sketch](#25-probabilistic--count-min-sketch)
26. [Probabilistic — HyperLogLog](#26-probabilistic--hyperloglog)
27. [Probabilistic — Skip list](#27-probabilistic--skip-list)
28. [Spatial — k-d tree](#28-spatial--k-d-tree)
29. [Spatial — R-tree](#29-spatial--r-tree)
30. [Specialized — LSM tree compaction](#30-specialized--lsm-tree-compaction)
31. [Specialized — Bitmap operations](#31-specialized--bitmap-operations)
32. [Specialized — Wavelet tree](#32-specialized--wavelet-tree)
33. [Complexity quick-reference table](#33-complexity-quick-reference-table)
34. [Domain cross-reference](#34-domain-cross-reference)

---

## 1. Linear — Array algorithms

### Binary search

```
Precondition : array is sorted in ascending order
Input        : sorted array A[0..n-1], target value t
Output       : index i such that A[i] == t, or -1

lo = 0, hi = n-1
while lo <= hi:
    mid = lo + (hi - lo) / 2       ← avoids integer overflow vs (lo+hi)/2
    if A[mid] == t  → return mid
    if A[mid] <  t  → lo = mid + 1  (answer is in right half)
    else            → hi = mid - 1  (answer is in left half)
return -1
```

| | |
|---|---|
| Time | O(log n) |
| Space | O(1) |
| Invariant | The answer, if it exists, is always within `[lo, hi]` |
| Why `lo + (hi-lo)/2` | `(lo+hi)/2` overflows when both are near `INT_MAX` |

**Domain use**
- Linux page cache: find page by index in sorted XArray leaves
- Filesystem extent lookup: binary search sorted extent list
- Database index scan: B+ leaf node binary search
- Network FIB: sorted prefix table fallback lookup

---

### Two-pointer (pair sum)

```
Precondition : array sorted ascending
Input        : A[0..n-1], target sum S
Output       : indices (i,j) with A[i]+A[j]==S, or false

lo = 0, hi = n-1
while lo < hi:
    sum = A[lo] + A[hi]
    if sum == S → return (lo, hi)
    if sum <  S → lo++     (need larger value — move left pointer right)
    if sum >  S → hi--     (need smaller value — move right pointer left)
return false
```

| | |
|---|---|
| Time | O(n) |
| Space | O(1) |
| Why it works | Sorted order means moving `lo` right only increases the sum; moving `hi` left only decreases it. Every possible pair is implicitly considered. |

**Domain use**
- Network packet pair matching (source+dest port sum constraints)
- Memory allocator: find two free blocks that together satisfy a request
- Compiler register allocation: pair assignment constraints

---

### Sliding window maximum (monotonic deque)

```
Input  : array A[0..n-1], window size k
Output : array M[0..n-k] where M[i] = max(A[i..i+k-1])

deque D (stores indices, front=max)
for i = 0 to n-1:
    // expire: remove indices outside window [i-k+1, i]
    while D not empty and D.front < i-k+1:
        D.pop_front()

    // dominate: remove back indices whose values ≤ A[i]
    // (they can never be the window max while A[i] is present)
    while D not empty and A[D.back] <= A[i]:
        D.pop_back()

    D.push_back(i)

    if i >= k-1:
        M[i-k+1] = A[D.front]   // front always holds window max index
```

| | |
|---|---|
| Time | O(n) total — each index pushed and popped at most once |
| Space | O(k) for the deque |
| Invariant | Deque stores indices in **strictly decreasing** order of their values |
| Key insight | A smaller element to the left of a larger one can **never** be a future window max — evict it immediately |

**Domain use**
- `fq_codel` (Linux): minimum RTT tracking over time window
- GPU max-pooling layer: 1D/2D sliding max across feature map
- SQL `MAX() OVER (ROWS BETWEEN k PRECEDING AND CURRENT ROW)`
- Network congestion control: bandwidth peak over recent window

---

### Prefix sum (cumulative sum / scan)

```
Build: P[0] = 0
       P[i] = P[i-1] + A[i-1]    for i = 1..n
       O(n) time, O(n) space

Query: sum(A[l..r]) = P[r+1] - P[l]
       O(1) time
```

| | |
|---|---|
| Build | O(n) time, O(n) space |
| Query | O(1) time |
| Why it works | P[r+1] counts everything from 0 to r; subtracting P[l] removes 0 to l-1 |

**Domain use**
- GPU parallel prefix scan (CUDA `thrust::inclusive_scan`)
- Database column aggregation: SUM over arbitrary ranges
- Network traffic accounting: bytes in any time interval
- Image processing: summed area table (2D prefix sum) for box filters

---

## 2. Linear — List algorithms

### List reversal (three-pointer)

```
prev = NULL, cur = head
while cur:
    next = cur.next    // save next before overwriting
    cur.next = prev    // reverse the link
    prev = cur
    cur = next
head = prev
```

| | |
|---|---|
| Time | O(n) |
| Space | O(1) in-place |

---

### Merge sort on linked lists

```
function merge_sort(head):
    if head == NULL or head.next == NULL: return head
    mid = find_middle(head)          // slow/fast pointer
    second = mid.next
    mid.next = NULL                  // split
    left  = merge_sort(head)
    right = merge_sort(second)
    return merge(left, right)

function merge(a, b):                // O(n+m), O(1) extra space
    dummy head, tail = dummy
    while a and b:
        if a.val <= b.val: tail.next = a; a = a.next
        else:              tail.next = b; b = b.next
        tail = tail.next
    tail.next = a or b
    return dummy.next
```

| | |
|---|---|
| Time | O(n log n) |
| Space | O(log n) stack frames (no array copies unlike array merge sort) |
| Advantage over array merge sort | No auxiliary array needed — split/merge are O(1) pointer operations |

**Domain use**
- Linux kernel: sorting `sk_buff` chains, inode list compaction
- External sort for database table joins on linked record chains

---

### Floyd's cycle detection (tortoise and hare)

```
slow = head, fast = head
while fast and fast.next:
    slow = slow.next
    fast = fast.next.next
    if slow == fast: return CYCLE_FOUND
return NO_CYCLE
```

| | |
|---|---|
| Time | O(n) |
| Space | O(1) |
| Why it works | If a cycle of length L exists, fast gains 1 step on slow per iteration. They meet after at most L iterations inside the cycle. |

**Domain use**
- Linux kernel: detecting corrupted circular list (`list_head` integrity)
- OS resource leak detection: reference count cycle detection
- Compiler: detecting infinite loops in CFG analysis

---

## 3. Linear — Stack algorithms

### Iterative DFS (explicit stack)

```
push src onto stack
while stack not empty:
    v = stack.pop()
    if visited[v]: continue
    mark visited[v]
    process(v)
    for each neighbour u of v:
        if not visited[u]: stack.push(u)
```

| | |
|---|---|
| Time | O(V+E) |
| Space | O(V) — avoids O(V) recursion stack frames |

**Why avoid recursive DFS?** Deep graphs (Linux module dependency trees can reach thousands of levels) overflow the kernel stack. Iterative DFS is mandatory in OS code.

---

### Postfix expression evaluation

```
for each token t in expression:
    if t is a number: push(t)
    if t is an operator:
        b = pop(), a = pop()
        push(a op b)
return pop()
```

| | |
|---|---|
| Time | O(n) tokens |
| Space | O(n) |

**Domain use**
- Linux eBPF bytecode execution (stack-based VM)
- Compiler expression trees (postfix = reverse Polish notation)
- Calculator engines, spreadsheet formula evaluation

---

### Balanced bracket checking

```
for each char c:
    if c is open  bracket: push(c)
    if c is close bracket:
        if stack empty: return false
        top = pop()
        if top doesn't match c: return false
return stack.is_empty()
```

| | |
|---|---|
| Time | O(n) |
| Space | O(n) |

---

## 4. Linear — Queue / Deque algorithms

### BFS (Breadth-First Search)

```
enqueue(src); visited[src] = true
while queue not empty:
    v = dequeue()
    process(v)
    for each neighbour u of v:
        if not visited[u]:
            visited[u] = true
            enqueue(u)
```

| | |
|---|---|
| Time | O(V+E) |
| Space | O(V) |
| Key property | Visits all vertices at distance d before any at distance d+1 → guarantees shortest path in unweighted graphs |

**Domain use**
- Network: shortest hop count routing, broadcast flood fill
- OS: module dependency level-order loading
- GPU: level-by-level task dispatch
- Social networks: degree-of-separation queries

---

## 5. Linear — Ring buffer (lock-free SPSC)

### Lock-free single-producer single-consumer ring

```
produce(val):
    tail = load(tail, relaxed)
    head = load(head, acquire)        // see consumer's progress
    if tail - head >= capacity: return FULL
    buf[tail & (cap-1)] = val
    store(tail+1, tail, release)      // publish: consumer sees val before new tail

consume(out):
    head = load(head, relaxed)
    tail = load(tail, acquire)        // see producer's writes
    if head == tail: return EMPTY
    *out = buf[head & (cap-1)]
    store(head+1, head, release)      // publish: producer sees slot is free
```

| | |
|---|---|
| Time | O(1) produce and consume |
| Space | O(capacity) |
| Memory orders | `acquire` on the load that observes the other side; `release` on the store that publishes progress |
| Why power-of-2 capacity | `index & (cap-1)` replaces `index % cap` — one instruction vs divide |

**Why memory ordering matters on ARM**  
Without `release` on the tail store, the ARM CPU may reorder the data write (`buf[tail] = val`) to happen *after* the tail advance. The consumer would then see the new tail but read stale/garbage data from the slot. The `release` fence guarantees all prior writes are visible before the tail is published.

**Domain use**
- Linux eBPF ring buffer (`BPF_MAP_TYPE_RINGBUF`)
- NIC RX/TX descriptor rings (DPDK `rte_ring`)
- Write-ahead log (WAL) journal ring in databases
- Audio driver circular buffer (ALSA)

---

## 6. Linear — Circular list / LRU

### LRU eviction (clock-hand pattern)

```
access(val, max_size):
    if val in list:
        move_to_front(val)    // O(1) — MRU promotion
        return NO_EVICTION
    push_front(val)           // O(1) — insert as MRU
    if size > max_size:
        evicted = pop_back()  // O(1) — remove LRU (back of list)
        return evicted
```

| | |
|---|---|
| Promote (hit) | O(1) |
| Insert + evict (miss) | O(1) |
| Lookup | O(n) without hash table; O(1) with combined hash+list |

**Production LRU = hash table + doubly-linked list**  
The hash table gives O(1) lookup; the list gives O(1) promote/evict.  
Linux `dentry` cache, `inode` cache, and database buffer pools all use this exact combination.

**Domain use**
- Linux page cache LRU (Multi-Generational LRU in kernel 5.17+)
- CPU L1/L2 cache replacement policy simulation
- DNS resolver cache, CDN edge cache
- Database buffer pool (InnoDB buffer pool uses LRU with a "young" and "old" sublist)

---

## 7. Trees — Red-black tree

### Properties (must hold after every operation)

```
1. Every node is RED or BLACK
2. Root is BLACK
3. Every NIL sentinel is BLACK
4. RED node → both children are BLACK (no two consecutive reds)
5. All simple paths from any node to descendant NIL leaves
   have the same number of BLACK nodes (black-height)
```

These five properties guarantee: **height ≤ 2·log₂(n+1)**

### Rotations

```
Left rotation on x:            Right rotation on y:
      x                  y           y                x
     / \                / \         / \              / \
    A   y    →         x   C       x   C    →       A   y
       / \            / \         / \                  / \
      B   C          A   B       A   B                B   C
```

Rotations preserve BST order and take O(1) time.

### Insert fixup — 3 cases (and their mirrors)

| Case | Condition | Action |
|---|---|---|
| 1 | Uncle is RED | Recolour parent+uncle BLACK, grandparent RED; move up |
| 2 | Uncle is BLACK, node is right child | Rotate left on parent → become case 3 |
| 3 | Uncle is BLACK, node is left child | Rotate right on grandparent, swap colours |

### Delete fixup — 4 cases

| Case | Condition | Action |
|---|---|---|
| 1 | Sibling is RED | Rotate left on parent, recolour → reduce to case 2/3/4 |
| 2 | Sibling BLACK, both nephews BLACK | Recolour sibling RED, move problem up |
| 3 | Sibling BLACK, far nephew BLACK, near nephew RED | Rotate right on sibling → case 4 |
| 4 | Sibling BLACK, far nephew RED | Rotate left on parent, recolour |

| Operation | Time |
|---|---|
| Insert | O(log n) |
| Delete | O(log n) |
| Search | O(log n) |
| Min (with leftmost cache) | **O(1)** |

**CFS leftmost cache pattern**  
The Linux CFS scheduler caches the leftmost (minimum vruntime) node pointer. On every insert/delete, the pointer is updated. `pick_next_task()` reads this cached pointer in O(1) — critical for scheduling latency at 1000+ Hz tick rate.

**Domain use**
- Linux CFS run queue (per-CPU RB-tree keyed by vruntime)
- Linux VMA management (pre-kernel 6.1, now Maple tree)
- `std::map` / `std::set` in C++ STL (libstdc++)
- Java `TreeMap`, `TreeSet`
- Nginx timer wheel, InnoDB lock manager

---

## 8. Trees — B-tree

### Key invariants (degree T)

```
- Root: 1 .. 2T-1 keys
- Internal nodes: T-1 .. 2T-1 keys
- All leaves at same depth
- n keys → n+1 child pointers
- Keys in each node are sorted
- Subtree[i] contains keys strictly between key[i-1] and key[i]
```

### Why B-trees are disk-friendly

Each node = one disk page (typically 4KB–16KB). With T=100, a tree of
height 3 can hold 100³ = 1 million keys with only 3 disk reads.
A binary BST of 1M nodes requires ~20 disk reads (one per level).

### Split (insert path)

```
When inserting into a full node (2T-1 keys):
1. Allocate new sibling node
2. Copy right half (T-1 keys) to sibling
3. Promote median key (position T-1) to parent
4. Parent gains one key and one child pointer

Proactive split (top-down): split every full node encountered
on the way down → guarantees space to insert at leaf without
a second upward pass.
```

| Operation | Time |
|---|---|
| Search | O(T · log_T n) = O(log n) |
| Insert | O(T · log_T n) |
| Delete | O(T · log_T n) |
| Range scan | O(log_T n + k) where k = results |

**Domain use**
- Linux btrfs, ext4 (HTree), XFS extent B-tree
- macOS APFS
- InnoDB (MySQL), PostgreSQL, SQLite B-tree index
- NTFS Master File Table

---

## 9. Trees — Trie / Radix tree

### Standard trie insert/search

```
insert(key, val):
    cur = root
    for each byte c in key:
        if cur.children[c] == NULL:
            cur.children[c] = new_node()
        cur = cur.children[c]
    cur.is_end = true; cur.val = val

search(key) → bool:
    cur = root
    for each byte c in key:
        if cur.children[c] == NULL: return false
        cur = cur.children[c]
    return cur.is_end
```

| Operation | Time |
|---|---|
| Insert | O(L) where L = key length |
| Search | O(L) |
| Longest prefix match | O(L) |
| Prefix enumeration | O(P + results) |

### Longest prefix match (IP routing)

```
cur = root; best_depth = 0; depth = 0
for each byte c in key:
    if cur.children[c] == NULL: break
    cur = cur.children[c]
    depth++
    if cur.is_end: best_depth = depth   // record last terminal
return best_depth
```

**Why tries beat hash tables for routing**  
Hash tables require an exact key match. IP routing needs the *longest matching prefix* (a packet for 192.168.1.55 should match the /24 route 192.168.1.0 before the /16 route 192.168.0.0). Tries traverse bit by bit and naturally find the longest match.

**Domain use**
- Linux kernel FIB trie (LC-trie with path compression)
- Linux XArray (radix trie for page cache indexing)
- DPDK longest-prefix-match table
- URL routing in web frameworks (HTTP path → handler)
- BPE tokenizer vocabulary (NLP/ML)
- Autocomplete / spell-check

---

## 10. Trees — Segment tree + lazy propagation

### Build

```
build(node, lo, hi):
    if lo == hi: tree[node] = arr[lo]; return
    mid = (lo+hi)/2
    build(2*node, lo, mid)
    build(2*node+1, mid+1, hi)
    tree[node] = tree[2*node] + tree[2*node+1]   // merge: sum/min/max
```

### Lazy propagation (range update)

```
The key idea: defer work.
When updating range [l,r], instead of visiting every element,
mark the covering node with a "pending delta" in lazy[].
When a node is later queried or split, push the delta down
to its children first (push_down).

push_down(node, lo, hi):
    if lazy[node] == 0: return
    mid = (lo+hi)/2
    tree[2*node]   += lazy[node] * (mid-lo+1)  // child subtree size
    tree[2*node+1] += lazy[node] * (hi-mid)
    lazy[2*node]   += lazy[node]
    lazy[2*node+1] += lazy[node]
    lazy[node] = 0
```

| Operation | Time |
|---|---|
| Build | O(n) |
| Point update | O(log n) |
| Range update (lazy) | O(log n) |
| Range query | O(log n) |

**Domain use**
- Virtual memory: update permissions across a contiguous VMA range
- Time-series databases: windowed sum/max over arbitrary intervals
- Game engines: range damage/heal to units in a spatial segment
- Compiler: liveness range analysis over instruction ranges

---

## 11. Trees — Interval tree

### Augmented BST construction

```
Each node stores interval [lo, hi] and augmented max_hi:
    max_hi = max(hi, left.max_hi, right.max_hi)

Stabbing query (find any interval containing point p):
    node = root
    while node:
        if p in [node.lo, node.hi]: return node   // hit
        if node.left and node.left.max_hi >= p:
            node = node.left   // left subtree might contain p
        else:
            node = node.right  // prune left — no interval there reaches p
```

| Operation | Time |
|---|---|
| Insert | O(log n) |
| Stabbing query | O(log n) |
| Overlap query (all k intervals) | O(k log n) |

**Why max_hi enables pruning**  
If `left.max_hi < p`, no interval in the left subtree can contain p (all their right endpoints are less than p). This is the augmentation that makes the O(log n) guarantee possible.

**Domain use**
- Linux mm: `find_vma()` — find the VMA containing a given address
- Firewall: find all rules whose port range contains a packet's port
- Database MVCC: find all transaction versions valid at a given timestamp
- CUDA: detect overlapping stream execution windows

---

## 12. Trees — Heap / Priority queue

### Sift-up and sift-down

```
sift_up(i):                     sift_down(i):
    while i > 0:                    while true:
        p = (i-1)/2                     best = i
        if key[p] > key[i]:             l = 2i+1, r = 2i+2
            swap(p, i); i = p           if l<n and key[l]<key[best]: best=l
        else: break                     if r<n and key[r]<key[best]: best=r
                                        if best==i: break
                                        swap(i, best); i = best
```

| Operation | Time |
|---|---|
| Push (insert) | O(log n) |
| Pop min/max | O(log n) |
| Peek | O(1) |
| Build from array (heapify) | O(n) — not O(n log n) |
| Heapsort | O(n log n) in-place |

**Why heapify is O(n) not O(n log n)**  
Building by inserting one at a time is O(n log n). Building bottom-up
(sift down from n/2 to 0) is O(n) because most nodes are near the leaves
where sift-down work is O(1). The sum telescopes to O(n).

---

## 13. Hash — Chaining hash table

### Operations

```
hash(key) → bucket index (0..capacity-1)

insert(key, val):
    idx = hash(key) % capacity
    walk chain at bucket[idx] — update if found
    else prepend new node → O(1) prepend

lookup(key):
    idx = hash(key) % capacity
    walk chain at bucket[idx] — O(1) average, O(n) worst

resize (when load factor > threshold):
    new_capacity = 2 * capacity
    for each entry: rehash into new_buckets
    O(n) amortised → O(1) per insert amortised
```

| | Average | Worst |
|---|---|---|
| Insert | O(1) amortised | O(n) |
| Lookup | O(1) | O(n) |
| Delete | O(1) | O(n) |

**Load factor and resize threshold**  
At load factor 0.75 (75% full), chains average ~1 node — O(1) lookup.
Above ~1.0, performance degrades toward O(n). Linux kernel hash tables
typically target 0.5–1.0 and use fixed-size power-of-2 tables.

**Domain use**
- Linux `pid_hashfn`: PID → task_struct
- Linux dcache: filename → dentry (directory entry)
- Linux netfilter conntrack: (src_ip, dst_ip, sport, dport, proto) → connection
- Database hash join: build phase hashes smaller relation

---

## 14. Hash — Open-addressing (linear probe)

```
insert(key, val):
    idx = hash(key) % capacity
    while slot[idx] is occupied and slot[idx].key != key:
        idx = (idx + 1) % capacity    // linear probe
    slot[idx] = {key, val}

lookup(key):
    idx = hash(key) % capacity
    while slot[idx] is occupied:
        if slot[idx].key == key: return slot[idx].val
        if slot[idx] is DELETED: pass  // tombstone — keep probing
        idx = (idx + 1) % capacity
    return NOT_FOUND
```

**Tombstone (lazy deletion)**  
Setting deleted slots to a special DELETED marker (rather than empty)
prevents false negatives: a live key past the deleted slot would be
unreachable if we stopped at the first empty.

| | Average (α < 0.7) | Worst |
|---|---|---|
| Insert | O(1) | O(n) |
| Lookup | O(1) | O(n) |
| Delete | O(1) with tombstone | — |

**Cache advantage over chaining**  
All slots are contiguous in memory → no pointer chasing → better cache
utilisation. CPython dict, Rust `HashMap`, Java 8+ `HashMap` all use
open addressing variants.

---

## 15. Hash — Cuckoo hashing

### Two-table displacement

```
Two hash tables T1 (hash h1) and T2 (hash h2), capacity N each.

insert(key, val):
    for kick in 0..MAX_KICKS:
        if T1[h1(key)] is empty: T1[h1(key)] = {key,val}; return OK
        swap {key,val} with T1[h1(key)]     // displace occupant
        if T2[h2(key)] is empty: T2[h2(key)] = {key,val}; return OK
        swap {key,val} with T2[h2(key)]     // displace occupant
    return REHASH_NEEDED                     // cycle detected

lookup(key):                                 // O(1) WORST CASE
    if T1[h1(key)].key == key: return T1[h1(key)].val
    if T2[h2(key)].key == key: return T2[h2(key)].val
    return NOT_FOUND
```

| | Time |
|---|---|
| Lookup | **O(1) worst case** — checks exactly 2 locations |
| Insert | O(1) amortised expected |
| Delete | O(1) |

**Why O(1) worst-case lookup matters**  
In the Linux fast path (packet forwarding, eBPF maps), predictable latency
beats average-case performance. Cuckoo hash gives hard upper bound of 2
memory accesses per lookup regardless of load factor.

**Domain use**
- Linux eBPF hash maps (kernel 4.6+)
- DPDK flow classification tables
- GPU hash tables for parallel insertion
- High-performance firewall IP/MAC lookup

---

## 16. Graph — BFS

See §4 Queue / Deque algorithms above. BFS guarantees shortest path in
terms of **edge count** (unweighted graphs).

---

## 17. Graph — DFS

```
dfs(v, visited):
    visited[v] = true
    process(v)
    for each neighbour u of v:
        if not visited[u]:
            dfs(u, visited)
```

| | |
|---|---|
| Time | O(V+E) |
| Space | O(V) recursion stack |
| Use iterative for deep graphs | Avoids stack overflow |

**Applications of DFS**
- Topological sort (DFS-based)
- Strongly connected components (Tarjan's, Kosaraju's)
- Cycle detection in directed graphs
- Maze solving
- Compiler: dominator tree computation

---

## 18. Graph — Dijkstra's shortest path

```
dist[src] = 0; dist[all others] = ∞
priority_queue.push((0, src))

while pq not empty:
    (d, u) = pq.pop_min()
    if visited[u]: continue           // lazy deletion of stale entries
    visited[u] = true
    for each edge (u→v, weight w):
        if dist[u] + w < dist[v]:
            dist[v] = dist[u] + w
            pq.push((dist[v], v))     // may push duplicates — handled by visited[]
```

| | |
|---|---|
| Time | O((V+E) log V) with binary heap |
| Time | O(V log V + E) with Fibonacci heap |
| Space | O(V) |
| Precondition | **No negative edge weights** |
| Greedy invariant | When u is popped, `dist[u]` is the true shortest distance |

**Why negative weights break Dijkstra**  
Greedy selection assumes: once a vertex is finalized, no later edge can
improve its distance. A negative edge from a later vertex could invalidate
this. Use Bellman-Ford for negative weights.

**Domain use**
- OSPF link-state routing (IP networks)
- GPS navigation (shortest travel time)
- Network latency minimisation
- Game AI pathfinding (A* is Dijkstra + heuristic)

---

## 19. Graph — Bellman-Ford

```
dist[src] = 0; dist[all others] = ∞

repeat V-1 times:
    for each edge (u→v, weight w):
        if dist[u] + w < dist[v]:
            dist[v] = dist[u] + w

// V-th pass: detect negative cycles
for each edge (u→v, weight w):
    if dist[u] + w < dist[v]:
        NEGATIVE CYCLE DETECTED
```

| | |
|---|---|
| Time | O(VE) |
| Space | O(V) |
| Handles negative weights | Yes |
| Detects negative cycles | Yes |

**Why V-1 iterations suffice**  
Any shortest path in a graph with no negative cycles has at most V-1 edges
(otherwise it contains a cycle, which can be removed without increasing cost
for non-negative cycles). After k iterations, all shortest paths of ≤ k edges
are correctly computed.

**Domain use**
- BGP (Border Gateway Protocol): policy weights can be negative
- Currency arbitrage detection (negative cycle = profit opportunity)
- Network distance-vector routing protocols

---

## 20. Graph — Topological sort (Kahn's algorithm)

```
compute in-degree of every vertex
enqueue all vertices with in-degree 0

while queue not empty:
    v = dequeue()
    append v to topological order
    for each edge v→u:
        in-degree[u]--
        if in-degree[u] == 0: enqueue(u)

if |order| < V: CYCLE EXISTS (not a DAG)
```

| | |
|---|---|
| Time | O(V+E) |
| Space | O(V) |
| Detects cycles | Yes — if output size < V |

**Intuition**  
A vertex with in-degree 0 has no prerequisites — it can be scheduled first.
Removing it unblocks its successors, potentially creating new zero-in-degree
vertices. This mirrors how a build system (make, Bazel) schedules tasks.

**Domain use**
- Linux kernel module load order
- CI/CD pipeline stage scheduling
- CUDA kernel launch dependency ordering
- Compiler instruction scheduling
- Makefile/Bazel/Gradle build targets

---

## 21. Graph — Topological sort (DFS-based)

```
dfs_topo(v, visited, stack):
    visited[v] = true
    for each neighbour u:
        if not visited[u]: dfs_topo(u, visited, stack)
    stack.push(v)             // push AFTER all successors are processed

for each unvisited v: dfs_topo(v, ...)
result = reverse(stack)
```

| | |
|---|---|
| Time | O(V+E) |
| Key insight | A vertex is pushed only after all its dependencies — so reversing gives correct order |

---

## 22. Graph — Critical path

```
Process vertices in topological order.
For each vertex v in topo order:
    for each edge v→u with weight w:
        dist[u] = max(dist[u], dist[v] + w)

Critical path length = max(dist[all vertices])
```

| | |
|---|---|
| Time | O(V+E) |
| Use case | Project scheduling (CPM — Critical Path Method) |

**Domain use**
- CUDA kernel launch graph: find the minimum completion time
- Compiler: find the minimum number of sequential instructions (ILP)
- Project management: identify tasks that cannot be delayed

---

## 23. Graph — Union-Find (DSU)

### Path compression

```
find(x):
    if parent[x] != x:
        parent[x] = find(parent[x])   // path compression: point directly to root
    return parent[x]
```

### Union by rank

```
union(x, y):
    rx = find(x), ry = find(y)
    if rx == ry: return false           // already in same set
    if rank[rx] < rank[ry]: swap(rx,ry)
    parent[ry] = rx                     // attach smaller-rank tree under larger
    if rank[rx] == rank[ry]: rank[rx]++
    components--
    return true
```

| Operation | Time |
|---|---|
| find | O(α(n)) amortised — inverse Ackermann, effectively O(1) |
| union | O(α(n)) amortised |

**Why both techniques together give O(α(n))**  
Path compression flattens trees toward single-level. Union by rank prevents
tall trees from forming in the first place. Together they bound the amortised
cost to the inverse Ackermann function — practically constant for all n < 10^80.

**Domain use**
- Linux: namespace hierarchy merging, cgroup tree unification
- Network: detecting when a partition becomes connected
- Kruskal's MST algorithm (union-find as backbone)
- `fsck`: connected component labelling of filesystem blocks
- CUDA: parallel connected-components computation
- Cluster merging in ML (hierarchical agglomerative clustering)

---

## 24. Probabilistic — Bloom filter

### Optimal parameter selection

```
Given: n expected items, target false-positive rate p
    m (bits)    = -n · ln(p) / (ln 2)²
    k (hashes)  = (m/n) · ln 2

Double hashing: h_k(x) = h1(x) + k · h2(x)   mod m
    → k hash functions from 2 hash evaluations
```

### Operations

```
add(x):
    for i in 0..k-1:
        bit_set(bits, hash(x, i) % m)

test(x) → bool:
    for i in 0..k-1:
        if not bit_set(bits, hash(x, i) % m): return DEFINITELY_NOT_IN_SET
    return PROBABLY_IN_SET   // false positive possible
```

| | |
|---|---|
| Space | O(m) bits — typically 10 bits/element for 1% FP rate |
| Add | O(k) — typically k ≈ 7 |
| Test | O(k) |
| False negatives | **Impossible** — a set element is always found |
| False positives | P(FP) = (1 - e^{-kn/m})^k |

**When false positives are acceptable**  
A false positive means "maybe in set" — the caller does an expensive
lookup to confirm. The bloom filter eliminates most expensive lookups
(true negatives), which is the common case. A false positive just causes
one unnecessary expensive lookup.

**Domain use**
- RocksDB/LevelDB/Cassandra: one bloom filter per SSTable level. Before an expensive disk read, test the bloom filter. ~99% of "not found" lookups skip the disk entirely.
- eBPF: fast pre-filter for connection tracking — avoid hash table lookup for unknown IPs
- CDN: test if a URL is in the local cache before querying origin
- Chrome safe browsing: malware URL pre-screening

---

## 25. Probabilistic — Count-Min sketch

### Construction

```
d rows (one per hash function), w columns
    w = ceil(e / ε)       ε = error bound
    d = ceil(ln(1/δ))     δ = failure probability

add(x, count=1):
    for i in 0..d-1:
        table[i][hash_i(x) % w] += count

query(x) → estimated count:
    return min over i of table[i][hash_i(x) % w]
```

| | |
|---|---|
| Space | O(w · d) = O(1/ε · ln(1/δ)) |
| Add | O(d) |
| Query | O(d) — returns value ≥ true count (never underestimates) |
| Error guarantee | With prob ≥ 1-δ: estimate ≤ true_count + ε · total_items |

**Why min across rows?**  
Each row gives an independent (possibly inflated) estimate due to hash
collisions. The minimum is the least inflated — closest to the true count.

**Domain use**
- Network monitoring: top-K flows by byte count (heavy hitters)
- DDoS detection: identify IPs sending disproportionate traffic
- Linux eBPF: per-flow packet/byte counters in limited map space
- Database query optimizer: cardinality estimation for join planning
- ClickHouse/Druid: approximate frequency aggregation

---

## 26. Probabilistic — HyperLogLog

### Algorithm

```
For each element x:
    h = hash(x)                        // 64-bit hash
    reg = h >> (64-b)                  // top b bits → register index (0..m-1)
    w   = h << b                       // remaining 64-b bits
    rank = position_of_leftmost_1(w)   // = clz(w) + 1
    register[reg] = max(register[reg], rank)

estimate():
    Z = 1 / Σ 2^{-register[i]}        // harmonic mean of 2^register values
    E = α · m² / Z

    if E <= 2.5m and some registers == 0:
        E = m · ln(m / empty_register_count)   // small-range correction

    return E
```

| | |
|---|---|
| Space | O(m) bytes where m = 2^b registers |
| Add | O(1) |
| Estimate | O(m) — one pass over registers |
| Error | ≈ 1.04/√m → b=12 (4096 regs, 4KB) gives ~1.6% error |

**Why the harmonic mean works**  
The maximum leading zeros in a hash stream of n distinct values is
approximately log₂(n). Using m independent registers (each tracking one
bit-prefix of the hash space) and combining with harmonic mean cancels
most variance, giving a surprisingly accurate estimate with tiny space.

**Domain use**
- PostgreSQL `HLL` extension: `COUNT(DISTINCT column)` approximation
- Redis `PFCOUNT` command
- ClickHouse `uniqHLL12()` aggregation
- eBPF programs: unique IP or flow count per second
- Kafka/Flink: streaming cardinality monitoring

---

## 27. Probabilistic — Skip list

### Randomised level generation

```
new_node_level():
    level = 1
    while random() < p and level < MAX_LEVEL:
        level++
    return level     // geometric distribution with mean 1/(1-p)
```

### Insert

```
find predecessors update[0..level-1] (nodes just before insertion point at each level)
generate new_level for node
for i in 0..new_level-1:
    node.forward[i] = update[i].forward[i]
    update[i].forward[i] = node
```

| | Expected | Worst |
|---|---|---|
| Search | O(log n) | O(n) |
| Insert | O(log n) | O(n) |
| Delete | O(log n) | O(n) |
| Range scan | O(log n + k) | — |
| Space | O(n log n) expected | — |

**Skip list vs RB-tree**  
Skip lists are often simpler to implement correctly (especially lock-free variants).
RB-trees have better worst-case guarantees (O(log n) hard). Redis chose skip list
for sorted sets because it's easier to implement lock-free operations and the
range scan is cache-friendlier.

**Domain use**
- Redis sorted sets (`ZADD`, `ZRANGEBYSCORE`) — uses skip list internally
- RocksDB/LevelDB memtable — in-memory sorted buffer before SSTable flush
- CockroachDB MVCC layer — ordered version store

---

## 28. Spatial — k-d tree

### Build (median split)

```
build(points, depth):
    if points empty: return NULL
    axis = depth % k                          // cycle through dimensions
    sort points by axis coordinate
    mid = len(points) / 2
    node.point = points[mid]
    node.left  = build(points[:mid],   depth+1)
    node.right = build(points[mid+1:], depth+1)
    return node
```

### Nearest-neighbour search

```
nn(node, query, depth, best, best_dist²):
    if node == NULL: return
    d² = dist²(query, node.point)
    if d² < best_dist²: best = node; best_dist² = d²

    axis = depth % k
    diff = query[axis] - node.point[axis]
    near = diff < 0 ? left : right
    far  = diff < 0 ? right : left

    nn(near, query, depth+1, best, best_dist²)   // always explore near

    // only explore far branch if splitting hyperplane intersects
    // the best-distance hypersphere: diff² < best_dist²
    if diff² < best_dist²:
        nn(far, query, depth+1, best, best_dist²)
```

| | Average | Worst |
|---|---|---|
| Build | O(n log n) | O(n log n) |
| NN search | O(log n) | O(n) |
| Range search | O(√n + k) | O(n) |

**Domain use**
- GPU BVH (bounding volume hierarchy) for ray tracing — k-d tree variant
- PostGIS point-in-polygon and nearest-POI queries
- CUDA point cloud processing (LiDAR, 3D scene reconstruction)
- Machine learning k-NN classifier inference
- NUMA topology: find nearest NUMA node for a given CPU

---

## 29. Spatial — R-tree

### Bounding box overlap test

```
overlap(A, B):
    return A.x1 <= B.x2 and A.x2 >= B.x1
       and A.y1 <= B.y2 and A.y2 >= B.y1
```

### Choose subtree (insert path)

```
For each child, compute area enlargement needed to include new rect.
Choose child with minimum enlargement.
Break ties by minimum area (minimises dead space).
```

### Search

```
search(node, query_rect):
    if not overlap(node.mbr, query_rect): return   // prune entire subtree
    if node is leaf:
        for each entry: if overlap(entry.rect, query_rect): report entry
    else:
        for each child: search(child, query_rect)
```

| | Average |
|---|---|
| Insert | O(log n) |
| Search | O(log n + k) |
| Space | O(n) |

**Domain use**
- PostGIS `ST_Intersects`, `ST_Within`, `ST_Contains`
- MongoDB 2dsphere index for geo queries
- GPU BVH ray-box intersection test (R-tree variant)
- Geo-fencing: does vehicle position fall within any zone?
- Network zone overlap detection (CIDR block containment)

---

## 30. Specialized — LSM tree compaction

### Write path

```
write(key, val):
    append to in-memory MemTable (skip list or RB-tree)
    write to WAL (write-ahead log) for crash recovery
    if MemTable full:
        flush MemTable to L0 SSTable (sorted, immutable file)
```

### Read path

```
read(key):
    check MemTable first (newest)         O(log n)
    check L0 SSTables (may overlap)       O(L0_count · log n)
    check L1 .. Ln (non-overlapping)      O(log n) per level with bloom filter
```

### Leveled compaction

```
When L(i) exceeds size limit:
    pick an SSTable from L(i)
    find all overlapping SSTables in L(i+1)
    merge-sort all selected SSTables
    write merged result to L(i+1)
    delete merged inputs
    remove tombstones for keys not present in lower levels
```

| | |
|---|---|
| Write | O(1) amortised — sequential append |
| Read | O(log² n) worst case across all levels |
| Space amplification | ~10× (leveled), ~30× (tiered) |
| Write amplification | ~30× (leveled) — each byte written ~30× across compactions |

**Read vs write amplification trade-off**  
RocksDB leveled compaction: low read amp (good for reads), high write amp.  
Tiered compaction: low write amp, higher read amp. Most OLAP systems prefer leveled.

**Domain use**
- RocksDB (used by Meta/Facebook, MyRocks, TiKV)
- Apache Cassandra (SSTable engine)
- LevelDB (Chrome IndexedDB, many key-value stores)
- ScyllaDB, CockroachDB storage engine

---

## 31. Specialized — Bitmap operations

### Core operations

```
set(i):    words[i/64] |=  (1ULL << (i%64))
clear(i):  words[i/64] &= ~(1ULL << (i%64))
test(i):   return words[i/64] & (1ULL << (i%64))

popcount:  Σ __builtin_popcountll(words[i])     // POPCNT instruction
find_first_set: first non-zero word → __builtin_ctzll(word)  // CTZ instruction
```

### SIMD-style bulk operations

```
AND: for each word: dst[i] = a[i] & b[i]   // can SIMD vectorise
OR:  for each word: dst[i] = a[i] | b[i]
NOT: for each word: dst[i] = ~a[i]
```

| Operation | Time |
|---|---|
| Set/clear/test | O(1) |
| find_first_set | O(1) with CTZ instruction |
| popcount (all n bits) | O(n/64) |
| AND/OR/NOT (n bits) | O(n/64) |

**RT scheduler O(1) find_first_set**  
Linux RT scheduler maintains a bitmap of 100 priority levels. `find_first_bit()`
uses `__ffs()` which compiles to a single `BSF` (bit scan forward) instruction —
O(1) to find the highest-priority runnable task.

**Domain use**
- Linux CPU masks (`cpumask_t`), IRQ masks
- RT scheduler priority bitmap
- ext4 block/inode allocation bitmap
- GPU CUDA warp lane masks (`__ballot_sync`, `__activemask`)
- Roaring Bitmap (hybrid bitmap + array for compressed sets in Druid/ClickHouse)

---

## 32. Specialized — Wavelet tree

### Build

```
build(seq, lo, hi):
    if lo == hi: return leaf
    mid = (lo+hi)/2
    bitmap[i] = 1 if seq[i] > mid else 0   // 1 = goes right
    left_seq  = [x for x in seq if x <= mid]
    right_seq = [x for x in seq if x >  mid]
    left  = build(left_seq,  lo,    mid)
    right = build(right_seq, mid+1, hi)
```

### rank(c, i) — count of c in seq[0..i-1]

```
At each node from root to leaf:
    ones  = popcount(bitmap[0..i-1])      // elements that went right
    zeros = i - ones                       // elements that went left
    if c <= mid: i = zeros;  go left
    else:        i = ones;   go right
return i
```

### quantile(l, r, k) — k-th smallest in seq[l..r]

```
At each node:
    zeros = count of 0-bits in bitmap[l..r]   // went left
    if k <= zeros: recurse left with mapped [l,r]
    else:          k -= zeros; recurse right with mapped [l,r]
```

| Operation | Time |
|---|---|
| Build | O(n log σ) where σ = alphabet size |
| rank(c, i) | O(log σ) |
| select(c, k) | O(log σ) |
| quantile(l, r, k) | O(log σ) |
| Space | O(n log σ) bits |

**Domain use**
- DuckDB succinct column store: range percentile queries
- FM-index for DNA sequence alignment (bioinformatics)
- Compressed inverted index for full-text search
- Entropy-based anomaly detection: frequency distribution over window

---

## 33. Complexity quick-reference table

| Structure | Search | Insert | Delete | Space | Notes |
|---|---|---|---|---|---|
| Array (unsorted) | O(n) | O(1) amort | O(n) | O(n) | |
| Array (sorted) | O(log n) | O(n) | O(n) | O(n) | Binary search |
| Linked list | O(n) | O(1) head | O(n) | O(n) | |
| Stack | O(n) | O(1) | O(1) | O(n) | |
| Queue | O(n) | O(1) | O(1) | O(n) | |
| Ring buffer | O(n) | O(1) | O(1) | O(cap) | Lock-free SPSC |
| Red-black tree | O(log n) | O(log n) | O(log n) | O(n) | Min O(1) w/ cache |
| B-tree (deg T) | O(log_T n) | O(log_T n) | O(log_T n) | O(n) | Disk-friendly |
| Trie | O(L) | O(L) | O(L) | O(n·L) | L=key length |
| Segment tree | O(log n) | O(log n) | O(log n) | O(n) | Range query O(log n) |
| Interval tree | O(log n) | O(log n) | O(log n) | O(n) | Stabbing query |
| Min-heap | O(n) | O(log n) | O(log n) | O(n) | Min peek O(1) |
| Hash (chaining) | O(1) avg | O(1) avg | O(1) avg | O(n) | O(n) worst |
| Hash (open addr) | O(1) avg | O(1) avg | O(1) avg | O(n) | Load < 0.7 |
| Cuckoo hash | O(1) worst | O(1) amort | O(1) | O(n) | 2 lookups max |
| Skip list | O(log n) exp | O(log n) exp | O(log n) exp | O(n log n) | |
| Union-Find | — | — | — | O(n) | union/find O(α) |
| BFS / DFS | O(V+E) | — | — | O(V) | |
| Dijkstra | O((V+E) log V) | — | — | O(V) | No neg weights |
| Bellman-Ford | O(VE) | — | — | O(V) | Handles neg weights |
| Topo sort | O(V+E) | — | — | O(V) | DAG only |
| Bloom filter | O(k) | O(k) | — | O(m) bits | FP possible, no FN |
| Count-Min sketch | O(d) | O(d) | — | O(w·d) | Overestimates only |
| HyperLogLog | O(1) | O(1) | — | O(m) bytes | ~1.6% error at b=12 |
| k-d tree | O(log n) avg | O(log n) | O(log n) | O(n) | NN search |
| R-tree | O(log n) avg | O(log n) | O(log n) | O(n) | BBox search |
| LSM tree | O(log² n) | O(1) amort | O(1) amort | O(n) | Write-optimised |
| Bitmap (n bits) | O(1) | O(1) | O(1) | O(n/64) words | popcount O(n/64) |
| Wavelet tree | O(log σ) | — | — | O(n log σ) | rank/select/quantile |

---

## 34. Domain cross-reference

### Linux kernel

| Structure / Algorithm | Where |
|---|---|
| Red-black tree + leftmost cache | CFS scheduler run queue (kernel/sched/fair.c) |
| Bitmap + find_first_bit | RT scheduler priority array (kernel/sched/rt.c) |
| Trie (XArray / radix tree) | Page cache index (lib/xarray.c) |
| Interval tree (augmented RB) | VMA lookup mm/mmap.c (`find_vma`) |
| Circular doubly-linked list | `list_head` — used in ~10,000 places |
| Hash table (chaining) | PID table, dentry cache, inode cache, conntrack |
| Lock-free ring buffer | eBPF ring buffer, kfifo, perf ring |
| Union-Find | Cgroup hierarchy, namespace merging |
| BFS / topo-sort | Module load order (init/main.c) |
| Bloom filter | eBPF map fast-reject |

### Networking (Linux stack + DPDK)

| Structure / Algorithm | Where |
|---|---|
| Trie (LC-trie) | FIB longest-prefix-match routing table |
| Hash table | Connection tracking (src,dst,sport,dport,proto) → conntrack |
| Cuckoo hash | DPDK `rte_hash` flow classification |
| Ring buffer (SPSC) | NIC RX/TX descriptor ring |
| Queue (Qdisc) | HTB, FQ-CoDel, CAKE packet schedulers |
| Count-Min sketch | DDoS heavy-hitter detection |
| HyperLogLog | Unique source IP counting per second |
| Sliding window max | `fq_codel` minimum RTT tracking |

### Storage / Filesystems

| Structure / Algorithm | Where |
|---|---|
| B-tree | ext4 HTree, XFS extent tree, btrfs, APFS, NTFS |
| LSM tree | RocksDB, LevelDB, Cassandra, ScyllaDB |
| Bloom filter | RocksDB per-SSTable level skip |
| Interval tree | Extent overlap detection |
| Ring buffer | Write-ahead log (WAL) journal ring |
| Bitmap | ext4 block/inode allocation |
| Hash table | btrfs chunk map |

### Databases

| Structure / Algorithm | Where |
|---|---|
| B+ tree | InnoDB (MySQL), PostgreSQL, SQLite primary index |
| Hash join | PostgreSQL, DuckDB, Spark |
| Skip list | Redis sorted sets (`ZADD`/`ZRANGEBYSCORE`) |
| HyperLogLog | PostgreSQL `HLL` ext, Redis `PFCOUNT`, ClickHouse |
| Count-Min | ClickHouse, Druid approximate frequency |
| Bloom filter | RocksDB level skip, Cassandra SSTable |
| Bitmap (Roaring) | Apache Druid, ClickHouse, Pinot column indexes |
| Wavelet tree | DuckDB succinct column encoding |
| Topo-sort | Query plan DAG ordering |

### GPU / Compute

| Structure / Algorithm | Where |
|---|---|
| Ring buffer | CUDA command queue, profiler event ring |
| k-d tree / BVH | Ray tracing scene intersection |
| R-tree | Bounding volume hierarchy (BVH) construction |
| Bitmap (warp mask) | `__ballot_sync`, `__activemask`, lane masking |
| Parallel prefix sum | CUDA `thrust::inclusive_scan`, warp scan |
| Hash table (cuckoo) | GPU parallel hash table (Thrust, cuCollections) |
| Union-Find | CUDA connected components (parallel Shiloach-Vishkin) |
| DAG | CUDA stream dependency graph, graph execution |

### Machine Learning / AI

| Structure / Algorithm | Where |
|---|---|
| Array (tensor) | PyTorch / NumPy n-dimensional storage |
| DAG | Autograd computation graph (backprop) |
| Trie | BPE tokenizer vocabulary, autocomplete |
| Hash table | Embedding lookup table, feature store |
| k-d tree | k-NN inference, point cloud k-nearest |
| HyperLogLog | Dataset cardinality estimation |
| Bloom filter | Approximate dedup in online training |
| Heap | Beam search (top-k generation), A* pathfinding |
| Skip list | Approximate sorted embeddings |
| Prefix sum | Attention mask computation, CUDA scan |
| Union-Find | Cluster merging (hierarchical agglomerative clustering) |
