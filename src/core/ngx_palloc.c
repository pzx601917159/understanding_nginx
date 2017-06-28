
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>

//这几个函数为static,不能在文件外部使用
static ngx_inline void *ngx_palloc_small(ngx_pool_t *pool, size_t size,
    ngx_uint_t align);//分配小内存
static void *ngx_palloc_block(ngx_pool_t *pool, size_t size);//分配块
static void *ngx_palloc_large(ngx_pool_t *pool, size_t size);//分配大块内存

//创建内存池
ngx_pool_t * ngx_create_pool(size_t size, ngx_log_t *log)
{
    ngx_pool_t  *p;

    //分配内存
    p = ngx_memalign(NGX_POOL_ALIGNMENT, size, log);
    if (p == NULL) {
        return NULL;
    }

    //根据分配的内存初始化ngx_pool_t
    p->d.last = (u_char *) p + sizeof(ngx_pool_t);//可用内存的起始位置
    p->d.end = (u_char *) p + size;//可用内存的结束位置
    p->d.next = NULL;   //next初始化为null
    p->d.failed = 0;    //失败次数初始化为0

    size = size - sizeof(ngx_pool_t);//可用内存的大小
    //可以使用的内存大小
    p->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;

    p->current = p;//当前的pool就指向刚刚分配的pool
    p->chain = NULL;//初始化为null
    p->large = NULL;
    p->cleanup = NULL;
    p->log = log;

    return p;
}

//销毁内存池
void
ngx_destroy_pool(ngx_pool_t *pool)
{
    ngx_pool_t          *p, *n;
    ngx_pool_large_t    *l;
    ngx_pool_cleanup_t  *c;
    //调用所有的clean函数
    for (c = pool->cleanup; c; c = c->next) {
        if (c->handler) {
            ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
                           "run cleanup: %p", c);
            c->handler(c->data);
        }
    }

#if (NGX_DEBUG)

    /*
     * we could allocate the pool->log from this pool
     * so we cannot use this log while free()ing the pool
     */

    for (l = pool->large; l; l = l->next) {
        ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0, "free: %p", l->alloc);
    }

    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
                       "free: %p, unused: %uz", p, p->d.end - p->d.last);

        if (n == NULL) {
            break;
        }
    }

#endif
    //释放大块内存
    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            ngx_free(l->alloc);
        }
    }
    //释放pool的内存
    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        ngx_free(p);

        if (n == NULL) {
            break;
        }
    }
}

//重置pool
void
ngx_reset_pool(ngx_pool_t *pool)
{
    ngx_pool_t        *p;
    ngx_pool_large_t  *l;
    //释放大块的内存
    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            ngx_free(l->alloc);
        }
    }
    //重置pool的last指针和failed次数
    for (p = pool; p; p = p->d.next) {
        p->d.last = (u_char *) p + sizeof(ngx_pool_t);
        p->d.failed = 0;
    }
    //重置当前pool指针
    pool->current = pool;
    pool->chain = NULL;//重置chain为null
    pool->large = NULL;//重置large为null
}

//分配内存，默认进行内存对齐
void *
ngx_palloc(ngx_pool_t *pool, size_t size)
{
#if !(NGX_DEBUG_PALLOC)
    if (size <= pool->max) {//如果size比pool的max小，则从pool中分配
        return ngx_palloc_small(pool, size, 1);
    }
#endif
    //否则，调用分配大块内存的函数
    return ngx_palloc_large(pool, size);
}

//有个n,和ngx_palloc唯一的区别就是不进行内存对齐
void *
ngx_pnalloc(ngx_pool_t *pool, size_t size)
{
#if !(NGX_DEBUG_PALLOC)
    if (size <= pool->max) {
        return ngx_palloc_small(pool, size, 0);
    }
#endif

    return ngx_palloc_large(pool, size);
}

//分配小块的内存
static ngx_inline void *
ngx_palloc_small(ngx_pool_t *pool, size_t size, ngx_uint_t align)
{
    u_char      *m;
    ngx_pool_t  *p;

    p = pool->current;
    //如果当前pool没有足够内存就到下一个pool
    do {
        m = p->d.last;

        if (align) {
            m = ngx_align_ptr(m, NGX_ALIGNMENT);//进行内存对齐
        }
        //判断pool剩余的内存大小是否足够
        if ((size_t) (p->d.end - m) >= size) {
            p->d.last = m + size;

            return m;
        }

        p = p->d.next;

    } while (p);
    //如果在pool中没有分配到内存就直接调用ngx_palloc_block,其实就是新建了一个pool
    return ngx_palloc_block(pool, size);
}

//分配内存，这里新建了一个pool
static void *
ngx_palloc_block(ngx_pool_t *pool, size_t size)
{
    u_char      *m;
    size_t       psize;
    ngx_pool_t  *p, *new;
    //获取之前内存池的大小
    psize = (size_t) (pool->d.end - (u_char *) pool);
    //分配内存
    m = ngx_memalign(NGX_POOL_ALIGNMENT, psize, pool->log);
    if (m == NULL) {
        return NULL;
    }
    //转换为ngx_pool结构体,转换成ngx_pool_t的作用是为了用current指针记录,来和第一个pool统一(后面的pool其实都不需要d后面的成员)
    new = (ngx_pool_t *) m;//虽然这里时转换成了一个pool指针，但是结构体实际上只是用了sizeof(ngx_pool_data_t)大小的内存
    //初始化成员
    new->d.end = m + psize;
    new->d.next = NULL;
    new->d.failed = 0;

    m += sizeof(ngx_pool_data_t);//注意这里时+sizeof(ngx_pool_data_t)，而不是ngx_pool_data_t
    m = ngx_align_ptr(m, NGX_ALIGNMENT);
    new->d.last = m + size;
    //判断当前pool分配失败的次数，如果大于则更新current指针
    for (p = pool->current; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) {
            pool->current = p->d.next;
        }
    }
    //插入data链表
    p->d.next = new;

    return m;
}

