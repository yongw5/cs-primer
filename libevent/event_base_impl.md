# event_base 解密
## event_base 定义
event_base 定义比较复杂，并且不对外公开，其完整定义如下：
```
/// event-internal.h
struct event_base {
    const struct eventop *evsel; // backend 实现
    void *evbase; // backend 数据
    struct event_changelist changelist; // changlelist 数据

    const struct eventop *evsigsel; // 信号 backend
    struct evsig_info sig; // signal 事件管理器

    int virtual_event_count; 
    int virtual_event_count_max;
    int event_count;
    int event_count_max;
    int event_count_active;
    int event_count_active_max;

    int event_gotterm;  // event_base_loopexit() 是否调用
    int event_break;    // event_base_loopbreak() 是否调用
    int event_continue; // event_base_loopcontinue() 是否调用

    int event_running_priority; // 正在处理的 cb 的优先级

    int running_loop; // 是否正在执行 event_loop

    int n_deferreds_queued; // 添加的 deferred_cbs 数量

    struct evcallback_list *activequeues; // ACTIVE 队列
    int nactivequeues;                    // ACTIVE 队列长度
    struct evcallback_list active_later_queue; // ACTIVE_LATER 队列

    struct common_timeout_list **common_timeout_queues;
    int n_common_timeouts;
    int n_common_timeouts_allocated;

    struct event_io_map io; // fd=>event*
    struct event_signal_map sigmap; // signo=>event*
    struct min_heap timeheap; // 时间堆

    struct timeval tv_cache; // 缓存时间
    struct evutil_monotonic_timer monotonic_timer;
    struct timeval tv_clock_diff; // 内部和gettimeofday差值
    time_t last_updated_clock_diff;

#ifndef EVENT__DISABLE_THREAD_SUPPORT
    unsigned long th_owner_id; // 执行 event_loop 线程的 tid
    void *th_base_lock;        // 互斥锁
    void *current_event_cond;  // 条件变量
    int current_event_waiters; // 等待用户数
#endif
    struct event_callback *current_event; // 正在处理的 event

#ifdef _WIN32
    struct event_iocp_port *iocp;
#endif

    enum event_base_config_flag flags;
    struct timeval max_dispatch_time;
    int max_dispatch_callbacks;
    int limit_callbacks_after_prio;

    int is_notify_pending;           // 是否收到唤醒
    evutil_socket_t th_notify_fd[2]; // 用于实现异步唤醒
    struct event th_notify;          // 用于实现异步唤醒
    int (*th_notify_fn)(struct event_base *base); // 异步唤醒 cb

    struct evutil_weakrand_state weakrand_seed; // 随机种子

    LIST_HEAD(once_event_list, event_once) once_events;
};
```

