#include "rb_tree.h"
#include <string.h> // memcpy

/**
 * 初始化红黑树
 */
void rb_tree_init(rb_tree_t *tree, int capacity, uintptr_t compare, uintptr_t get_node, uintptr_t get_parent)
{
    tree->capacity = capacity;
    tree->count = 0;
    tree->compare = (int (*)(const void *, const void *))compare;
    tree->get_node = (rb_node_t * (*)(const void *)) get_node;
    tree->get_parent = (void *(*)(rb_node_t *))get_parent;

    // **初始化 `tree->nil`**
    static rb_node_t nil_node = {NULL, NULL, NULL, BLACK, 0};
    tree->nil = &nil_node;
    tree->root = tree->nil;
}

/**
 * 左旋
 */
static void rb_tree_left_rotate(rb_tree_t *tree, rb_node_t *x)
{
    rb_node_t *y = x->right;
    x->right = y->left;
    if (y->left)
        y->left->parent = x;
    y->parent = x->parent;
    if (!x->parent)
        tree->root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;
    y->left = x;
    x->parent = y;
}

/**
 * 右旋
 */
static void rb_tree_right_rotate(rb_tree_t *tree, rb_node_t *y)
{
    rb_node_t *x = y->left;
    y->left = x->right;
    if (x->right)
        x->right->parent = y;
    x->parent = y->parent;
    if (!y->parent)
        tree->root = x;
    else if (y == y->parent->right)
        y->parent->right = x;
    else
        y->parent->left = x;
    x->right = y;
    y->parent = x;
}

/**
 * 插入修复
 */
