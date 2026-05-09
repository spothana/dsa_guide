/*
 * dag.h — DAG: topological sort + critical path
 *
 * Algorithms: Kahn's, DFS topo-sort, critical path (longest path)
 * Domain use: module load order, CUDA kernel launch DAG, query plan DAG,
 *             autograd computation graph, CI pipeline, COW snapshot ancestry
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    int  **adj;
    int   *weights;       /* edge weights for critical path */
    int   *in_degree;
    int    n;
} DAG;

DAG   *dag_create(int n);
void   dag_destroy(DAG *d);
bool   dag_add_edge(DAG *d, int u, int v, int weight);  /* returns false if cycle */

/* Kahn's BFS-based topological sort — O(V+E) */
int   *dag_topo_kahn(const DAG *d, int *out_n);

/* DFS-based topological sort — O(V+E) */
int   *dag_topo_dfs(const DAG *d, int *out_n);

/* Critical path (longest path) — O(V+E) on DAG */
int   *dag_critical_path(const DAG *d, int *out_len);

bool   dag_is_valid(const DAG *d);   /* checks for cycles */
