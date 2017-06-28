
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


u_char  ngx_linux_kern_ostype[50];
u_char  ngx_linux_kern_osrelease[50];


static ngx_os_io_t ngx_linux_io = {
    ngx_unix_recv,
    ngx_readv_chain,
    ngx_udp_unix_recv,
    ngx_unix_send,
    ngx_udp_unix_send,
#if (NGX_HAVE_SENDFILE)
    ngx_linux_sendfile_chain,
    NGX_IO_SENDFILE
#else
    ngx_writev_chain,
    0
#endif
};

//linux操作系统下的初始化
ngx_int_t
ngx_os_specific_init(ngx_log_t *log)
{
    struct utsname  u;
    //获取系统用的信息
    if (uname(&u) == -1) {
        ngx_log_error(NGX_LOG_ALERT, log, ngx_errno, "uname() failed");
        return NGX_ERROR;
    }
    //ostype
    (void) ngx_cpystrn(ngx_linux_kern_ostype, (u_char *) u.sysname,
                       sizeof(ngx_linux_kern_ostype));
    //osrelease
    (void) ngx_cpystrn(ngx_linux_kern_osrelease, (u_char *) u.release,
                       sizeof(ngx_linux_kern_osrelease));
    //os_io
    ngx_os_io = ngx_linux_io;

    return NGX_OK;
}


void
ngx_os_specific_status(ngx_log_t *log)
{
    ngx_log_error(NGX_LOG_NOTICE, log, 0, "OS: %s %s",
                  ngx_linux_kern_ostype, ngx_linux_kern_osrelease);
}