static void rb_tree_insert_fix(rb_tree_t *tree, rb_node_t *z)
{
    while (z->parent && z->parent->color == RED)
    {
        if (z->parent == z->parent->parent->left)
        {
            rb_node_t *y = z->parent->parent->right;
            if (y && y->color == RED)
            {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            }
            else
            {
                if (z == z->parent->right)
                {
                    z = z->parent;
                    rb_tree_left_rotate(tree, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                rb_tree_right_rotate(tree, z->parent->parent);
            }
        }
        else
        {
            rb_node_t *y = z->parent->parent->left;
            if (y && y->color == RED)
            {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            }
            else
            {
                if (z == z->parent->left)
                {
                    z = z->parent;
                    rb_tree_right_rotate(tree, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                rb_tree_left_rotate(tree, z->parent->parent);
            }
        }
    }
    tree->root->color = BLACK;
}

/**
 * 插入元素
 */
int rb_tree_insert(rb_tree_t *tree, void *data)
{
    rb_node_t *node = tree->get_node(data);
    rb_node_t *y = NULL;
    rb_node_t *x = tree->root;

    // 找到插入位置
    while (x)
    {
        y = x;
        int cmp = tree->compare(data, tree->get_parent(x));
        if (cmp == 0)
        {
            x->count++; // 相同元素，增加计数
            return 1;
        }
        x = (cmp < 0) ? x->left : x->right;
    }

    // 连接到树
    node->parent = y;
    node->left = node->right = NULL;
    node->color = RED;
    node->count = 1;

    if (!y)
        tree->root = node;
    else
    {
        if (tree->compare(data, tree->get_parent(y)) < 0)
            y->left = node;
        else
            y->right = node;
    }

    // 修复树
    rb_tree_insert_fix(tree, node);
    tree->count++;
    return 1;
}

/**
 * 中序遍历
 */
void rb_tree_inorder(rb_tree_t *tree, rb_node_t *node, void (*print)(const void *))
{
    if (node == NULL || node == tree->nil)
        return; // **确保 `node` 有效**

    rb_tree_inorder(tree, node->left, print);
    print(tree->get_parent(node));
    rb_tree_inorder(tree, node->right, print);
}

rb_node_t *rb_tree_find(rb_tree_t *tree, const void *data)
{
    rb_node_t *x = tree->root;

    while (x && x != tree->nil)
    {
        int cmp = tree->compare(data, tree->get_parent(x));
        if (cmp == 0)
        {
            return x;
        }
        x = (cmp < 0) ? x->left : x->right;
    }
    return NULL; // 未找到
}
static rb_node_t *rb_tree_minimum(rb_tree_t *tree, rb_node_t *node)
{
    if (node == NULL)
        return tree->nil; // **确保 `node` 有效**

    while (node->left != NULL && node->left != tree->nil)
    { // **防止访问 NULL**
        node = node->left;
    }
    return node;
}

static void rb_tree_remove_fix(rb_tree_t *tree, rb_node_t *x)
{
    while (x != tree->root && x->color == BLACK)
    {
        if (!x->parent)
            break; // 防止 x->parent 访问 NULL

        if (x == x->parent->left)
        {
            rb_node_t *w = x->parent->right;
            if (!w)
                w = tree->nil;
            if (w->color == RED)
            {
                w->color = BLACK;
                x->parent->color = RED;
                rb_tree_left_rotate(tree, x->parent);
                w = x->parent->right;
            }

            if ((!w->left || w->left->color == BLACK) &&
                (!w->right || w->right->color == BLACK))
            {
                w->color = RED;
                x = x->parent;
            }
            else
            {
                if (!w->right || w->right->color == BLACK)
                {
                    if (w->left)
                        w->left->color = BLACK;
                    w->color = RED;
                    rb_tree_right_rotate(tree, w);
                    w = x->parent->right;
                }

                w->color = x->parent->color;
                x->parent->color = BLACK;
                if (w->right)
                    w->right->color = BLACK;
                rb_tree_left_rotate(tree, x->parent);
                x = tree->root;
            }
        }
        else
        {
            rb_node_t *w = x->parent->left;
            if (!w)
                w = tree->nil; // **确保 `w` 有效**

            if (w->color == RED)
            {
                w->color = BLACK;
                x->parent->color = RED;
                rb_tree_right_rotate(tree, x->parent);
                w = x->parent->left;
                if (!w)
                    w = tree->nil; // **再次确保 `w` 有效**
            }

            if ((w->left == NULL || w->left->color == BLACK) &&
                (w->right == NULL || w->right->color == BLACK))
            {
                w->color = RED;
                x = x->parent;
            }
            else
            {
                if (w->left == NULL || w->left->color == BLACK)
                {
                    if (w->right)
                        w->right->color = BLACK;
                    w->color = RED;
                    rb_tree_left_rotate(tree, w);
                    w = x->parent->left;
                }

                w->color = x->parent->color;
                x->parent->color = BLACK;
                if (w->left)
                    w->left->color = BLACK;
                rb_tree_right_rotate(tree, x->parent);
                x = tree->root;
            }
        }
    }
    if (x)
        x->color = BLACK;
}

int rb_tree_remove(rb_tree_t *tree, void *data)
{
    rb_node_t *z = rb_tree_find(tree, data);
    if (!z)
        return 0;

    if (z->count > 1)
    {
        z->count--;
        return 1;
    }

    rb_node_t *y = z;
    rb_node_t *x;
    rb_color_t y_original_color = y->color;

    if (z->left == tree->nil)
    {
        x = z->right;
        if (z->parent)
        {
            if (z == z->parent->left)
                z->parent->left = x;
            else
                z->parent->right = x;
        }
        else
        {
            tree->root = x;
        }
        if (x)
            x->parent = z->parent;
    }
    else if (z->right == tree->nil)
    {
        x = z->left;
        if (z->parent)
        {
            if (z == z->parent->left)
                z->parent->left = x;
            else
                z->parent->right = x;
        }
        else
        {
            tree->root = x;
        }
        if (x)
            x->parent = z->parent;
    }
    else
    {
        y = rb_tree_minimum(tree, z->right);
        y_original_color = y->color;
        x = y->right;

        if (y->parent == z)
        {
            if (x)
                x->parent = y;
        }
        else
        {
            if (y->parent)
            {
                if (y == y->parent->left)
                    y->parent->left = x;
                else
                    y->parent->right = x;
            }
            if (x)
                x->parent = y->parent;
            y->right = z->right;
            if (y->right)
                y->right->parent = y;
        }

        if (z->parent)
        {
            if (z == z->parent->left)
                z->parent->left = y;
            else
                z->parent->right = y;
        }
        else
        {
            tree->root = y;
        }

        y->parent = z->parent;
        y->left = z->left;
        if (y->left)
            y->left->parent = y;
        y->color = z->color;
    }

    if (!x)
        x = tree->nil;

    if (y_original_color == BLACK)
    {
        rb_tree_remove_fix(tree, x);
    }

    if (tree->root == z)
    {
        tree->root = x;
    }

    tree->count--;
    return 1;
}

// 递归清除节点
static void rb_tree_clear_recursive(rb_tree_t *tree, rb_node_t *node)
{
    if (node == NULL || node == tree->nil)
        return;

    rb_tree_clear_recursive(tree, node->left);
    rb_tree_clear_recursive(tree, node->right);

    // 释放节点（如果你的程序动态分配了节点）
    node->left = node->right = NULL;
    node->parent = NULL;
}

// 清空红黑树
void rb_tree_clear(rb_tree_t *tree)
{
    if (!tree || !tree->root || tree->root == tree->nil)
        return;

    rb_tree_clear_recursive(tree, tree->root);

    tree->root = tree->nil;
    tree->count = 0;
}

/**
 * 根据key查找红黑树中的节点
 * @param tree 红黑树
 * @param key 查找的键 必须是键，不可以是任意类型
 * @param compare_key 自定义比较函数，格式 `int compare_key(const void *key, const void *data)`
 * @return 找到的 `rb_node_t *`，若找不到返回 `NULL`
 */
rb_node_t *rb_tree_find_by(rb_tree_t *tree, const void *key, int (*compare_key)(const void *, const void *))
{
    if (!tree || !tree->root || tree->root == tree->nil)
    {
        return NULL; // 空树，直接返回 NULL
    }

    rb_node_t *x = tree->root;

    while (x != tree->nil)
    {
        void *node_data = tree->get_parent(x); // 获取节点数据

        // 调试输出，检查数据是否正确
        // printf("Comparing key=%p with node_data=%p\n", key, node_data);

        int cmp = compare_key(key, node_data); // 使用自定义比较函数

        if (cmp == 0)
        {
            return x; // 找到匹配的节点
        }
        else if (cmp < 0)
        {
            x = x->left; // 在左子树中查找
        }
        else
        {
            x = x->right; // 在右子树中查找
        }
    }

    return NULL; // 未找到
}
