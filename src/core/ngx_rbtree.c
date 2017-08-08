
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


/*
 * The red-black tree code is based on the algorithm described in
 * the "Introduction to Algorithms" by Cormen, Leiserson and Rivest.
 */

//左旋
static ngx_inline void ngx_rbtree_left_rotate(ngx_rbtree_node_t **root,
    ngx_rbtree_node_t *sentinel, ngx_rbtree_node_t *node);
static ngx_inline void ngx_rbtree_right_rotate(ngx_rbtree_node_t **root,
    ngx_rbtree_node_t *sentinel, ngx_rbtree_node_t *node);


//插入
void
ngx_rbtree_insert(ngx_rbtree_t *tree, ngx_rbtree_node_t *node)
{
    ngx_rbtree_node_t  **root, *temp, *sentinel;

    /* a binary tree insert */
    //如果要对root赋值，必须要取二级指针
    root = (ngx_rbtree_node_t **) &tree->root;
    sentinel = tree->sentinel;
    //为空的情况
    if (*root == sentinel) {
        node->parent = NULL;
        node->left = sentinel;
        node->right = sentinel;
        ngx_rbt_black(node);
        *root = node;

        return;
    }

    tree->insert(*root, node, sentinel);

    /* re-balance tree */
    //调整红黑树,判断条件插入节点的父节点不为红节点或者不为根节点
    while (node != *root && ngx_rbt_is_red(node->parent)) 
    {
        //为祖父节点的左节点的情况
        if (node->parent == node->parent->parent->left) 
        {
            temp = node->parent->parent->right;
            if (ngx_rbt_is_red(temp)) {
                ngx_rbt_black(node->parent);
                ngx_rbt_black(temp);
                ngx_rbt_red(node->parent->parent);
                node = node->parent->parent;
            } 
            else 
            {
                if (node == node->parent->right) 
                {
                    node = node->parent;
                    ngx_rbtree_left_rotate(root, sentinel, node);
                }

                ngx_rbt_black(node->parent);
                ngx_rbt_red(node->parent->parent);
                ngx_rbtree_right_rotate(root, sentinel, node->parent->parent);
            }

        } 
        //为祖父节点的右节点的情况
        else 
        {           
            temp = node->parent->parent->left;

            if (ngx_rbt_is_red(temp)) {
                ngx_rbt_black(node->parent);
                ngx_rbt_black(temp);
                ngx_rbt_red(node->parent->parent);
                node = node->parent->parent;

            } 
            else 
            {
                if (node == node->parent->left) 
                {
                    node = node->parent;
                    ngx_rbtree_right_rotate(root, sentinel, node);
                }

                ngx_rbt_black(node->parent);
                ngx_rbt_red(node->parent->parent);
                ngx_rbtree_left_rotate(root, sentinel, node->parent->parent);
            }
        }
    }

    ngx_rbt_black(*root);
}

//插入节点
void
ngx_rbtree_insert_value(ngx_rbtree_node_t *temp, ngx_rbtree_node_t *node,
    ngx_rbtree_node_t *sentinel)
{
    ngx_rbtree_node_t  **p;

    for ( ;; ) 
    {
        p = (node->key < temp->key) ? &temp->left : &temp->right;

        if (*p == sentinel) {
            break;
        }

        temp = *p;
    }

    *p = node;
    node->parent = temp;
    node->left = sentinel;
    node->right = sentinel;
    ngx_rbt_red(node);
}


void
ngx_rbtree_insert_timer_value(ngx_rbtree_node_t *temp, ngx_rbtree_node_t *node,
    ngx_rbtree_node_t *sentinel)
{
    ngx_rbtree_node_t  **p;

    for ( ;; ) {

        /*
         * Timer values
         * 1) are spread in small range, usually several minutes,
         * 2) and overflow each 49 days, if milliseconds are stored in 32 bits.
         * The comparison takes into account that overflow.
         */

        /*  node->key < temp->key */

        p = ((ngx_rbtree_key_int_t) (node->key - temp->key) < 0)
            ? &temp->left : &temp->right;

        if (*p == sentinel) {
            break;
        }

        temp = *p;
    }

    *p = node;
    node->parent = temp;
    node->left = sentinel;
    node->right = sentinel;
    ngx_rbt_red(node);
}

