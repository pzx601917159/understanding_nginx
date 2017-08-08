
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_CYCLE_H_INCLUDED_
#define _NGX_CYCLE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


#ifndef NGX_CYCLE_POOL_SIZE
#define NGX_CYCLE_POOL_SIZE     NGX_DEFAULT_POOL_SIZE
#endif


#define NGX_DEBUG_POINTS_STOP   1
#define NGX_DEBUG_POINTS_ABORT  2

//共享内存
typedef struct ngx_shm_zone_s  ngx_shm_zone_t;
//函数指针,共享内存初始化函数
typedef ngx_int_t (*ngx_shm_zone_init_pt) (ngx_shm_zone_t *zone, void *data);

struct ngx_shm_zone_s {
    void                     *data;
    ngx_shm_t                 shm;  //共享内存
    ngx_shm_zone_init_pt      init;
    void                     *tag;
    ngx_uint_t                noreuse;  /* unsigned  noreuse:1; *///复用
};

//全局的cycle
struct ngx_cycle_s {
    void                  ****conf_ctx;//配置文件
    ngx_pool_t               *pool;//内存池

    ngx_log_t                *log;//日志
    ngx_log_t                 new_log;

    ngx_uint_t                log_use_stderr;  /* unsigned  log_use_stderr:1; *///使用标准出错打印日志

    ngx_connection_t        **files;//连接
    ngx_connection_t         *free_connections;//空闲的可用的连接对象
    ngx_uint_t                free_connection_n;//空闲的连接数量

    ngx_module_t            **modules;//module
    ngx_uint_t                modules_n;//module数量
    ngx_uint_t                modules_used;    /* unsigned  modules_used:1; */

    ngx_queue_t               reusable_connections_queue;

    ngx_array_t               listening;//侦听的array
    ngx_array_t               paths;
    ngx_array_t               config_dump;
    ngx_list_t                open_files;//打开的文件
    ngx_list_t                shared_memory;//共享内存

    ngx_uint_t                connection_n;
    ngx_uint_t                files_n;

    ngx_connection_t         *connections;//连接
    ngx_event_t              *read_events;//读事件
    ngx_event_t              *write_events;//写时间

    ngx_cycle_t              *old_cycle;

    ngx_str_t                 conf_file;//配置文件
    ngx_str_t                 conf_param;
    ngx_str_t                 conf_prefix;
    ngx_str_t                 prefix;
    ngx_str_t                 lock_file;
    ngx_str_t                 hostname;
};

//配置相关
typedef struct {
    ngx_flag_t                daemon;//守护进程
    ngx_flag_t                master;//master-salver

    ngx_msec_t                timer_resolution;

    ngx_int_t                 worker_processes;
    ngx_int_t                 debug_points;

    ngx_int_t                 rlimit_nofile;//文件描述符限制
    off_t                     rlimit_core;//core文件大小限制

    int                       priority;

    ngx_uint_t                cpu_affinity_auto;//cpu亲和度相关
    ngx_uint_t                cpu_affinity_n;
    ngx_cpuset_t             *cpu_affinity;

    char                     *username;
    ngx_uid_t                 user;
    ngx_gid_t                 group;

    ngx_str_t                 working_directory;
    ngx_str_t                 lock_file;

    ngx_str_t                 pid;
    ngx_str_t                 oldpid;

    ngx_array_t               env;
    char                    **environment;
} ngx_core_conf_t;


#define ngx_is_init_cycle(cycle)  (cycle->conf_ctx == NULL)

//cycle初始化
ngx_cycle_t *ngx_init_cycle(ngx_cycle_t *old_cycle);
ngx_int_t ngx_create_pidfile(ngx_str_t *name, ngx_log_t *log);
//删除pid文件
void ngx_delete_pidfile(ngx_cycle_t *cycle);
ngx_int_t ngx_signal_process(ngx_cycle_t *cycle, char *sig);
//重新打开文件
void ngx_reopen_files(ngx_cycle_t *cycle, ngx_uid_t user);
//设置环境变量
char **ngx_set_environment(ngx_cycle_t *cycle, ngx_uint_t *last);

ngx_pid_t ngx_exec_new_binary(ngx_cycle_t *cycle, char *const *argv);
//获取cpu亲和度
ngx_cpuset_t *ngx_get_cpu_affinity(ngx_uint_t n);
//增加共享内存
ngx_shm_zone_t *ngx_shared_memory_add(ngx_conf_t *cf, ngx_str_t *name,
    size_t size, void *tag);


extern volatile ngx_cycle_t  *ngx_cycle;
extern ngx_array_t            ngx_old_cycles;
extern ngx_module_t           ngx_core_module;
extern ngx_uint_t             ngx_test_config;
extern ngx_uint_t             ngx_dump_config;
extern ngx_uint_t             ngx_quiet_mode;


#endif /* _NGX_CYCLE_H_INCLUDED_ */
