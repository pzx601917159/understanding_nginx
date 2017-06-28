
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_PALLOC_H_INCLUDED_
#define _NGX_PALLOC_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


/*
 * NGX_MAX_ALLOC_FROM_POOL should be (ngx_pagesize - 1), i.e. 4095 on x86.
 * On Windows NT it decreases a number of locked pages in a kernel.
 */
#define NGX_MAX_ALLOC_FROM_POOL  (ngx_pagesize - 1) //从pool中最大分配的大小

#define NGX_DEFAULT_POOL_SIZE    (16 * 1024)//默认的pool的大小为16k

#define NGX_POOL_ALIGNMENT       16//pool对齐大小
#define NGX_MIN_POOL_SIZE   \
    ngx_align((sizeof(ngx_pool_t) + 2 * sizeof(ngx_pool_large_t)),  \
              NGX_POOL_ALIGNMENT)       //最小的大小为ngx_pool_t结构体本身的大小+两个ngx_pool_large_s结构体的大小，且为16的整数倍


typedef void (*ngx_pool_cleanup_pt)(void *data);//清理ngx_pool资源的函数

typedef struct ngx_pool_cleanup_s  ngx_pool_cleanup_t; //清理资源的结构体

struct ngx_pool_cleanup_s {
    ngx_pool_cleanup_pt   handler;  //清理资源的函数
    void                 *data;     //需要清理的数据 
    ngx_pool_cleanup_t   *next;     //下一个清理结构体指针
};


typedef struct ngx_pool_large_s  ngx_pool_large_t;

struct ngx_pool_large_s {
    ngx_pool_large_t     *next;     //指向下个结构体
    void                 *alloc;    //分配的内存
};


typedef struct {
    u_char               *last;     //指向可以分配内存的初始位置
    u_char               *end;      //指想可用内存的结束位置
    ngx_pool_t           *next;     //指向下一个pool
    ngx_uint_t            failed;   //失败次数
} ngx_pool_data_t;


struct ngx_pool_s {
    ngx_pool_data_t       d;        //内存节点的相关数据
    size_t                max;      //最大内存
    ngx_pool_t           *current;  //当前的pool
    ngx_chain_t          *chain;    //就是个ngx_buf
    ngx_pool_large_t     *large;    //pool_large指针
    ngx_pool_cleanup_t   *cleanup;  //清理资源
    ngx_log_t            *log;      //日志
};


typedef struct {
    ngx_fd_t              fd;//文件描述符
    u_char               *name;//文件名
    ngx_log_t            *log;//日志
} ngx_pool_cleanup_file_t;//文件的清理结构体


void *ngx_alloc(size_t size, ngx_log_t *log);//分配内存
void *ngx_calloc(size_t size, ngx_log_t *log);//分配内存，并置0

ngx_pool_t *ngx_create_pool(size_t size, ngx_log_t *log);//创建内存池的函数
void ngx_destroy_pool(ngx_pool_t *pool);//销毁内存池
void ngx_reset_pool(ngx_pool_t *pool);//重置内存池

void *ngx_palloc(ngx_pool_t *pool, size_t size);//从pool中分配内存
void *ngx_pnalloc(ngx_pool_t *pool, size_t size);
void *ngx_pcalloc(ngx_pool_t *pool, size_t size);
void *ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment);//分配内存，内存对齐
ngx_int_t ngx_pfree(ngx_pool_t *pool, void *p);//释放内存


ngx_pool_cleanup_t *ngx_pool_cleanup_add(ngx_pool_t *p, size_t size);//增减清理函数
void ngx_pool_run_cleanup_file(ngx_pool_t *p, ngx_fd_t fd);//执行清理文件资源函数
void ngx_pool_cleanup_file(void *data);//清理文件资源
void ngx_pool_delete_file(void *data);//删除文件


#endif /* _NGX_PALLOC_H_INCLUDED_ */
