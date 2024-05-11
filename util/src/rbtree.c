#include <alloc.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum Color {
    RED,
    BLACK
} Color;

// Define node structure
typedef struct RBTreeNode {
    Color color;
    int key;
    uint64_t value;
    struct RBTreeNode *left, *right, *parent;
} Node;

typedef struct RBTree {
    Node *root;
} RBTree;

// Function to create new node
Node *create_node(int key, uint64_t value)
{
    Node *node  = (Node *)g_kmalloc(sizeof(Node));
    node->key   = key;
    node->value = value;
    node->color = RED;
    node->left = node->right = node->parent = NULL;
    return node;
}

static const Node *csearch(const Node *node, int key)
{
    while (node != NULL) {
        if (key < node->key) {
            node = node->left;
        }
        else if (key > node->key) {
            node = node->right;
        }
        else {
            return node;
        }
    }
    return NULL;
}

static Node *search(Node *node, int key)
{
    while (node != NULL) {
        if (key < node->key) {
            node = node->left;
        }
        else if (key > node->key) {
            node = node->right;
        }
        else {
            return node;
        }
    }
    return NULL;
}


static Node *minimum(Node *node)
{
    while (node->left != NULL) {
        node = node->left;
    }
    return node;
}

static const Node *cminimum(const Node *node)
{
    while (node->left != NULL) {
        node = node->left;
    }
    return node;
}

static const Node *cmaximum(const Node *node)
{
    while (node->right != NULL) {
        node = node->right;
    }
    return node;
}


bool rb_min(const RBTree *rb, int *key)
{
    if (rb->root == NULL) {
        return false;
    }
    const Node *n = cminimum(rb->root);
    if (n != NULL) {
        if (key) {
            *key = n->key;
        }
        return true;
    }
    return false;
}

bool rb_max(const RBTree *rb, int *key)
{
    if (rb->root == NULL) {
        return false;
    }
    const Node *n = cmaximum(rb->root);
    if (n != NULL) {
        if (key) {
            *key = n->key;
        }
        return true;
    }
    return false;
}

bool rb_min_val(const RBTree *rb, uint64_t *value)
{
    if (rb->root == NULL) {
        return false;
    }
    const Node *n = cminimum(rb->root);
    if (n) {
        if (value) {
            *value = n->value;
        }
        return true;
    }
    return false;
}

bool rb_max_val(const RBTree *rb, uint64_t *value)
{
    if (rb->root == NULL) {
        return false;
    }
    const Node *n = cmaximum(rb->root);
    if (n) {
        if (value) {
            *value = n->value;
        }
        return true;
    }
    return false;
}

// Function to left rotate tree at given node
static void left_rotate(RBTree *rb, Node *x)
{
    Node *y  = x->right;
    x->right = y->left;
    if (y->left != NULL) {
        y->left->parent = x;
    }
    y->parent = x->parent;
    if (x->parent == NULL) {
        rb->root = y;
    }
    else if (x == x->parent->left) {
        x->parent->left = y;
    }
    else {
        x->parent->right = y;
    }
    y->left   = x;
    x->parent = y;
}

// Function to right rotate tree at given node
static void right_rotate(RBTree *rb, Node *y)
{
    Node *x = y->left;
    y->left = x->right;
    if (x->right != NULL) {
        x->right->parent = y;
    }
    x->parent = y->parent;
    if (y->parent == NULL) {
        rb->root = x;
    }
    else if (y == y->parent->left) {
        y->parent->left = x;
    }
    else {
        y->parent->right = x;
    }
    x->right  = y;
    y->parent = x;
}

// Function to fix the red-black tree after insertion
static void fix_insert(RBTree *rb, Node *z)
{
    while (z != rb->root && z->parent->color == RED) {
        if (z->parent == z->parent->parent->left) {
            Node *y = z->parent->parent->right;
            if (y != NULL && y->color == RED) {
                z->parent->color         = BLACK;
                y->color                 = BLACK;
                z->parent->parent->color = RED;
                z                        = z->parent->parent;
            }
            else {
                if (z == z->parent->right) {
                    z = z->parent;
                    left_rotate(rb, z);
                }
                z->parent->color         = BLACK;
                z->parent->parent->color = RED;
                right_rotate(rb, z->parent->parent);
            }
        }
        else {
            Node *y = z->parent->parent->left;
            if (y != NULL && y->color == RED) {
                z->parent->color         = BLACK;
                y->color                 = BLACK;
                z->parent->parent->color = RED;
                z                        = z->parent->parent;
            }
            else {
                if (z == z->parent->left) {
                    z = z->parent;
                    right_rotate(rb, z);
                }
                z->parent->color         = BLACK;
                z->parent->parent->color = RED;
                left_rotate(rb, z->parent->parent);
            }
        }
    }
    rb->root->color = BLACK;
}

