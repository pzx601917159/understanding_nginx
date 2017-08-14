
/*
 * Copyright (C) Ruslan Ermilov
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_RWLOCK_H_INCLUDED_
#define _NGX_RWLOCK_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>

//ngx读写锁
//加读锁
void ngx_rwlock_wlock(ngx_atomic_t *lock);
//加写锁
void ngx_rwlock_rlock(ngx_atomic_t *lock);
//解锁
void ngx_rwlock_unlock(ngx_atomic_t *lock);


#endif /* _NGX_RWLOCK_H_INCLUDED_ */
