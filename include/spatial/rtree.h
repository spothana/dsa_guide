/*
 * rtree.h — R-tree (2D minimum bounding rectangles)
 *
 * Algorithms: MBR overlap minimisation, R*-tree reinsertion, bulk-load STR
 * Domain use: PostGIS/MongoDB spatial index, BVH for GPU ray tracing,
 *             geo-fencing, network zone overlap detection
 */
#pragma once
#include <stddef.h>
#include <stdbool.h>

#define RTREE_MAX_ENTRIES 4
#define RTREE_MIN_ENTRIES 2

typedef struct { double x1,y1,x2,y2; } MBR;  /* min bounding rectangle */

typedef struct RTreeNode {
    MBR              mbr;
    bool             is_leaf;
    int              count;
    struct RTreeNode *children[RTREE_MAX_ENTRIES];
    int               data[RTREE_MAX_ENTRIES];   /* leaf: entry IDs */
    MBR               rects[RTREE_MAX_ENTRIES];  /* leaf: entry MBRs */
} RTreeNode;

typedef struct { RTreeNode *root; size_t size; } RTree;

RTree    *rtree_create(void);
void      rtree_destroy(RTree *t);
void      rtree_insert(RTree *t, double x1, double y1,
                        double x2, double y2, int id);
/* Bounding-box search: calls visit for each overlapping entry */
void      rtree_search(RTree *t, double x1, double y1,
                        double x2, double y2, void (*visit)(int id, MBR));