//删除,删除的时候同样需要调整红黑树，如果删除的是黑色节点
void
ngx_rbtree_delete(ngx_rbtree_t *tree, ngx_rbtree_node_t *node)
{
    ngx_uint_t           red;
    ngx_rbtree_node_t  **root, *sentinel, *subst, *temp, *w;
    //root根节点 sentinel哨兵空节点 subst替换的节点 temp为替换的新节点  w兄弟节点

    /* a binary tree delete */
    root = (ngx_rbtree_node_t **) &tree->root;  //对root赋值
    sentinel = tree->sentinel;                  //对哨兵节点赋值
    //左子树为空
    if (node->left == sentinel) {
        temp = node->right; //temp为右节点
        subst = node;       //subst为需要删除的节点
    }
    //右子树为空
    else if (node->right == sentinel) 
    {
        temp = node->left;  //temp为做节点
        subst = node;       //subst为需要删除的节点
    } 
    else 
    {
        //取右子树的最小的节点
        subst = ngx_rbtree_min(node->right, sentinel);//用来替换node

        if (subst->left != sentinel) {
            temp = subst->left;
        } else {
            temp = subst->right;
        }
    }
    
    //需要删除的节点为根节点的情况
    if (subst == *root) {
        *root = temp;
        ngx_rbt_black(temp);

        /* DEBUG stuff */
        node->left = NULL;
        node->right = NULL;
        node->parent = NULL;
        node->key = 0;

        return;
    }
    //获取删除节点的颜色
    red = ngx_rbt_is_red(subst);

    //替换节点
    if (subst == subst->parent->left) 
    {
        subst->parent->left = temp;
    } 
    else 
    {
        subst->parent->right = temp;
    }
    
    //如果替换的节点就是node节点,新替换节点的parent就是node的父节点
    if (subst == node) 
    {
        temp->parent = subst->parent;
    } 
    else 
    {
        if (subst->parent == node) 
        {
            temp->parent = subst;
        }
        else 
        {
            temp->parent = subst->parent;
        }

        subst->left = node->left;
        subst->right = node->right;
        subst->parent = node->parent;
        ngx_rbt_copy_color(subst, node);

        if (node == *root) 
        {
            *root = subst;
        } 
        else 
        {
            if (node == node->parent->left) 
            {
                node->parent->left = subst;
            }
            else 
            {
                node->parent->right = subst;
            }
        }

        if (subst->left != sentinel) 
        {
            subst->left->parent = subst;
        }

        if (subst->right != sentinel) 
        {
            subst->right->parent = subst;
        }
    }

    /* DEBUG stuff */
    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;
    node->key = 0;
    //如果替换的这个节点为红色的节点并没有破坏红黑数的平衡性，不许要做任何操作了
    if (red) {
        return;
    }

    /* a delete fixup */
    //如果替换的节点为黑色节点则影响了红黑数的平衡
    while (temp != *root && ngx_rbt_is_black(temp)) {
        //分两种情况，替换的节点为父节点的左节点的情况
        if (temp == temp->parent->left) 
        {
            //兄弟节点
            w = temp->parent->right;
            //兄弟节点为红色的情况
            if (ngx_rbt_is_red(w)) 
            {
                ngx_rbt_black(w);
                ngx_rbt_red(temp->parent);
                ngx_rbtree_left_rotate(root, sentinel, temp->parent);
                w = temp->parent->right;
            }
            //兄弟节点的子节点都为黑色的情况
            if (ngx_rbt_is_black(w->left) && ngx_rbt_is_black(w->right)) {
                //减少右子树的黑高
                ngx_rbt_red(w);
                temp = temp->parent;

            } 
            else 
            {
                if (ngx_rbt_is_black(w->right)) 
                {
                    ngx_rbt_black(w->left);
                    ngx_rbt_red(w);
                    ngx_rbtree_right_rotate(root, sentinel, w);
                    w = temp->parent->right;
                }

                ngx_rbt_copy_color(w, temp->parent);
                ngx_rbt_black(temp->parent);
                ngx_rbt_black(w->right);
                ngx_rbtree_left_rotate(root, sentinel, temp->parent);
                temp = *root;
            }

        } 
        else 
        {
            w = temp->parent->left;

            if (ngx_rbt_is_red(w)) 
            {
                ngx_rbt_black(w);
                ngx_rbt_red(temp->parent);
                ngx_rbtree_right_rotate(root, sentinel, temp->parent);
                w = temp->parent->left;
            }

            if (ngx_rbt_is_black(w->left) && ngx_rbt_is_black(w->right)) {
                ngx_rbt_red(w);
                temp = temp->parent;

            } 
            else 
            {
                if (ngx_rbt_is_black(w->left)) 
                {
                    ngx_rbt_black(w->right);
                    ngx_rbt_red(w);
                    ngx_rbtree_left_rotate(root, sentinel, w);
                    w = temp->parent->left;
                }

                ngx_rbt_copy_color(w, temp->parent);
                ngx_rbt_black(temp->parent);
                ngx_rbt_black(w->left);
                ngx_rbtree_right_rotate(root, sentinel, temp->parent);
                temp = *root;
            }
        }
    }

    ngx_rbt_black(temp);
}


static ngx_inline void
ngx_rbtree_left_rotate(ngx_rbtree_node_t **root, ngx_rbtree_node_t *sentinel,
    ngx_rbtree_node_t *node)
{
    ngx_rbtree_node_t  *temp;

    temp = node->right;
    node->right = temp->left;

    if (temp->left != sentinel) {
        temp->left->parent = node;
    }

    temp->parent = node->parent;

    if (node == *root) {
        *root = temp;

    } else if (node == node->parent->left) {
        node->parent->left = temp;

    } else {
        node->parent->right = temp;
    }

    temp->left = node;
    node->parent = temp;
}


static ngx_inline void
ngx_rbtree_right_rotate(ngx_rbtree_node_t **root, ngx_rbtree_node_t *sentinel,
    ngx_rbtree_node_t *node)
{
    ngx_rbtree_node_t  *temp;

    temp = node->left;
    node->left = temp->right;

    if (temp->right != sentinel) {
        temp->right->parent = node;
    }

    temp->parent = node->parent;

    if (node == *root) {
        *root = temp;

    } else if (node == node->parent->right) {
        node->parent->right = temp;

    } else {
        node->parent->left = temp;
    }

    temp->right = node;
    node->parent = temp;
}
