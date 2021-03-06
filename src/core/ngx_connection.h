
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_CONNECTION_H_INCLUDED_
#define _NGX_CONNECTION_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct ngx_listening_s  ngx_listening_t;
//侦听的结构体
struct ngx_listening_s {
    ngx_socket_t        fd;//套接字

    struct sockaddr    *sockaddr;//addr
    socklen_t           socklen;    /* size of sockaddr */
    size_t              addr_text_max_len;
    ngx_str_t           addr_text;//addr对应的文字信息

    int                 type;

    int                 backlog;//侦听队列的最大长度
    int                 rcvbuf;//接受buf
    int                 sndbuf;//发送buf
#if (NGX_HAVE_KEEPALIVE_TUNABLE)
    int                 keepidle;//keepalive的三个选项，空闲时间
    int                 keepintvl;//间隔时间
    int                 keepcnt;//总次数
#endif

    /* handler of accepted connection */
    ngx_connection_handler_pt   handler;//accept的回调函数

    void               *servers;  /* array of ngx_http_in_addr_t, for example */

    ngx_log_t           log;//日志
    ngx_log_t          *logp;//日志指针

    size_t              pool_size;
    /* should be here because of the AcceptEx() preread */
    size_t              post_accept_buffer_size;
    /* should be here because of the deferred accept */
    ngx_msec_t          post_accept_timeout;

    ngx_listening_t    *previous;//上一个listening_t
    ngx_connection_t   *connection;//连接

    ngx_uint_t          worker;

    unsigned            open:1;
    unsigned            remain:1;
    unsigned            ignore:1;

    unsigned            bound:1;       /* already bound */
    unsigned            inherited:1;   /* inherited from previous process */
    unsigned            nonblocking_accept:1;//非阻塞accept
    unsigned            listen:1;
    unsigned            nonblocking:1;//非阻塞
    unsigned            shared:1;    /* shared between threads or processes */
    unsigned            addr_ntop:1;
    unsigned            wildcard:1;

#if (NGX_HAVE_INET6 && defined IPV6_V6ONLY)
    unsigned            ipv6only:1;
#endif
#if (NGX_HAVE_REUSEPORT)
    unsigned            reuseport:1;//重用端口
    unsigned            add_reuseport:1;
#endif
    unsigned            keepalive:2;//开启keepalive

#if (NGX_HAVE_DEFERRED_ACCEPT)
    unsigned            deferred_accept:1;
    unsigned            delete_deferred:1;
    unsigned            add_deferred:1;
#ifdef SO_ACCEPTFILTER
    char               *accept_filter;
#endif
#endif
#if (NGX_HAVE_SETFIB)
    int                 setfib;
#endif

#if (NGX_HAVE_TCP_FASTOPEN)
    int                 fastopen;
#endif

};

//connection的错误
typedef enum {
    NGX_ERROR_ALERT = 0,
    NGX_ERROR_ERR,
    NGX_ERROR_INFO,
    NGX_ERROR_IGNORE_ECONNRESET,
    NGX_ERROR_IGNORE_EINVAL
} ngx_connection_log_error_e;

//tcp_nodelay选项
typedef enum {
    NGX_TCP_NODELAY_UNSET = 0,
    NGX_TCP_NODELAY_SET,
    NGX_TCP_NODELAY_DISABLED
} ngx_connection_tcp_nodelay_e;

//tcp_nopush选项
typedef enum {
    NGX_TCP_NOPUSH_UNSET = 0,
    NGX_TCP_NOPUSH_SET,
    NGX_TCP_NOPUSH_DISABLED
} ngx_connection_tcp_nopush_e;


#define NGX_LOWLEVEL_BUFFERED  0x0f
#define NGX_SSL_BUFFERED       0x01
#define NGX_HTTP_V2_BUFFERED   0x02

//connection结构体
struct ngx_connection_s {
    void               *data;//数据
    ngx_event_t        *read;//读
    ngx_event_t        *write;//写

    ngx_socket_t        fd;//套接子

    ngx_recv_pt         recv;//接收回调函数
    ngx_send_pt         send;//发送回调函数
    ngx_recv_chain_pt   recv_chain;
    ngx_send_chain_pt   send_chain;

    ngx_listening_t    *listening;

    off_t               sent;

    ngx_log_t          *log;

    ngx_pool_t         *pool;

    int                 type;

    struct sockaddr    *sockaddr;
    socklen_t           socklen;
    ngx_str_t           addr_text;

    ngx_str_t           proxy_protocol_addr;

#if (NGX_SSL)
    ngx_ssl_connection_t  *ssl;
#endif

    struct sockaddr    *local_sockaddr;
    socklen_t           local_socklen;

    ngx_buf_t          *buffer;

    ngx_queue_t         queue;

    ngx_atomic_uint_t   number;

    ngx_uint_t          requests;

    unsigned            buffered:8;

    unsigned            log_error:3;     /* ngx_connection_log_error_e */

    unsigned            unexpected_eof:1;
    unsigned            timedout:1;
    unsigned            error:1;
    unsigned            destroyed:1;

    unsigned            idle:1;
    unsigned            reusable:1;
    unsigned            close:1;
    unsigned            shared:1;

    unsigned            sendfile:1;
    unsigned            sndlowat:1;
    unsigned            tcp_nodelay:2;   /* ngx_connection_tcp_nodelay_e */
    unsigned            tcp_nopush:2;    /* ngx_connection_tcp_nopush_e */

    unsigned            need_last_buf:1;

#if (NGX_HAVE_IOCP)
    unsigned            accept_context_updated:1;
#endif

#if (NGX_HAVE_AIO_SENDFILE)
    unsigned            busy_count:2;
#endif

#if (NGX_THREADS)
    ngx_thread_task_t  *sendfile_task;
#endif
};


#define ngx_set_connection_log(c, l)                                         \
                                                                             \
    c->log->file = l->file;                                                  \
    c->log->next = l->next;                                                  \
    c->log->writer = l->writer;                                              \
    c->log->wdata = l->wdata;                                                \
    if (!(c->log->log_level & NGX_LOG_DEBUG_CONNECTION)) {                   \
        c->log->log_level = l->log_level;                                    \
    }

//创建侦听
ngx_listening_t *ngx_create_listening(ngx_conf_t *cf, void *sockaddr,
    socklen_t socklen);
//拷贝侦听
ngx_int_t ngx_clone_listening(ngx_conf_t *cf, ngx_listening_t *ls);
//设置继承scokets
ngx_int_t ngx_set_inherited_sockets(ngx_cycle_t *cycle);
//打开侦听
ngx_int_t ngx_open_listening_sockets(ngx_cycle_t *cycle);
//配置侦听
void ngx_configure_listening_sockets(ngx_cycle_t *cycle);
//关闭侦听的套接字
void ngx_close_listening_sockets(ngx_cycle_t *cycle);
//关闭连接
void ngx_close_connection(ngx_connection_t *c);
//关闭空闲的连接
void ngx_close_idle_connections(ngx_cycle_t *cycle);
ngx_int_t ngx_connection_local_sockaddr(ngx_connection_t *c, ngx_str_t *s,
    ngx_uint_t port);
//连接出错
ngx_int_t ngx_connection_error(ngx_connection_t *c, ngx_err_t err, char *text);
//获取连接
ngx_connection_t *ngx_get_connection(ngx_socket_t s, ngx_log_t *log);
//释放连接
void ngx_free_connection(ngx_connection_t *c);
//重用连接结构体
void ngx_reusable_connection(ngx_connection_t *c, ngx_uint_t reusable);

#endif /* _NGX_CONNECTION_H_INCLUDED_ */
