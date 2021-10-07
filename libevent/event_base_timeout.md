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

