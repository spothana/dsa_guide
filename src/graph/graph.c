/*
 * graph.c — Adjacency-list graph: BFS, DFS, Dijkstra, Topo-sort, cycle detect
 */
#include "graph/graph.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

Graph *graph_create(int n, bool directed) {
    Graph *g = malloc(sizeof *g);
    g->n = n; g->directed = directed;
    g->adj = calloc(n, sizeof(GEdge*));
    return g;
}

static void free_edges(GEdge *e) {
    while (e) { GEdge *nx = e->next; free(e); e = nx; }
}

void graph_destroy(Graph *g) {
    for (int i=0;i<g->n;i++) free_edges(g->adj[i]);
    free(g->adj); free(g);
}

void graph_add_edge(Graph *g, int u, int v, int weight) {
    GEdge *e = malloc(sizeof *e);
    e->to = v; e->weight = weight; e->next = g->adj[u];
    g->adj[u] = e;
    if (!g->directed) {
        GEdge *f = malloc(sizeof *f);
        f->to = u; f->weight = weight; f->next = g->adj[v];
        g->adj[v] = f;
    }
}

/*
 * BFS — O(V+E)
 * Uses integer array as queue. Visits all reachable vertices.
 * Domain: shortest unweighted path, network flood fill, OS module deps
 */
void graph_bfs(const Graph *g, int src, int *order, int *out_n) {
    bool *vis = calloc(g->n, sizeof(bool));
    int  *q   = malloc(g->n * sizeof(int));
    int front = 0, back = 0, cnt = 0;
    vis[src] = true; q[back++] = src;
    while (front < back) {
        int v = q[front++];
        order[cnt++] = v;
        for (GEdge *e = g->adj[v]; e; e = e->next)
            if (!vis[e->to]) { vis[e->to]=true; q[back++]=e->to; }
    }
    *out_n = cnt;
    free(vis); free(q);
}

static void dfs_r(const Graph *g, int v, bool *vis, int *order, int *cnt) {
    vis[v] = true; order[(*cnt)++] = v;
    for (GEdge *e = g->adj[v]; e; e = e->next)
        if (!vis[e->to]) dfs_r(g, e->to, vis, order, cnt);
}

void graph_dfs(const Graph *g, int src, int *order, int *out_n) {
    bool *vis = calloc(g->n, sizeof(bool));
    *out_n = 0;
    dfs_r(g, src, vis, order, out_n);
    free(vis);
}

/*
 * Dijkstra — O((V+E) log V)
 * Min-heap stores (dist, vertex). Lazy deletion for visited.
 */
int *graph_dijkstra(const Graph *g, int src) {
    int *dist = malloc(g->n * sizeof(int));
    bool *vis = calloc(g->n, sizeof(bool));
    int *heap_k = malloc(g->n*4*sizeof(int));
    int *heap_v = malloc(g->n*4*sizeof(int));
    int hsz = 0;

    for (int i=0;i<g->n;i++) dist[i]=INT_MAX;
    dist[src]=0;

    /* simple heap push */
    #define HPUSH(k,v) do { heap_k[hsz]=k; heap_v[hsz]=v; \
        int _i=hsz++; \
        while(_i>0&&heap_k[(_i-1)/2]>heap_k[_i]) { \
            int _p=(_i-1)/2; \
            int _tk=heap_k[_i]; heap_k[_i]=heap_k[_p]; heap_k[_p]=_tk; \
            int _tv=heap_v[_i]; heap_v[_i]=heap_v[_p]; heap_v[_p]=_tv; \
            _i=_p; } } while(0)
    #define HPOP_V do { int _l,_r,_b,_i=0; \
        heap_k[0]=heap_k[--hsz]; heap_v[0]=heap_v[hsz]; \
        while(1){_l=2*_i+1;_r=2*_i+2;_b=_i; \
            if(_l<hsz&&heap_k[_l]<heap_k[_b])_b=_l; \
            if(_r<hsz&&heap_k[_r]<heap_k[_b])_b=_r; \
            if(_b==_i)break; \
            int _tk=heap_k[_i];heap_k[_i]=heap_k[_b];heap_k[_b]=_tk; \
            int _tv=heap_v[_i];heap_v[_i]=heap_v[_b];heap_v[_b]=_tv; \
            _i=_b;} } while(0)

    HPUSH(0, src);
    while (hsz > 0) {
        int d = heap_k[0], u = heap_v[0]; HPOP_V;
        if (vis[u]) continue; vis[u]=true; (void)d;
        for (GEdge *e=g->adj[u]; e; e=e->next) {
            if (!vis[e->to] && dist[u]!=INT_MAX && dist[u]+e->weight<dist[e->to]) {
                dist[e->to] = dist[u]+e->weight;
                HPUSH(dist[e->to], e->to);
            }
        }
    }
    free(vis); free(heap_k); free(heap_v);
    return dist;
}