## event_base_new()
event_base_new() 首先调用 event_config_new() 获取一个 event_config 变量，然后使用默认 event_config 调用 event_base_new_with_config() 函数构造一个 event_base 对象。
```
/// event.c
struct event_base *
event_base_new(void)
{
    struct event_base *base = NULL;
    struct event_config *cfg = event_config_new();
    if (cfg) {
        base = event_base_new_with_config(cfg);
        event_config_free(cfg);
    }
    return base;
}
```
event_base_new_with_config() 定义如下：
```
struct event_base *
event_base_new_with_config(const struct event_config *cfg)
{
    int i;
    struct event_base *base;
    int should_check_environment;

#ifndef EVENT__DISABLE_DEBUG_MODE
    event_debug_mode_too_late = 1;
#endif

    if ((base = mm_calloc(1, sizeof(struct event_base))) == NULL) {
        // 内存申请失败，返回 NULL
    }

    if (cfg)
        base->flags = cfg->flags;

    should_check_environment =
        !(cfg && (cfg->flags & EVENT_BASE_FLAG_IGNORE_ENV));

    {
        struct timeval tmp;
        int precise_time =
            cfg && (cfg->flags & EVENT_BASE_FLAG_PRECISE_TIMER);
        int flags;
        if (should_check_environment && !precise_time) {
            precise_time = evutil_getenv_("EVENT_PRECISE_TIMER") != NULL;
            if (precise_time) {
                base->flags |= EVENT_BASE_FLAG_PRECISE_TIMER;
            }
        }
        flags = precise_time ? EV_MONOT_PRECISE : 0;
        evutil_configure_monotonic_time_(&base->monotonic_timer, flags);

        gettime(base, &tmp);
    }

    min_heap_ctor_(&base->timeheap); // 时间堆初始化

    base->sig.ev_signal_pair[0] = -1;
    base->sig.ev_signal_pair[1] = -1;
    base->th_notify_fd[0] = -1;
    base->th_notify_fd[1] = -1;

    TAILQ_INIT(&base->active_later_queue);

    evmap_io_initmap_(&base->io);
    evmap_signal_initmap_(&base->sigmap);
    event_changelist_init_(&base->changelist);

    base->evbase = NULL;

    if (cfg) {
        memcpy(&base->max_dispatch_time,
            &cfg->max_dispatch_interval, sizeof(struct timeval));
        base->limit_callbacks_after_prio =
            cfg->limit_callbacks_after_prio;
    } else {
        base->max_dispatch_time.tv_sec = -1;
        base->limit_callbacks_after_prio = 1;
    }
    if (cfg && cfg->max_dispatch_callbacks >= 0) {
        base->max_dispatch_callbacks = cfg->max_dispatch_callbacks;
    } else {
        base->max_dispatch_callbacks = INT_MAX;
    }
    if (base->max_dispatch_callbacks == INT_MAX &&
        base->max_dispatch_time.tv_sec == -1)
        base->limit_callbacks_after_prio = INT_MAX;

    for (i = 0; eventops[i] && !base->evbase; i++) {
        if (cfg != NULL) {
            // 过滤 backend method 
            if (event_config_is_avoided_method(cfg,
                eventops[i]->name))
                continue;
            if ((eventops[i]->features & cfg->require_features)
                != cfg->require_features)
                continue;
        }

        if (should_check_environment &&
            event_is_method_disabled(eventops[i]->name))
            continue;

        base->evsel = eventops[i];
        // backend 初始化
        base->evbase = base->evsel->init(base);
    }

    if (base->evbase == NULL) {
        // backend 选择或者初始化失败，清理后返回 NULL
    }

    if (evutil_getenv_("EVENT_SHOW_METHOD"))
        event_msgx("libevent using: %s", base->evsel->name);

    // 初始化 ACTIVE 队列
    if (event_base_priority_init(base, 1) < 0) {
        event_base_free(base);
        return NULL;
    }

    /* prepare for threading */

#if !defined(EVENT__DISABLE_THREAD_SUPPORT) && !defined(EVENT__DISABLE_DEBUG_MODE)
    event_debug_created_threadable_ctx_ = 1;
#endif

#ifndef EVENT__DISABLE_THREAD_SUPPORT
    if (EVTHREAD_LOCKING_ENABLED() &&
        (!cfg || !(cfg->flags & EVENT_BASE_FLAG_NOLOCK))) {
        int r;
        EVTHREAD_ALLOC_LOCK(base->th_base_lock, 0);
        EVTHREAD_ALLOC_COND(base->current_event_cond);
        r = evthread_make_base_notifiable(base);
        if (r<0) {
            // 失败，清理后返回 NULL
        }
    }
#endif

#ifdef _WIN32_/*_WIN32*/
    /* ... */
#endif

    return (base);
}
```
## eventop
eventop 定义了所有平台 IO 多路复用公共的接口。
```
/// event-internal.h
struct eventop {
    const char *name;                     // 名字
    void *(*init)(struct event_base *);   // 初始化，event_base::evbase 保存其返回值
    int (*add)(struct event_base *, 
               evutil_socket_t fd, 
               short old, short events, 
               void *fdinfo);             // 注册事件，events 可以是 EV_{READ, WRITE, SIGNAL, ET}
    int (*del)(struct event_base *, 
               evutil_socket_t fd, 
               short old, short events, 
               void *fdinfo);             // 取消或者更改 event
    int (*dispatch)(struct event_base *, 
                    struct timeval *);    // 等待 event 就绪
    void (*dealloc)(struct event_base *); // 清理，释放内存
    int need_reinit;                      // fork() 调用后，是否需要重新初始化
    enum event_method_feature features;   // 支持的特性
    size_t fdinfo_len;                    // TODO
};
```