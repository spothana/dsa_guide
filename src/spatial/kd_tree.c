/*
 * kd_tree.c — 2D k-d tree: nearest-neighbour + range search
 */
#include "spatial/kd_tree.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

KDTree *kd_create(void) {
    KDTree *t = malloc(sizeof *t);
    t->root = NULL; t->size = 0;
    return t;
}

static void kd_free_r(KDNode *n) {
    if (!n) return;
    kd_free_r(n->left); kd_free_r(n->right); free(n);
}

void kd_destroy(KDTree *t) { kd_free_r(t->root); free(t); }

/*
 * Insert — alternates splitting axis by depth (x at even, y at odd)
 * Complexity: O(log n) average for balanced trees
 * Domain: geo-IP lookup, PostGIS point index, GPU ray-scene intersection
 */
static KDNode *kd_insert_r(KDNode *n, double x, double y, int val, int depth) {
    if (!n) {
        KDNode *node = malloc(sizeof *node);
        node->point[0]=x; node->point[1]=y; node->val=val;
        node->left=node->right=NULL;
        return node;
    }
    int axis = depth % KD_DIM;
    double cmp = (axis==0) ? x-n->point[0] : y-n->point[1];
    if (cmp < 0) n->left  = kd_insert_r(n->left,  x,y,val,depth+1);
    else         n->right = kd_insert_r(n->right, x,y,val,depth+1);
    return n;
}

void kd_insert(KDTree *t, double x, double y, int val) {
    t->root = kd_insert_r(t->root, x, y, val, 0);
    t->size++;
}

static double dist2(double ax, double ay, double bx, double by) {
    double dx=ax-bx, dy=ay-by; return dx*dx+dy*dy;
}

/*
 * Nearest-neighbour search — O(log n) average, O(n) worst-case
 *
 * Algorithm:
 *  1. Descend to the leaf most likely to contain nearest point
 *  2. Unwind: for each node, check if the splitting hyperplane
 *     is closer than current best — if so, explore other branch
 *
 * The hyperplane check is the key: dist_to_plane² < best_dist²
 * means the other half-space might have a closer point.
 *
 * Domain: k-NN classification, CUDA point cloud processing,
 *         GPS nearest-POI lookup
 */
static void nn_r(KDNode *n, double qx, double qy, int depth,
                 KDNode **best, double *best_dist2) {
    if (!n) return;
    double d2 = dist2(qx, qy, n->point[0], n->point[1]);
    if (d2 < *best_dist2) { *best_dist2=d2; *best=n; }

    int axis = depth % KD_DIM;
    double diff = (axis==0) ? qx-n->point[0] : qy-n->point[1];
    KDNode *near = diff<0 ? n->left : n->right;
    KDNode *far  = diff<0 ? n->right : n->left;

    nn_r(near, qx, qy, depth+1, best, best_dist2);
    /* only explore far branch if splitting plane is within best distance */
    if (diff*diff < *best_dist2)
        nn_r(far, qx, qy, depth+1, best, best_dist2);
}

KDNode *kd_nearest(KDTree *t, double x, double y) {
    KDNode *best = NULL; double bd2 = DBL_MAX;
    nn_r(t->root, x, y, 0, &best, &bd2);
    return best;
}

static void range_r(KDNode *n, double qx, double qy, double r2,
                    int depth, void (*visit)(KDNode *)) {
    if (!n) return;
    if (dist2(qx,qy,n->point[0],n->point[1]) <= r2) visit(n);
    int axis = depth % KD_DIM;
    double diff = (axis==0) ? qx-n->point[0] : qy-n->point[1];
    range_r(n->left,  qx,qy,r2,depth+1,visit);
    if (diff*diff <= r2)   /* both halves may overlap sphere */
        range_r(n->right, qx,qy,r2,depth+1,visit);
}

void kd_range_search(KDTree *t, double x, double y, double r,
                     void (*visit)(KDNode *)) {
    range_r(t->root, x, y, r*r, 0, visit);
}

/* Bulk-load: sort points by median, recurse — O(n log n), balanced */
static int cmp_x(const void *a, const void *b) {
    double d = ((double(*)[KD_DIM])a)[0][0] - ((double(*)[KD_DIM])b)[0][0];
    return d<0?-1:d>0?1:0;
}
static int cmp_y(const void *a, const void *b) {
    double d = ((double(*)[KD_DIM])a)[0][1] - ((double(*)[KD_DIM])b)[0][1];
    return d<0?-1:d>0?1:0;
}
static KDNode *build_r(double (*pts)[KD_DIM], int *vals, int n, int depth) {
    if (n<=0) return NULL;
    if (depth%2==0) qsort(pts, n, sizeof(pts[0]), cmp_x);
    else            qsort(pts, n, sizeof(pts[0]), cmp_y);
    int mid=n/2;
    KDNode *node = malloc(sizeof *node);
    node->point[0]=pts[mid][0]; node->point[1]=pts[mid][1];
    node->val=vals?vals[mid]:0;
    node->left  = build_r(pts,       vals,       mid,   depth+1);
    node->right = build_r(pts+mid+1, vals?vals+mid+1:NULL, n-mid-1, depth+1);
    return node;
}

KDTree *kd_build(double (*points)[KD_DIM], int *vals, int n) {
    KDTree *t = kd_create();
    t->root = build_r(points, vals, n, 0);
    t->size = (size_t)n;
    return t;
}
