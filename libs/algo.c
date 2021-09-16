#include "algo.h"
#include "lock.h"

typedef mutex_lock_t item_t;

void qsort(void *a, int left, int right, int cmp(void *, void *))
{
    if (left >= right)
        return;

    item_t **b = (item_t **)a;
    int last = left;
    for (int i = left; i <= right; ++i)
    {
        if (cmp((void *)b[i], (void *)b[last]) < 0)
        {
            item_t *tmp = b[i];
            b[i] = b[++last];
            b[last] = tmp;
        }
    }
    item_t *tmp = b[left];
    b[left] = b[last];
    b[last] = tmp;
    qsort(a, left, last, cmp);
    qsort(a, last + 1, right, cmp);
}

int cmp_lock(void *a, void *b)
{
    return ((item_t *)a)->lock_id - ((item_t *)b)->lock_id;
}

// Get parent of a node in the tree. Return NULL if no parent.
node_t *get_parent(node_t *n)
{
    return n == NULL ? NULL : n->parent;
}

// Get grandparent of a node in the tree. Return NULL if no grandparent.
node_t *get_grandparent(node_t *n)
{
    return get_parent(get_parent(n));
}

// Get uncle of a node in the tree. Return NULL if no uncle.
node_t *get_sibling(node_t *n)
{
    node_t *p = get_parent(n);

    // no parent means no sibling
    if (p == NULL)
    {
        return NULL;
    }

    if (p->left == n)
    {
        return p->right;
    }
    else
    {
        return p->left;
    }
}

// Get sibling of a node in the tree. Return NULL if no sibling.
node_t *get_uncle(node_t *n)
{
    return get_sibling(get_parent(n));
}

// rotate the node n left to make its right child the new top
void rotate_left(node_t *n)
{
    node_t *nnew = n->right;
    node_t *p = get_parent(n);

    n->right = nnew->left;
    nnew->left = n;
    n->parent = nnew;
    if (n->right != NULL)
    {
        n->right->parent = n;
    }
    if (p != NULL)
    {
        if (p->left = n)
        {
            p->left = nnew;
        }
        else
        {
            p->right = nnew;
        }
    }
    nnew->parent = p;
}

// rotate the node n right to make its left child the new top
void rotate_right(node_t *n)
{
    node_t *nnew = n->left;
    node_t *p = get_parent(n);

    n->left = nnew->right;
    nnew->right = n;
    n->parent = nnew;
    if (n->left != NULL)
    {
        n->left->parent = n;
    }
    if (p != NULL)
    {
        if (p->left == n)
        {
            p->left = nnew;
        }
        else
        {
            p->right = nnew;
        }
    }
    nnew->parent = p;
}

// insert a node into the tree and return the new root, with red-black properties maintained
node_t *insert(node_t *root, node_t *n)
{
    // insert the node without maintaining red-black properties
    insert_recurse(root, n);
    // repair the red-black properties
    insert_repair(n);
    // find the new root to return
    root = n;
    while (get_parent(root) != NULL)
    {
        root = get_parent(root);
    }
    return root;
}

// recursively insert a node into the tree, without red-black properties maintained
void insert_recurse(node_t *root, node_t *n)
{
    if (root != NULL && root->key > n->key)
    {
        if (root->left != NULL)
        {
            insert_recurse(root->left, n);
            return;
        }
        else
        {
            root->left = n;
        }
    }
    else if (root != NULL)
    {
        if (root->right != NULL)
        {
            insert_recurse(root->right, n);
            return;
        }
        else
        {
            root->right = n;
        }
    }

    // only leaf function calls reach here
    n->parent = root;
    n->left = n->right = NULL;
    n->color = RED;
}

// repair the red-black properties after insert_recuse
void insert_repair(node_t *n)
{

    // note that n might not be a leaf node for recursive calls.

    if (get_parent(n) == NULL)
    {
        // the new node is inserted as the root, only to color it to black is enough.
        n->color = BLACK;
    }
    else if (get_parent(n)->color == BLACK)
    {
        // nothing to do actually, because there are only limitations on red nodes' children's colors.
    }
    else if (get_uncle(n) != NULL && get_uncle(n)->color == RED)
    {
        // parent and uncle are red, conflicting the rule that red nodes have only black children.
        // recolor n's parent, uncle and grand parent then perform a recursive call on n's grandparent.
        get_parent(n)->color = BLACK;
        get_uncle(n)->color = BLACK;
        node_t *grandpa = get_grandparent(n);
        grandpa->color = RED;
        insert_repair(grandpa);
    }
    else
    {
        // parent is red and uncle is black.
        // NULL node is considered black and no uncle entry access occurs here so there's no need to check if uncle is NULL
        // perform a double rotate and recolor.

        // cancel the zig-zag pattern of n, n's parent and n's grandparent
        node_t *p = get_parent(n);
        node_t *g = get_grandparent(n);
        if (g->left == p && p->right == n)
        {
            rotate_left(p);
            n = n->left;
        }
        else if (g->right == p && p->left == n)
        {
            rotate_right(p);
            n = n->right;
        }
        p = get_parent(n);
        g = get_grandparent(n);
        // now only left-left or right-right
        if (p->left == n)
        {
            rotate_right(g);
        }
        else if (p->right == n)
        {
            rotate_left(g);
        }
        p->color = BLACK;
        g->color = RED;
    }
}
