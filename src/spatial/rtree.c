/*
 * rtree.c — Simplified R-tree (2D bounding boxes)
 */
#include "spatial/rtree.h"
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>

static double mbr_area(MBR r) { return (r.x2-r.x1)*(r.y2-r.y1); }
static bool   mbr_overlap(MBR a, MBR b) {
    return a.x1<=b.x2 && a.x2>=b.x1 && a.y1<=b.y2 && a.y2>=b.y1;
}
static MBR mbr_union(MBR a, MBR b) {
    return (MBR){
        .x1=a.x1<b.x1?a.x1:b.x1, .y1=a.y1<b.y1?a.y1:b.y1,
        .x2=a.x2>b.x2?a.x2:b.x2, .y2=a.y2>b.y2?a.y2:b.y2
    };
}
static double mbr_enlarge(MBR orig, MBR add) {
    MBR u = mbr_union(orig, add);
    return mbr_area(u) - mbr_area(orig);
}

static RTreeNode *rtnode_new(bool leaf) {
    RTreeNode *n = calloc(1, sizeof *n);
    n->is_leaf = leaf; n->count = 0;
    n->mbr = (MBR){DBL_MAX, DBL_MAX, -DBL_MAX, -DBL_MAX};
    return n;
}

RTree *rtree_create(void) {
    RTree *t = malloc(sizeof *t);
    t->root = rtnode_new(true);
    t->size = 0;
    return t;
}

static void rtnode_free(RTreeNode *n) {
    if (!n) return;
    if (!n->is_leaf) for (int i=0;i<n->count;i++) rtnode_free(n->children[i]);
    free(n);
}

void rtree_destroy(RTree *t) { rtnode_free(t->root); free(t); }

static void rtnode_recompute_mbr(RTreeNode *n) {
    n->mbr = (MBR){DBL_MAX, DBL_MAX, -DBL_MAX, -DBL_MAX};
    if (n->is_leaf) {
        for (int i=0;i<n->count;i++) n->mbr = mbr_union(n->mbr, n->rects[i]);
    } else {
        for (int i=0;i<n->count;i++) n->mbr = mbr_union(n->mbr, n->children[i]->mbr);
    }
}

/*
 * Choose subtree: pick child whose MBR needs least enlargement to contain rect.
 * Breaks ties by smallest area — minimises dead space.
 * Domain: PostGIS insert path, BVH construction for GPU ray tracing
 */
static int choose_subtree(RTreeNode *n, MBR rect) {
    int best=0; double best_enl=DBL_MAX, best_area=DBL_MAX;
    for (int i=0;i<n->count;i++) {
        double enl = mbr_enlarge(n->children[i]->mbr, rect);
        double area = mbr_area(n->children[i]->mbr);
        if (enl < best_enl || (enl==best_enl && area<best_area)) {
            best=i; best_enl=enl; best_area=area;
        }
    }
    return best;
}

/* Simplified insert (no split on overflow — educational version) */
static void insert_r(RTreeNode *n, MBR rect, int id) {
    if (n->is_leaf) {
        if (n->count < RTREE_MAX_ENTRIES) {
            n->rects[n->count] = rect;
            n->data[n->count]  = id;
            n->count++;
            n->mbr = mbr_union(n->mbr, rect);
        }
        /* overflow: in production, split node here */
        return;
    }
    int idx = choose_subtree(n, rect);
    insert_r(n->children[idx], rect, id);
    rtnode_recompute_mbr(n);
}

void rtree_insert(RTree *t, double x1, double y1, double x2, double y2, int id) {
    MBR rect = {x1, y1, x2, y2};
    /* if root is leaf and full, split into internal node */
    if (t->root->is_leaf && t->root->count == RTREE_MAX_ENTRIES) {
        RTreeNode *new_root = rtnode_new(false);
        new_root->children[0] = t->root;
        new_root->count = 1;
        rtnode_recompute_mbr(new_root);
        t->root = new_root;
    }
    insert_r(t->root, rect, id);
    t->size++;
}

/*
 * Bounding-box search — O(log n) average, O(n) worst-case
 * Prunes branches whose MBR does not overlap query rectangle.
 * Domain: PostGIS ST_Intersects, geo-fencing containment check,
 *         GPU BVH traversal for ray-box intersection test
 */
static void search_r(RTreeNode *n, MBR query, void (*visit)(int, MBR)) {
    if (!mbr_overlap(n->mbr, query)) return;
    if (n->is_leaf) {
        for (int i=0;i<n->count;i++)
            if (mbr_overlap(n->rects[i], query)) visit(n->data[i], n->rects[i]);
    } else {
        for (int i=0;i<n->count;i++) search_r(n->children[i], query, visit);
    }
}

void rtree_search(RTree *t, double x1, double y1, double x2, double y2,
                  void (*visit)(int id, MBR)) {
    search_r(t->root, (MBR){x1,y1,x2,y2}, visit);
}
