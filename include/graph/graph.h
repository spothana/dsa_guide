/*
 * graph.h — Adjacency-list graph with BFS, DFS, Dijkstra, Bellman-Ford, Topo-sort
 *
 * Algorithms: BFS, DFS, Dijkstra, Bellman-Ford, topological sort
 * Domain use: network topology, BGP AS path, filesystem link graph,
 *             CUDA stream deps, attack graph, neural net computation graph
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef struct GEdge {
    int         to;
    int         weight;
    struct GEdge *next;
} GEdge;

typedef struct {
    GEdge **adj;        /* adjacency list per vertex */
    int     n;          /* number of vertices */
    bool    directed;
} Graph;

Graph *graph_create(int n, bool directed);
void   graph_destroy(Graph *g);
void   graph_add_edge(Graph *g, int u, int v, int weight);

/* traversal — writes result into out[] (caller provides size-n array) */
void   graph_bfs(const Graph *g, int src, int *order, int *out_n);
void   graph_dfs(const Graph *g, int src, int *order, int *out_n);

/* shortest path */
int   *graph_dijkstra(const Graph *g, int src);           /* O((V+E) log V) */
int   *graph_bellman_ford(const Graph *g, int src,
                          bool *neg_cycle_out);           /* O(VE) */

/* topological sort (Kahn's algorithm, BFS-based) — only for DAGs */
int   *graph_topological_sort(const Graph *g, bool *is_dag);

/* cycle detection */
bool   graph_has_cycle(const Graph *g);

/* connected components */
int    graph_connected_components(const Graph *g, int *comp);

void   graph_print(const Graph *g);
