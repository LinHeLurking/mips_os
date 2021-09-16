#ifndef __ALGO_H
#define __ALGO_H

#include "type.h"

#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a < b ? b : a)

#define RED 0
#define BLACK 1

// data structure for red-black balanced searching tree
struct node_t
{
    uint8_t color;
    uint32_t key;
    void *entry;
    struct node_t *parent;
    struct node_t *left;
    struct node_t *right;
};

typedef struct node_t node_t;

// Get parent of a node in the tree. Return NULL if no parent.
node_t *get_parent(node_t *n);

// Get grandparent of a node in the tree. Return NULL if no grandparent.
node_t *get_grandparent(node_t *n);

// Get uncle of a node in the tree. Return NULL if no uncle.
node_t *get_uncle(node_t *n);

// Get sibling of a node in the tree. Return NULL if no sibling.
node_t *get_sibling(node_t *n);

// rotate the node n left to make its right child the new top
void rotate_left(node_t *n);

// rotate the node n right to make its left child the new top
void rotate_right(node_t *n);

// insert a node into the tree and return the new root, with red-black properties maintained
node_t *insert(node_t *root, node_t *n);

// recursively insert a node into the tree, without red-black properties maintained
void insert_recurse(node_t *root, node_t *n);

// repair the red-black properties after insert_recuse
void insert_repair(node_t *n);

void qsort(void *a, int left, int right, int cmp(void *, void *));

int cmp_lock(void *a, void *b);

#endif