//分配大块内存
static void *
ngx_palloc_large(ngx_pool_t *pool, size_t size)
{
    void              *p;
    ngx_uint_t         n;
    ngx_pool_large_t  *large;
    //其实就是直接调用malloc函数
    p = ngx_alloc(size, pool->log);
    if (p == NULL) {
        return NULL;
    }

    n = 0;
    //查找空余的large指针，如果没有使用就直接赋值返回
    for (large = pool->large; large; large = large->next) {
        if (large->alloc == NULL) {
            large->alloc = p;
            return p;
        }
        //最大查找三次
        if (n++ > 3) {
            break;
        }
    }
    //分配内存初始化large结构体
    large = ngx_palloc_small(pool, sizeof(ngx_pool_large_t), 1);
    if (large == NULL) {
        ngx_free(p);
        return NULL;
    }
    //放在链表头
    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}

//内存对齐分配内存
void *
ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment)
{
    void              *p;
    ngx_pool_large_t  *large;
    //内存对齐分配内存
    p = ngx_memalign(alignment, size, pool->log);
    if (p == NULL) {
        return NULL;
    }
    //为large结构体分配内存
    large = ngx_palloc_small(pool, sizeof(ngx_pool_large_t), 1);
    if (large == NULL) {
        ngx_free(p);
        return NULL;
    }
    //初始化large,分配的内存指针保存在pool->large中，这里分配的内存有可能会释放，所以有上面查找large指针指向的数据为null的操作
    large->alloc = p;
    large->next = pool->large;
    pool->large = large;
    //返回分配的内存
    return p;
}

//释放内存,就在这里并没有释放large
ngx_int_t
ngx_pfree(ngx_pool_t *pool, void *p)
{
    ngx_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (p == l->alloc) {
            ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
                           "free: %p", l->alloc);
            ngx_free(l->alloc); //释放内存
            l->alloc = NULL;    //large的数据指向null

            return NGX_OK;
        }
    }

    return NGX_DECLINED;
}

//内存对齐分配内存并初始化为0
void *
ngx_pcalloc(ngx_pool_t *pool, size_t size)
{
    void *p;

    p = ngx_palloc(pool, size);
    if (p) {
        ngx_memzero(p, size);
    }

    return p;
}

//增加空的清理资源回调函数
ngx_pool_cleanup_t *
ngx_pool_cleanup_add(ngx_pool_t *p, size_t size)
{
    ngx_pool_cleanup_t  *c;

    c = ngx_palloc(p, sizeof(ngx_pool_cleanup_t));
    if (c == NULL) {
        return NULL;
    }

    if (size) {
        c->data = ngx_palloc(p, size);
        if (c->data == NULL) {
            return NULL;
        }

    } else {
        c->data = NULL;
    }

    c->handler = NULL;
    c->next = p->cleanup;

    p->cleanup = c;

    ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, p->log, 0, "add cleanup: %p", c);

    return c;
}

//清理fd对应的文件资源
void
ngx_pool_run_cleanup_file(ngx_pool_t *p, ngx_fd_t fd)
{
    ngx_pool_cleanup_t       *c;
    ngx_pool_cleanup_file_t  *cf;

    for (c = p->cleanup; c; c = c->next) {
        if (c->handler == ngx_pool_cleanup_file) {

            cf = c->data;

            if (cf->fd == fd) {
                c->handler(cf);
                c->handler = NULL;
                return;
            }
        }
    }
}

//关闭文件描述符
void
ngx_pool_cleanup_file(void *data)
{
    ngx_pool_cleanup_file_t  *c = data;

    ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, c->log, 0, "file cleanup: fd:%d",
                   c->fd);

    if (ngx_close_file(c->fd) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, c->log, ngx_errno,
                      ngx_close_file_n " \"%s\" failed", c->name);
    }
}

//删除文件,关闭文件描述符
void
ngx_pool_delete_file(void *data)
{
    ngx_pool_cleanup_file_t  *c = data;

    ngx_err_t  err;

    ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, c->log, 0, "file cleanup: fd:%d %s",
                   c->fd, c->name);

    if (ngx_delete_file(c->name) == NGX_FILE_ERROR) {
        err = ngx_errno;

        if (err != NGX_ENOENT) {
            ngx_log_error(NGX_LOG_CRIT, c->log, err,
                          ngx_delete_file_n " \"%s\" failed", c->name);
        }
    }

    if (ngx_close_file(c->fd) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, c->log, ngx_errno,
                      ngx_close_file_n " \"%s\" failed", c->name);
    }
}


#if 0

static void *
ngx_get_cached_block(size_t size)
{
    void                     *p;
    ngx_cached_block_slot_t  *slot;

    if (ngx_cycle->cache == NULL) {
        return NULL;
    }

    slot = &ngx_cycle->cache[(size + ngx_pagesize - 1) / ngx_pagesize];

    slot->tries++;

    if (slot->number) {
        p = slot->block;
        slot->block = slot->block->next;
        slot->number--;
        return p;
    }

    return NULL;
}

#endif
