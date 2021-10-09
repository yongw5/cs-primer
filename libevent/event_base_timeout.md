# event_base 如何管理 timeout 事件
在 [Lievent 的定时器](./timer.md) 章节中，Libevent 使用 common-timeout 管理大量具有相同超时时长的 timer。
```
/// event-internal.h
struct event_base {
    /// ...
    struct common_timeout_list **common_timeout_queues;
    int n_common_timeouts;
    int n_common_timeouts_allocated;
    /// ...
    struct min_heap timeheap; // 时间堆
    /// ...
    struct timeval tv_cache;  // 缓存时间
    struct evutil_monotonic_timer monotonic_timer;
    struct timeval tv_clock_diff; // clock_monotic和gettimeofday差值
    time_t last_updated_clock_diff; // 更新时间
    /// ...
```

## common_timeout_list
event_base::common_timeout_queues 是一个数组，数组元素是指向 common_timeout_list 的指针。get_common_timeout_list() 函数可以获取一个 common-timeout 对应的 common_timeout_list。
```
static inline struct common_timeout_list *
get_common_timeout_list(struct event_base *base, const struct timeval *tv)
{
    return base->common_timeout_queues[COMMON_TIMEOUT_IDX(tv)];
}
```
common_timeout_list 定义如下：
```
struct common_timeout_list {
    struct event_list events;   // 等待的 event
    struct timeval duration;    // 超时时长（需要解码）
    struct event timeout_event; // 最先超时 event，保存到 min-heap
    struct event_base *base;
};
```
timeout_event 设置的回调函数为 common_timeout_callback() 函数，其定义如下：
```
static void
common_timeout_callback(evutil_socket_t fd, short what, void *arg)
{
    struct timeval now;
    struct common_timeout_list *ctl = arg;
    struct event_base *base = ctl->base;
    struct event *ev = NULL;
    EVBASE_ACQUIRE_LOCK(base, th_base_lock);
    gettime(base, &now);
    while (1) {
        ev = TAILQ_FIRST(&ctl->events);
        if (!ev || ev->ev_timeout.tv_sec > now.tv_sec ||
            (ev->ev_timeout.tv_sec == now.tv_sec &&
                (ev->ev_timeout.tv_usec & MICROSECONDS_MASK) > now.tv_usec))
            break;
        event_del_nolock_(ev, EVENT_DEL_NOBLOCK);
        event_active_nolock_(ev, EV_TIMEOUT, 1);
    }
    if (ev)
        common_timeout_schedule(ctl, &now, ev);
    EVBASE_RELEASE_LOCK(base, th_base_lock);
}
```

## min_heap
min_heap 是最小堆数据结构，用于组织普通的 timeout，其定义如下：
```
typedef struct min_heap
{
    struct event** p;
    unsigned n, a;
} min_heap_t;
```
### min_heap_push_()
```
int min_heap_push_(min_heap_t* s, struct event* e)
{
    if (s->n == UINT32_MAX || min_heap_reserve_(s, s->n + 1))
        return -1;
    min_heap_shift_up_(s, s->n++, e);
    return 0;
}
```
### min_heap_reserve_()

min_heap_reserve_() 函数用于扩张 min_heap 的空间。
```
int min_heap_reserve_(min_heap_t* s, unsigned n)
{
    if (s->a < n)
    {
        struct event** p;
        unsigned a = s->a ? s->a * 2 : 8;
        if (a < n)
            a = n;
#if (SIZE_MAX == UINT32_MAX)
        if (a > SIZE_MAX / sizeof *p)
            return -1;
#endif
        if (!(p = (struct event**)mm_realloc(s->p, a * sizeof *p)))
            return -1;
        s->p = p;
        s->a = a;
    }
    return 0;
}
```

