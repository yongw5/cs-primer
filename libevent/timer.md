# Lievent 的定时器
Libevent 没有专门的定时器，如果 event::ev_fd 为 -1，event 就是一个定时器。
```
#define evtimer_assign(ev, b, cb, arg) \
    event_assign((ev), (b), -1, 0, (cb), (arg))
#define evtimer_new(b, cb, arg) event_new((b), -1, 0, (cb), (arg))
#define evtimer_add(ev, tv) event_add((ev), (tv))
#define evtimer_del(ev) event_del(ev)
#define evtimer_pending(ev, tv) event_pending((ev), EV_TIMEOUT, (tv))
#define evtimer_initialized(ev) event_initialized(ev)
```

## 时间管理
libevent 中有两个时间相关的数据结构：min-heap 和 common-timeout-list。

首先区分两个概念：超时时间和超时时长
- 超时时间：表示一个时间点
- 超时时长：表示一段时间间隔

libevent 统一使用 timeval 表示超时时间和超时时长，下面结合源码分析
```
// tv 表示超时时长
event_add(ev, tv):
    event_add_nolock_(ev, tv, 0);
```
event_add_nolock_() 最后一个参数表示 tv 是否是绝对时间。因此从 event_add() 的调用过程来看，event_add() 中设置的 tv 表示超时时长。

从 event_add_nolock_() 可以看到，ev_io_timeout 记录的是超时时长（timeout），ev_timeout 记录的是超时时间（deadline）
```
event_add_nolock_(ev, tv, tv_is_absolute) 
{
    /// ...
    if (ev->ev_closure == EV_CLOSURE_EVENT_PERSIST && !tv_is_absolute)
        ev->ev_io_timeout = *tv;

    if (tv_is_absolute) {
            ev->ev_timeout = *tv;
    } else if (common_timeout) {
        struct timeval tmp = *tv;
        tmp.tv_usec &= MICROSECONDS_MASK;
        evutil_timeradd(&now, &tmp, &ev->ev_timeout);
        ev->ev_timeout.tv_usec |= (tv->tv_usec & ~MICROSECONDS_MASK);
    } else {
        evutil_timeradd(&now, tv, &ev->ev_timeout);
    }
    /// ...
}   
```
可以看到，不管是 commom-timer 还是 min-heap 中保存的都是绝对时间，表示超时时长，其值为调用 event_add() 的时间点，加上指定的超时时长。

## common_timeout
时间堆插入和删除操作时间复杂度都是 O(logN)，但是如果把大量具有相同超时时长的 event 放到时间堆上，效率会下降很多。common-timeout 是专门处理有大量相同的超时时间的情况。

超时时间（deadline） = 调用 event_add() 的时间（now） + 超时时长（timeout）

对于具有相同超时时长的超时事件，可以把它们按照超时时间升序放到一个 common-timeout 队列中。那么队首元素必定最先超时，如果队首元素没有超时，该队列里所有元素都没有超时。libevent 专门为该队列创建了一个代表事件 -- common-timeout 超时事件，它的超时时间被设置为队首元素的超时时间，由它代替队列中的元素插入到小根堆中。

主要算法如下：
1. common-timeout 超时事件超时后，在其 callback 函数中检查队首元素是否超时；
1. 如果队首元素超时，取出队首元素，直接放入激活队列，然后继续检查是否有其他队内元素超时；
1. 如果队首元素未超时，就把队首元素的超时时间赋值给 common-timeout 超时事件，重新注册 common-timeout 事件。注册 commom-timeout 超时事件时，会把它插入时间堆中，用时间堆监控该事件是否超时。如果 common-timeout 超时事件激活了，就又调用回调函数，又是一个循环。

这样就不用把大量具有相同超时时长的超时事件放入时间堆，而只需要把这些相同超时时长的超时事件按时间顺序放入 common-timeout 队列，生成一个代表事件，把该事件放入事件堆，从而减轻了时间堆的负担。

libevent 在 timeval.tv_usec 中标记是否是 common-timeout。tv_usec 的单位是微妙，最大值为 999999，只需要 20 比特位就足以存储，但是 tv_usec 至少是 32 位。因此，libevent 是这样区分的：

