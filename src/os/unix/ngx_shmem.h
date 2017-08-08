
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_SHMEM_H_INCLUDED_
#define _NGX_SHMEM_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct {
    u_char      *addr;  //地址
    size_t       size;  //大小
    ngx_str_t    name;  //名字
    ngx_log_t   *log;   //日志
    ngx_uint_t   exists;   /* unsigned  exists:1;  */
} ngx_shm_t;

//分配
ngx_int_t ngx_shm_alloc(ngx_shm_t *shm);
//释放
void ngx_shm_free(ngx_shm_t *shm);


#endif /* _NGX_SHMEM_H_INCLUDED_ */