## event_queue_*()
### event_queue_insert_timeout()
event_queue_insert_timeout() 函数将一个 timeout 事件插入 event_base。如果是 common-timeout，将其插入到 common_timeout_queues，否则插入 timeheap。
```
static void
event_queue_insert_timeout(struct event_base *base, struct event *ev)
{
    EVENT_BASE_ASSERT_LOCKED(base);

    if (EVUTIL_FAILURE_CHECK(ev->ev_flags & EVLIST_TIMEOUT)) {
        event_errx(1, "%s: %p(fd " EV_SOCK_FMT ") already on timeout", __func__,
            ev, EV_SOCK_ARG(ev->ev_fd));
        return;
    }

    INCR_EVENT_COUNT(base, ev->ev_flags);

    ev->ev_flags |= EVLIST_TIMEOUT;

    if (is_common_timeout(&ev->ev_timeout, base)) {
        struct common_timeout_list *ctl =
            get_common_timeout_list(base, &ev->ev_timeout);
        insert_common_timeout_inorder(ctl, ev);
    } else {
        min_heap_push_(&base->timeheap, ev);
    }
}
```
common_timeout_list::events 使用单向链表管理具有相同超时时长的 event，为了方便查找具有最小超时时长的 event，在插入是，按照超时时间（超时时长+添加时间点）递增的顺序组织。insert_common_timeout_inorder() 函数负责插入有序性，其定义如下：
```
static void
insert_common_timeout_inorder(struct common_timeout_list *ctl, struct event *ev)
{
    struct event *e;
    TAILQ_FOREACH_REVERSE (e, &ctl->events, event_list,
        ev_timeout_pos.ev_next_with_common_timeout) {
        EVUTIL_ASSERT(is_same_common_timeout(&e->ev_timeout, &ev->ev_timeout));
        if (evutil_timercmp(&ev->ev_timeout, &e->ev_timeout, >=)) {
            TAILQ_INSERT_AFTER(&ctl->events, e, ev,
                ev_timeout_pos.ev_next_with_common_timeout);
            return;
        }
    }
    TAILQ_INSERT_HEAD(
        &ctl->events, ev, ev_timeout_pos.ev_next_with_common_timeout);
}
```
### event_queue_remove_timeout()
event_queue_remove_timeout() 负责删除一个 timeout 事件。
```
static void
event_queue_remove_timeout(struct event_base *base, struct event *ev)
{
    EVENT_BASE_ASSERT_LOCKED(base);
    if (EVUTIL_FAILURE_CHECK(!(ev->ev_flags & EVLIST_TIMEOUT))) {
        event_errx(1, "%s: %p(fd " EV_SOCK_FMT ") not on queue %x", __func__,
            ev, EV_SOCK_ARG(ev->ev_fd), EVLIST_TIMEOUT);
        return;
    }
    DECR_EVENT_COUNT(base, ev->ev_flags);
    ev->ev_flags &= ~EVLIST_TIMEOUT;

    if (is_common_timeout(&ev->ev_timeout, base)) {
        struct common_timeout_list *ctl =
            get_common_timeout_list(base, &ev->ev_timeout);
        TAILQ_REMOVE(
            &ctl->events, ev, ev_timeout_pos.ev_next_with_common_timeout);
    } else {
        min_heap_erase_(&base->timeheap, ev);
    }
}
```

## monotonic_time
### evutil_monotonic_timer()
event_base 使用 evutil_monotonic_timer 结构单调时间。在 Linux 平台，evutil_monotonic_timer 的定义如下：
```
/// time-internal.h
struct evutil_monotonic_timer {
#ifdef HAVE_POSIX_MONOTONIC
    int monotonic_clock; // CLOCK_MONOTONIC_COARSE
                         // CLOCK_MONOTONIC
#endif

    struct timeval adjust_monotonic_clock; // 补偿时间
    struct timeval last_time; // 上次返回的时间
};
```
如果对时间精度没有要求或者没有特殊设置，Libevent 优先使用 CLOCK_MONOTONIC_COARSE（更快）获取 clock_gettime() 时间，否则使用 CLOCK_MONOTONIC 获取时间。当然，可以设置 EV_MONOT_FALLBACK，使用 gettimeofday() 获取时间。
```
int
evutil_configure_monotonic_time_(struct evutil_monotonic_timer *base,
    int flags)
{
    /* CLOCK_MONOTONIC exists on FreeBSD, Linux, and Solaris.  You need to
     * check for it at runtime, because some older kernel versions won't
     * have it working. */
#ifdef CLOCK_MONOTONIC_COARSE
    const int precise = flags & EV_MONOT_PRECISE;
#endif
    const int fallback = flags & EV_MONOT_FALLBACK;
    struct timespec ts;

#ifdef CLOCK_MONOTONIC_COARSE
    if (CLOCK_MONOTONIC_COARSE < 0) {
        event_errx(1,"I didn't expect CLOCK_MONOTONIC_COARSE to be < 0");
    }
    if (! precise && ! fallback) { // 没有特殊要求
        if (clock_gettime(CLOCK_MONOTONIC_COARSE, &ts) == 0) {
            base->monotonic_clock = CLOCK_MONOTONIC_COARSE;
            return 0;
        }
    }
#endif
    if (!fallback && clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        base->monotonic_clock = CLOCK_MONOTONIC;
        return 0;
    }

    if (CLOCK_MONOTONIC < 0) {
        event_errx(1,"I didn't expect CLOCK_MONOTONIC to be < 0");
    }

    base->monotonic_clock = -1; // EV_MONOT_FALLBACK，使用 gettimeofday
    return 0;
}
```
### event_gettime_monotonic()
event_gettime_monotonic() 用于返回单调时间，直接调用 evutil_gettime_monotonic_() 获取。
```
int
event_gettime_monotonic(struct event_base *base, struct timeval *tv)
{
    int rv = -1;

    if (base && tv) {
        EVBASE_ACQUIRE_LOCK(base, th_base_lock);
        rv = evutil_gettime_monotonic_(&(base->monotonic_timer), tv);
        EVBASE_RELEASE_LOCK(base, th_base_lock);
    }

    return rv;
}
```
evutil_gettime_monotonic_() 用于返回单调时间，如果配置了 EV_MONOT_FALLBACK，使用 gettimeofday() 获取时间，然后再做调整。否则使用 clock_gettime() 获取时间。
```
int
evutil_gettime_monotonic_(struct evutil_monotonic_timer *base,
    struct timeval *tp)
{
    struct timespec ts;

    if (base->monotonic_clock < 0) {
        if (evutil_gettimeofday(tp, NULL) < 0)
            return -1;
        adjust_monotonic_time(base, tp);
        return 0;
    }

    if (clock_gettime(base->monotonic_clock, &ts) == -1)
        return -1;
    tp->tv_sec = ts.tv_sec;
    tp->tv_usec = ts.tv_nsec / 1000;

    return 0;
}
```
adjust_monotonic_time() 用于调整时间，使其满足单调性质（不小于上次返回的时间），定义如下：
```
static void
adjust_monotonic_time(struct evutil_monotonic_timer *base,
    struct timeval *tv)
{   // 加上补偿时间
    evutil_timeradd(tv, &base->adjust_monotonic_clock, tv);
    // 比上返回的小，调整
    if (evutil_timercmp(tv, &base->last_time, <)) {
        struct timeval adjust;
        evutil_timersub(&base->last_time, tv, &adjust);
        evutil_timeradd(&adjust, &base->adjust_monotonic_clock,
            &base->adjust_monotonic_clock);
        *tv = base->last_time;
    }
    base->last_time = *tv;
}
```

