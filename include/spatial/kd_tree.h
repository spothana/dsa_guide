/*
 * kd_tree.h — k-d tree (2D specialised for clarity, generalises to k dims)
 *
 * Algorithms: O(log n) avg nearest-neighbour, median-split partitioning
 * Domain use: NUMA topology lookup, geo-IP lookup, PostGIS spatial queries,
 *             CUDA ray-triangle intersection, k-NN inference, point cloud
 */
#pragma once
#include <stddef.h>
#include <stdbool.h>

#define KD_DIM 2   /* 2-dimensional for clarity; trivially extendable */

typedef struct KDNode {
    double         point[KD_DIM];
    int            val;
    struct KDNode *left, *right;
} KDNode;

typedef struct {
    KDNode *root;
    size_t  size;
} KDTree;

KDTree  *kd_create(void);
void     kd_destroy(KDTree *t);
void     kd_insert(KDTree *t, double x, double y, int val);

/* Nearest-neighbour search — returns closest node */
KDNode  *kd_nearest(KDTree *t, double x, double y);

/* Range search: all points within radius r of (x,y) */
void     kd_range_search(KDTree *t, double x, double y, double r,
                         void (*visit)(KDNode *));

/* Build balanced tree from point array (bulk load) — O(n log n) */
KDTree  *kd_build(double (*points)[KD_DIM], int *vals, int n);