/*
 * Bellman-Ford — O(VE), handles negative weights
 */
int *graph_bellman_ford(const Graph *g, int src, bool *neg_cycle_out) {
    int *dist = malloc(g->n * sizeof(int));
    for (int i=0;i<g->n;i++) dist[i]=INT_MAX;
    dist[src]=0; *neg_cycle_out=false;
    for (int pass=0; pass<g->n-1; pass++)
        for (int u=0; u<g->n; u++)
            for (GEdge *e=g->adj[u]; e; e=e->next)
                if (dist[u]!=INT_MAX && dist[u]+e->weight<dist[e->to])
                    dist[e->to]=dist[u]+e->weight;
    for (int u=0; u<g->n; u++)
        for (GEdge *e=g->adj[u]; e; e=e->next)
            if (dist[u]!=INT_MAX && dist[u]+e->weight<dist[e->to])
                *neg_cycle_out=true;
    return dist;
}

/*
 * Topological sort — Kahn's BFS algorithm O(V+E)
 * Invariant: process vertices with in-degree 0 first.
 * Shrinks in-degrees as edges are consumed.
 * Domain: build systems (make), CUDA kernel scheduling, module load order
 */
int *graph_topological_sort(const Graph *g, bool *is_dag) {
    int *indeg = calloc(g->n, sizeof(int));
    for (int u=0;u<g->n;u++)
        for (GEdge *e=g->adj[u];e;e=e->next) indeg[e->to]++;
    int *q = malloc(g->n*sizeof(int));
    int *order = malloc(g->n*sizeof(int));
    int front=0,back=0,cnt=0;
    for (int i=0;i<g->n;i++) if (indeg[i]==0) q[back++]=i;
    while (front<back) {
        int v=q[front++]; order[cnt++]=v;
        for (GEdge *e=g->adj[v];e;e=e->next)
            if (--indeg[e->to]==0) q[back++]=e->to;
    }
    *is_dag = (cnt == g->n);
    free(indeg); free(q);
    return order;
}

static bool has_cycle_dfs(const Graph *g, int v, bool *vis, bool *rec) {
    vis[v]=rec[v]=true;
    for (GEdge *e=g->adj[v];e;e=e->next) {
        if (!vis[e->to] && has_cycle_dfs(g,e->to,vis,rec)) return true;
        if (rec[e->to]) return true;
    }
    rec[v]=false; return false;
}

bool graph_has_cycle(const Graph *g) {
    bool *vis = calloc(g->n,sizeof(bool));
    bool *rec = calloc(g->n,sizeof(bool));
    bool found=false;
    for (int i=0;i<g->n&&!found;i++)
        if (!vis[i]) found=has_cycle_dfs(g,i,vis,rec);
    free(vis);free(rec);return found;
}

int graph_connected_components(const Graph *g, int *comp) {
    bool *vis=calloc(g->n,sizeof(bool));
    int *ord=malloc(g->n*sizeof(int));
    int nc=0;
    for (int i=0;i<g->n;i++) if (!vis[i]) {
        int cnt=0; dfs_r(g,i,vis,ord,&cnt);
        for (int j=0;j<cnt;j++) comp[ord[j]]=nc;
        nc++;
    }
    free(vis);free(ord);return nc;
}

void graph_print(const Graph *g) {
    printf("Graph (%s, V=%d):\n", g->directed?"directed":"undirected", g->n);
    for (int u=0;u<g->n;u++) {
        printf("  %d:", u);
        for (GEdge *e=g->adj[u];e;e=e->next) printf(" ->%d(w=%d)",e->to,e->weight);
        printf("\n");
    }
}