## cached_time
为了减少调用 gettimeofday() 次数，Libevent 可以缓存系统时间。如果有缓存的时间，event_base_gettimeofday_cached() 函数直接返回，否则调用 gettimeofday() 获取。
```
int
event_base_gettimeofday_cached(struct event_base *base, struct timeval *tv)
{
    int r;
    if (!base) {
        base = current_base;
        if (!current_base)
            return evutil_gettimeofday(tv, NULL);
    }

    EVBASE_ACQUIRE_LOCK(base, th_base_lock);
    if (base->tv_cache.tv_sec == 0) {
        r = evutil_gettimeofday(tv, NULL);
    } else { // 调整
        evutil_timeradd(&base->tv_cache, &base->tv_clock_diff, tv);
        r = 0;
    }
    EVBASE_RELEASE_LOCK(base, th_base_lock);
    return r;
}
```
可以调用 event_base_update_cache_time() 函数更新缓存时间。只有在执行 event_loop 的时候，才真正更新，其直接调用 update_time_cache() 函数。
```
int
event_base_update_cache_time(struct event_base *base)
{

    if (!base) {
        base = current_base;
        if (!current_base)
            return -1;
    }

    EVBASE_ACQUIRE_LOCK(base, th_base_lock);
    if (base->running_loop)
        update_time_cache(base);
    EVBASE_RELEASE_LOCK(base, th_base_lock);
    return 0;
}

static inline void
update_time_cache(struct event_base *base)
{
    base->tv_cache.tv_sec = 0;
    if (!(base->flags & EVENT_BASE_FLAG_NO_CACHE_TIME))
        gettime(base, &base->tv_cache);
}
```
如果有缓存时间，gettime() 函数将其返回，否则调用 evutil_gettime_monotonic_() 函数获取，并且调整 tv_clock_diff 的值。
```
static int
gettime(struct event_base *base, struct timeval *tp)
{
    EVENT_BASE_ASSERT_LOCKED(base);

    if (base->tv_cache.tv_sec) {
        *tp = base->tv_cache;
        return (0);
    }

    if (evutil_gettime_monotonic_(&base->monotonic_timer, tp) == -1) {
        return -1;
    }

    if (base->last_updated_clock_diff + CLOCK_SYNC_INTERVAL < tp->tv_sec) {
        struct timeval tv;
        evutil_gettimeofday(&tv, NULL);
        evutil_timersub(&tv, tp, &base->tv_clock_diff);
        base->last_updated_clock_diff = tp->tv_sec;
    }

    return 0;
}
```