// Function to insert a node into the red-black tree
void rb_insert(RBTree *rb, int key, uint64_t value)
{
    Node *z = create_node(key, value);
    Node *y = NULL;
    Node *x = rb->root;
    while (x != NULL) {
        y = x;
        if (z->key < x->key) {
            x = x->left;
        }
        else {
            x = x->right;
        }
    }
    z->parent = y;
    if (y == NULL) {
        rb->root = z;
    }
    else if (z->key < y->key) {
        y->left = z;
    }
    else {
        y->right = z;
    }
    fix_insert(rb, z);
}

static void transplant(RBTree *rb, Node *u, Node *v)
{
    if (u->parent == NULL) {
        rb->root = v;
    }
    else if (u == u->parent->left) {
        u->parent->left = v;
    }
    else {
        u->parent->right = v;
    }
    if (v != NULL) {
        v->parent = u->parent;
    }
}

static void delete_fixup(RBTree *rb, Node *x, Node *x_parent)
{
    Node *w;
    while (x != rb->root && (x == NULL || x->color == BLACK)) {
        if (x == x_parent->left) {
            w = x_parent->right;
            if (w->color == RED) {
                w->color        = BLACK;
                x_parent->color = RED;
                left_rotate(rb, x_parent);
                w = x_parent->right;
            }
            if ((w->left == NULL || w->left->color == BLACK) &&
                (w->right == NULL || w->right->color == BLACK)) {
                w->color = RED;
                x        = x_parent;
                x_parent = x_parent->parent;
            }
            else {
                if (w->right == NULL || w->right->color == BLACK) {
                    if (w->left != NULL) {
                        w->left->color = BLACK;
                    }
                    w->color = RED;
                    right_rotate(rb, w);
                    w = x_parent->right;
                }
                w->color        = x_parent->color;
                x_parent->color = BLACK;
                if (w->right != NULL) {
                    w->right->color = BLACK;
                }
                left_rotate(rb, x_parent);
                x = rb->root;
                break;
            }
        }
        else {
            w = x_parent->left;
            if (w->color == RED) {
                w->color        = BLACK;
                x_parent->color = RED;
                right_rotate(rb, x_parent);
                w = x_parent->left;
            }
            if ((w->left == NULL || w->left->color == BLACK) &&
                (w->right == NULL || w->right->color == BLACK)) {
                w->color = RED;
                x        = x_parent;
                x_parent = x_parent->parent;
            }
            else {
                if (w->left == NULL || w->left->color == BLACK) {
                    if (w->right != NULL) {
                        w->right->color = BLACK;
                    }
                    w->color = RED;
                    left_rotate(rb, w);
                    w = x_parent->left;
                }
                w->color        = x_parent->color;
                x_parent->color = BLACK;
                if (w->left != NULL) {
                    w->left->color = BLACK;
                }
                right_rotate(rb, x_parent);
                x = rb->root;
                break;
            }
        }
    }
    if (x != NULL) {
        x->color = BLACK;
    }
}

void rb_delete(RBTree *rb, int key)
{
    Node *z = search(rb->root, key);
    if (z == NULL) {
        return;
    }
    Node *x, *y = z;
    Color y_original_color = y->color;
    if (z->left == NULL) {
        x = z->right;
        transplant(rb, z, z->right);
    }
    else if (z->right == NULL) {
        x = z->left;
        transplant(rb, z, z->left);
    }
    else {
        y                = minimum(z->right);
        y_original_color = y->color;
        x                = y->right;
        if (y->parent == z) {
            if (x != NULL) {
                x->parent = y;
            }
        }
        else {
            transplant(rb, y, y->right);
            y->right         = z->right;
            y->right->parent = y;
        }
        transplant(rb, z, y);
        y->left         = z->left;
        y->left->parent = y;
        y->color        = z->color;
    }
    if (y_original_color == BLACK) {
        delete_fixup(rb, x, y->parent);
    }
    g_kfree(z);
}

struct RBTree *rb_new(void)
{
    return (RBTree *)g_kcalloc(1, sizeof(RBTree));
}

bool rb_find(const struct RBTree *rb, int key, uint64_t *value)
{
    const Node *node = csearch(rb->root, key);

    if (!node) {
        return false;
    }
    if (value) {
        *value = node->value;
    }
    return true;
}

static void rbfree_node(Node *node)
{
    if (node != NULL) {
        rbfree_node(node->right);
        rbfree_node(node->left);
        g_kfree(node);
    }
}

void rb_clear(struct RBTree *rb)
{
    rbfree_node(rb->root);
    rb->root = NULL;
}

void rb_free(struct RBTree *rb)
{
    rbfree_node(rb->root);
    g_kfree(rb);
}
