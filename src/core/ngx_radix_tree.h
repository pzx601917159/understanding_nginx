
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_RADIX_TREE_H_INCLUDED_
#define _NGX_RADIX_TREE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


#define NGX_RADIX_NO_VALUE   (uintptr_t) -1

typedef struct ngx_radix_node_s  ngx_radix_node_t;

struct ngx_radix_node_s {
    ngx_radix_node_t  *right;//左
    ngx_radix_node_t  *left;//右
    ngx_radix_node_t  *parent;//父
    uintptr_t          value;//值
};


typedef struct {
    ngx_radix_node_t  *root;//根
    ngx_pool_t        *pool;//内存池
    ngx_radix_node_t  *free;//
    char              *start;
    size_t             size;//大小
} ngx_radix_tree_t;

//创建radix_tree
ngx_radix_tree_t *ngx_radix_tree_create(ngx_pool_t *pool,
    ngx_int_t preallocate);
//插入
ngx_int_t ngx_radix32tree_insert(ngx_radix_tree_t *tree,
    uint32_t key, uint32_t mask, uintptr_t value);
//删除
ngx_int_t ngx_radix32tree_delete(ngx_radix_tree_t *tree,
    uint32_t key, uint32_t mask);
//查找
uintptr_t ngx_radix32tree_find(ngx_radix_tree_t *tree, uint32_t key);

#if (NGX_HAVE_INET6)
//128位的插入
ngx_int_t ngx_radix128tree_insert(ngx_radix_tree_t *tree,
    u_char *key, u_char *mask, uintptr_t value);
//128位的删除
ngx_int_t ngx_radix128tree_delete(ngx_radix_tree_t *tree,
    u_char *key, u_char *mask);
//128位的查找
uintptr_t ngx_radix128tree_find(ngx_radix_tree_t *tree, u_char *key);
#endif


#endif /* _NGX_RADIX_TREE_H_INCLUDED_ */
