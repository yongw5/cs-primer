# signal 事件
libevent 将 signal 事件转换为 io 事件处理，将 signal 和 io 事件统一。signal 事件的所有接口，都是借用普通 event 事件接口处理。
```
#define evsignal_new(base, signum, cb, arg) \
    event_new(base, signum, EV_SIGNAL|EV_PERSIST, cb, arg)
#define evsignal_add(ev, tv) \
    event_add((ev),(tv))
#define evsignal_del(ev) \
    event_del(ev)
#define evsignal_pending(ev, what, tv_out) \
    event_pending((ev), (what), (tv_out))
#define evsignal_assign(event, base, signum, callback, arg) \
    event_assign(event, base, signum, EV_SIGNAL|EV_PERSIST, callback, arg)
```
其原理是：当信号发生时，信号处理函数向 pipe 写入数据。此时监听 pipe 可读的 event 变成 ACTIVE 状态，其回调函数从 pipe 中读取数据，然后处理。这样就将 signal 事件转换为 io 事件。

## evsig_info
```
typedef void (*ev_sighandler_t)(int);

struct evsig_info {
    struct event ev_signal;    // 关注 ev_signal_pair[1] 可读事件
    evutil_socket_t ev_signal_pair[2];
    int ev_signal_added;       // 是否已经 PENDING
    int ev_n_signals_added;
#ifdef EVENT__HAVE_SIGACTION
    struct sigaction **sh_old; // 缓存替代的 sigaction，用于还原
#else
    // ...
#endif
    int sh_old_max;
};
```

## 信号捕获
调用 evnet_add() 将 signal 事件变成 PENDING 状态，Libevent 为信号 |signo| 设置信号处理函数
```
int 
evsig_set_handler_(struct event_base *base,
    int evsignal, void (__cdecl *handler)(int))
{
#ifdef EVENT__HAVE_SIGACTION
    struct sigaction sa;
#else
    // ...
#endif
    struct evsig_info *sig = &base->sig;
    void *p;

    if (evsignal >= sig->sh_old_max) {
        int new_max = evsignal + 1;
        event_debug(("%s: evsignal (%d) >= sh_old_max (%d), resizing",
                __func__, evsignal, sig->sh_old_max));
        p = mm_realloc(sig->sh_old, new_max * sizeof(*sig->sh_old));
        if (p == NULL) {
            event_warn("realloc");
            return (-1);
        }

        memset((char *)p + sig->sh_old_max * sizeof(*sig->sh_old),
            0, (new_max - sig->sh_old_max) * sizeof(*sig->sh_old));

        sig->sh_old_max = new_max;
        sig->sh_old = p;
    }

    /* allocate space for previous handler out of dynamic array */
    sig->sh_old[evsignal] = mm_malloc(sizeof *sig->sh_old[evsignal]);
    if (sig->sh_old[evsignal] == NULL) {
        event_warn("malloc");
        return (-1);
    }

    // 设置新的 sigaction，并将替换的 sigaction 保存
#ifdef EVENT__HAVE_SIGACTION
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);

    if (sigaction(evsignal, &sa, sig->sh_old[evsignal]) == -1) {
        event_warn("sigaction");
        mm_free(sig->sh_old[evsignal]);
        sig->sh_old[evsignal] = NULL;
        return (-1);
    }
#else
    // ...
#endif

    return (0);
}
```
注册的信号处理函数为：
```
static void __cdecl
evsig_handler(int sig)
{
    int save_errno = errno;
#ifdef _WIN32
    // ..
#endif
    ev_uint8_t msg;

    if (evsig_base == NULL) {
        event_warnx(
            "%s: received signal %d, but have no base configured",
            __func__, sig);
        return;
    }

#ifndef EVENT__HAVE_SIGACTION
    // ...
#endif

    msg = sig;
#ifdef _WIN32
    // ...
#else
    {
        int r = write(evsig_base_fd, (char*)&msg, 1);
        (void)r; 
    }
#endif
    errno = save_errno;
#ifdef _WIN32
    // ...
#endif
}
```
从 evsig_handler() 函数可知：当 signal 发生时，向 evsig_base_fd 写入 signo。

## signal 事件处理
evsig_info::ev_signal 被注册，关注 ev_signal_pair[0] 可读事件。回调函数为 evsig_cb()，其定义如下：
```
static void
evsig_cb(evutil_socket_t fd, short what, void *arg)
{
    static char signals[1024];
    ev_ssize_t n;
    int i;
    int ncaught[NSIG];
    struct event_base *base;

    base = arg;

    memset(&ncaught, 0, sizeof(ncaught));

    while (1) {
#ifdef _WIN32
        // ...
#else
        n = read(fd, signals, sizeof(signals));
#endif
        if (n == -1) {
            int err = evutil_socket_geterror(fd);
            if (! EVUTIL_ERR_RW_RETRIABLE(err))
                event_sock_err(1, fd, "%s: recv", __func__);
            break;
        } else if (n == 0) {
            /* XXX warn? */
            break;
        }
        for (i = 0; i < n; ++i) {
            ev_uint8_t sig = signals[i];
            if (sig < NSIG)
                ncaught[sig]++;
        }
    }

    EVBASE_ACQUIRE_LOCK(base, th_base_lock);
    for (i = 0; i < NSIG; ++i) {
        if (ncaught[i])
            evmap_signal_active_(base, i, ncaught[i]); // => ACTIVE
    }
    EVBASE_RELEASE_LOCK(base, th_base_lock);
}
```
从上面的流程可以看出，libevent 内部注册一个关注 sig.ev_signal_pair[0] 可读的 io 事件，当其可读时，从中读出所有数据（signal 就绪的事件），然后在将对应的 signal 事件激活。