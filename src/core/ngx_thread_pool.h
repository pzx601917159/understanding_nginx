
/*
 * Copyright (C) Nginx, Inc.
 * Copyright (C) Valentin V. Bartenev
 */


#ifndef _NGX_THREAD_POOL_H_INCLUDED_
#define _NGX_THREAD_POOL_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>


struct ngx_thread_task_s {
    ngx_thread_task_t   *next;//下个节点
    ngx_uint_t           id;//id
    void                *ctx;//上下文
    void               (*handler)(void *data, ngx_log_t *log);//函数指针
    ngx_event_t          event;//事件
};

//线程池
typedef struct ngx_thread_pool_s  ngx_thread_pool_t;

//添加线程
ngx_thread_pool_t *ngx_thread_pool_add(ngx_conf_t *cf, ngx_str_t *name);
//获取线程
ngx_thread_pool_t *ngx_thread_pool_get(ngx_cycle_t *cycle, ngx_str_t *name);
//分配task
ngx_thread_task_t *ngx_thread_task_alloc(ngx_pool_t *pool, size_t size);
//添加task
ngx_int_t ngx_thread_task_post(ngx_thread_pool_t *tp, ngx_thread_task_t *task);


#endif /* _NGX_THREAD_POOL_H_INCLUDED_ */