- tv_usec 的低 20 位存储微妙值；
- 中间 8 位存储 commom-timeout 数组的下标值；
- 高 4 位标志 commom-timeout；

下面的宏定义可以看出具体的规则
```
#define MICROSECONDS_MASK COMMON_TIMEOUT_MICROSECONDS_MASK
#define COMMON_TIMEOUT_IDX_MASK 0x0ff00000
#define COMMON_TIMEOUT_IDX_SHIFT 20
#define COMMON_TIMEOUT_MASK 0xf0000000
#define COMMON_TIMEOUT_MAGIC 0x50000000

#define COMMON_TIMEOUT_IDX(tv) \
    (((tv)->tv_usec & COMMON_TIMEOUT_IDX_MASK) >> COMMON_TIMEOUT_IDX_SHIFT)
```

## event_base_init_common_timeout()
event_base_init_common_timeout() 函数用于返回一个 commom-timer
```
const struct timeval *
event_base_init_common_timeout(
    struct event_base *base, const struct timeval *duration)
{
    int i;
    struct timeval tv;
    const struct timeval *result = NULL;
    struct common_timeout_list *new_ctl;

    EVBASE_ACQUIRE_LOCK(base, th_base_lock);
    if (duration->tv_usec > 1000000) {
        /// 归一化 duration
        memcpy(&tv, duration, sizeof(struct timeval));
        if (is_common_timeout(duration, base))
            tv.tv_usec &= MICROSECONDS_MASK;
        tv.tv_sec += tv.tv_usec / 1000000;
        tv.tv_usec %= 1000000;
        duration = &tv;
    }
    for (i = 0; i < base->n_common_timeouts; ++i) {
        /// 在已知中是否存在
        const struct common_timeout_list *ctl = base->common_timeout_queues[i];
        if (duration->tv_sec == ctl->duration.tv_sec &&
            duration->tv_usec == (ctl->duration.tv_usec & MICROSECONDS_MASK)) {
            EVUTIL_ASSERT(is_common_timeout(&ctl->duration, base));
            result = &ctl->duration;
            goto done;
        }
    }
    if (base->n_common_timeouts == MAX_COMMON_TIMEOUTS) {
        event_warnx("%s: Too many common timeouts already in use; "
                    "we only support %d per event_base",
            __func__, MAX_COMMON_TIMEOUTS);
        goto done;
    }
    if (base->n_common_timeouts_allocated == base->n_common_timeouts) {
        int n = base->n_common_timeouts < 16 ? 16 : base->n_common_timeouts * 2;
        struct common_timeout_list **newqueues =
            mm_realloc(base->common_timeout_queues,
                n * sizeof(struct common_timeout_queue *));
        if (!newqueues) {
            event_warn("%s: realloc", __func__);
            goto done;
        }
        base->n_common_timeouts_allocated = n;
        base->common_timeout_queues = newqueues;
    }
    new_ctl = mm_calloc(1, sizeof(struct common_timeout_list));
    if (!new_ctl) {
        event_warn("%s: calloc", __func__);
        goto done;
    }
    TAILQ_INIT(&new_ctl->events);
    new_ctl->duration.tv_sec = duration->tv_sec;
    new_ctl->duration.tv_usec =
        duration->tv_usec | COMMON_TIMEOUT_MAGIC |
        (base->n_common_timeouts << COMMON_TIMEOUT_IDX_SHIFT);
    evtimer_assign(
        &new_ctl->timeout_event, base, common_timeout_callback, new_ctl);
    new_ctl->timeout_event.ev_flags |= EVLIST_INTERNAL;
    event_priority_set(&new_ctl->timeout_event, 0);
    new_ctl->base = base;
    base->common_timeout_queues[base->n_common_timeouts++] = new_ctl;
    result = &new_ctl->duration;

done:
    if (result)
        EVUTIL_ASSERT(is_common_timeout(result, base));

    EVBASE_RELEASE_LOCK(base, th_base_lock);
    return result;
}
```
timeout_event 的回调函数为 common_timeout_callback()，其检查 common-timeout 对应的所有 event，将已经超时的 event 标记为 ACTIVE 状态。