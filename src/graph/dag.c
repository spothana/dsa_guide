/*
 * dag.c — DAG: Kahn's topo-sort, DFS topo-sort, critical path
 */
#include "graph/dag.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

DAG *dag_create(int n) {
    DAG *d = malloc(sizeof *d);
    d->n = n;
    d->adj      = calloc(n, sizeof(int*));
    d->weights  = NULL;
    d->in_degree = calloc(n, sizeof(int));
    for (int i=0;i<n;i++) d->adj[i] = calloc(n+1, sizeof(int)); /* adj[i][0]=count */
    return d;
}

void dag_destroy(DAG *d) {
    for (int i=0;i<d->n;i++) free(d->adj[i]);
    free(d->adj); free(d->in_degree); free(d->weights); free(d);
}

bool dag_add_edge(DAG *d, int u, int v, int weight) {
    (void)weight;
    int cnt = d->adj[u][0];
    d->adj[u][0]++;
    d->adj[u][cnt+1] = v;
    d->in_degree[v]++;
    return true;  /* cycle check omitted for brevity — use dag_is_valid() */
}

/*
 * Kahn's BFS topological sort — O(V+E)
 *
 * Algorithm:
 *  1. Initialise queue with all zero-indegree vertices
 *  2. Dequeue vertex v, add to order, decrement neighbour in-degrees
 *  3. If neighbour in-degree hits 0, enqueue it
 *  4. If processed count < V → graph has a cycle
 *
 * Domain: Linux kbuild module order, CI pipeline stage scheduling,
 *         CUDA kernel launch order, makefile target resolution
 */
int *dag_topo_kahn(const DAG *d, int *out_n) {
    int *indeg = malloc(d->n * sizeof(int));
    memcpy(indeg, d->in_degree, d->n * sizeof(int));

    int *queue = malloc(d->n * sizeof(int));
    int *order = malloc(d->n * sizeof(int));
    int front=0, back=0, cnt=0;

    for (int i=0;i<d->n;i++) if (indeg[i]==0) queue[back++]=i;

    while (front < back) {
        int v = queue[front++];
        order[cnt++] = v;
        for (int i=1; i<=d->adj[v][0]; i++) {
            int nb = d->adj[v][i];
            if (--indeg[nb] == 0) queue[back++] = nb;
        }
    }
    *out_n = cnt;
    free(indeg); free(queue);
    return order;
}

static void dfs_topo_r(const DAG *d, int v, bool *vis, int *stack, int *top) {
    vis[v] = true;
    for (int i=1;i<=d->adj[v][0];i++) {
        int nb=d->adj[v][i];
        if (!vis[nb]) dfs_topo_r(d, nb, vis, stack, top);
    }
    stack[(*top)++] = v;
}

int *dag_topo_dfs(const DAG *d, int *out_n) {
    bool *vis   = calloc(d->n, sizeof(bool));
    int *stack  = malloc(d->n * sizeof(int));
    int *order  = malloc(d->n * sizeof(int));
    int top=0;

    for (int i=0;i<d->n;i++) if (!vis[i]) dfs_topo_r(d,i,vis,stack,&top);

    /* stack is reverse topo order */
    for (int i=0;i<top;i++) order[i]=stack[top-1-i];
    *out_n = top;
    free(vis); free(stack);
    return order;
}

/*
 * Critical path (longest path in DAG) — O(V+E)
 * Process vertices in topological order, relax edges forward.
 * Domain: project scheduling (CPM), CUDA kernel stream latency,
 *         compiler instruction scheduling
 */
int *dag_critical_path(const DAG *d, int *out_len) {
    int n; int *order = dag_topo_kahn(d, &n);
    int *dist = malloc(d->n * sizeof(int));
    for (int i=0;i<d->n;i++) dist[i]=0;

    for (int i=0;i<n;i++) {
        int v=order[i];
        for (int j=1;j<=d->adj[v][0];j++) {
            int nb=d->adj[v][j];
            if (dist[v]+1 > dist[nb]) dist[nb]=dist[v]+1;
        }
    }
    int mx=0;
    for (int i=0;i<d->n;i++) if (dist[i]>mx) mx=dist[i];
    *out_len=mx;
    free(order);
    return dist;
}

bool dag_is_valid(const DAG *d) {
    int n; int *order = dag_topo_kahn(d, &n);
    free(order);
    return n == d->n;
}
