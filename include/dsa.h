/*
 * dsa.h — Master header for DSA Guide
 *
 * Include this single header to access every data structure
 * and algorithm implemented in this project.
 *
 * Organisation:
 *   Linear        : array, linked_list, stack, queue, ring_buffer, circular_list
 *   Trees         : red_black_tree, btree, trie, segment_tree, heap
 *   Hash          : hash_table, cuckoo_hash
 *   Graph         : graph (BFS/DFS/Dijkstra/Bellman-Ford/Topo), dag, union_find
 *   Probabilistic : bloom_filter, skip_list, count_min, hyperloglog
 *   Spatial       : kd_tree, rtree
 *   Specialized   : lsm_tree, bitmap, wavelet_tree
 */

#pragma once

/* ── Linear ─────────────────────────────────────────────────────────────── */
#include "linear/array.h"
#include "linear/linked_list.h"
#include "linear/stack.h"
#include "linear/queue.h"
#include "linear/ring_buffer.h"
#include "linear/circular_list.h"

/* ── Trees ──────────────────────────────────────────────────────────────── */
#include "trees/red_black_tree.h"
#include "trees/btree.h"
#include "trees/trie.h"
#include "trees/segment_tree.h"
#include "trees/heap.h"

/* ── Hash ───────────────────────────────────────────────────────────────── */
#include "hash/hash_table.h"
#include "hash/cuckoo_hash.h"

/* ── Graph ──────────────────────────────────────────────────────────────── */
#include "graph/graph.h"
#include "graph/dag.h"
#include "graph/union_find.h"

/* ── Probabilistic ──────────────────────────────────────────────────────── */
#include "probabilistic/bloom_filter.h"
#include "probabilistic/skip_list.h"
#include "probabilistic/count_min.h"
#include "probabilistic/hyperloglog.h"

/* ── Spatial ────────────────────────────────────────────────────────────── */
#include "spatial/kd_tree.h"
#include "spatial/rtree.h"

/* ── Specialized ────────────────────────────────────────────────────────── */
#include "specialized/lsm_tree.h"
#include "specialized/bitmap.h"
#include "specialized/wavelet_tree.h"
