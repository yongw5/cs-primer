# event 注册与注销
## event_add\*()
### event_add()
```
int
event_add(struct event *ev, const struct timeval *tv)
{
    int res;

    if (EVUTIL_FAILURE_CHECK(!ev->ev_base)) {
        // error, return -1
    }

    EVBASE_ACQUIRE_LOCK(ev->ev_base, th_base_lock);
    res = event_add_nolock_(ev, tv, 0);
    EVBASE_RELEASE_LOCK(ev->ev_base, th_base_lock);

    return (res);
}
```
### event_add_nolock_()
```
int
event_add_nolock_(
    struct event *ev, const struct timeval *tv, int tv_is_absolute)
{
    struct event_base *base = ev->ev_base;
    int res = 0;
    int notify = 0;

    EVENT_BASE_ASSERT_LOCKED(base);
    event_debug_assert_is_setup_(ev);
// ... debug output info
    EVUTIL_ASSERT(!(ev->ev_flags & ~EVLIST_ALL));

    if (ev->ev_flags & EVLIST_FINALIZING) {
        return (-1);
    }

    if (tv != NULL && !(ev->ev_flags & EVLIST_TIMEOUT)) {
        if (min_heap_reserve_(
                &base->timeheap, 1 + min_heap_size_(&base->timeheap)) == -1)
            return (-1); /* ENOMEM == errno */
    }
/// 如果不在主线程并且主线程正在调用回调函数，等待 event_loop 结束
#ifndef EVENT__DISABLE_THREAD_SUPPORT
    if (base->current_event == event_to_event_callback(ev) &&
        (ev->ev_events & EV_SIGNAL) && !EVBASE_IN_THREAD(base)) {
        ++base->current_event_waiters;
        EVTHREAD_COND_WAIT(base->current_event_cond, base->th_base_lock);
    }
#endif

    if ((ev->ev_events & (EV_READ | EV_WRITE | EV_CLOSED | EV_SIGNAL)) &&
        !(ev->ev_flags &
            (EVLIST_INSERTED | EVLIST_ACTIVE | EVLIST_ACTIVE_LATER))) {
        if (ev->ev_events & (EV_READ | EV_WRITE | EV_CLOSED))
            res = evmap_io_add_(base, ev->ev_fd, ev);
        else if (ev->ev_events & EV_SIGNAL)
            res = evmap_signal_add_(base, (int)ev->ev_fd, ev);
        if (res != -1)
            event_queue_insert_inserted(base, ev);
        if (res == 1) {
            /* evmap says we need to notify the main thread. */
            notify = 1;
            res = 0;
        }
    }
/// tv 不为 NULL，需要设置 timeout
    if (res != -1 && tv != NULL) {
        struct timeval now;
        int common_timeout;
#ifdef USE_REINSERT_TIMEOUT
        // ...
#endif

        if (ev->ev_closure == EV_CLOSURE_EVENT_PERSIST && !tv_is_absolute)
            ev->ev_io_timeout = *tv;

#ifndef USE_REINSERT_TIMEOUT
        if (ev->ev_flags & EVLIST_TIMEOUT) {
            event_queue_remove_timeout(base, ev);
        }
#endif
        // 先从 ACTIVE 队列移除
        if ((ev->ev_flags & EVLIST_ACTIVE) && (ev->ev_res & EV_TIMEOUT)) {
            if (ev->ev_events & EV_SIGNAL) {
                if (ev->ev_ncalls && ev->ev_pncalls) {
                    *ev->ev_pncalls = 0; // 退出 event_loop
                }
            }

            event_queue_remove_active(base, event_to_event_callback(ev));
        }

        gettime(base, &now);

        common_timeout = is_common_timeout(tv, base);
#ifdef USE_REINSERT_TIMEOUT
        // ...
#endif

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
/// debug output...

#ifdef USE_REINSERT_TIMEOUT
        // ...
#else
        event_queue_insert_timeout(base, ev);
#endif

        if (common_timeout) {
            struct common_timeout_list *ctl =
                get_common_timeout_list(base, &ev->ev_timeout);
            if (ev == TAILQ_FIRST(&ctl->events)) {
                common_timeout_schedule(ctl, &now, ev);
            }
        } else {
            struct event *top = NULL;
            if (min_heap_elt_is_top_(ev))
                notify = 1;
            else if ((top = min_heap_top_(&base->timeheap)) != NULL &&
                     evutil_timercmp(&top->ev_timeout, &now, <))
                notify = 1;
        }
    }

    // 唤醒主线程
    if (res != -1 && notify && EVBASE_NEED_NOTIFY(base))
        evthread_notify_base(base);

    event_debug_note_add_(ev);

    return (res);
}
```

## event_del\*()
### event_del()
```
int
event_del(struct event *ev)
{
    return event_del_(ev, EVENT_DEL_AUTOBLOCK);
}
```
### event_del_block()
```
int
event_del_block(struct event *ev)
{
    return event_del_(ev, EVENT_DEL_BLOCK);
}
```
### event_del_noblock()
```
int
event_del_noblock(struct event *ev)
{
    return event_del_(ev, EVENT_DEL_NOBLOCK);
}
```
### event_del_()
```
static int
event_del_(struct event *ev, int blocking)
{
    int res;
    struct event_base *base = ev->ev_base;

    if (EVUTIL_FAILURE_CHECK(!base)) {
        event_warnx("%s: event has no event_base set.", __func__);
        return -1;
    }

    EVBASE_ACQUIRE_LOCK(base, th_base_lock);
    res = event_del_nolock_(ev, blocking);
    EVBASE_RELEASE_LOCK(base, th_base_lock);

    return (res);
}
```
### event_del_nolock_()
```
int
event_del_nolock_(struct event *ev, int blocking)
{
    struct event_base *base;
    int res = 0, notify = 0;

/// debug ouput...

    if (ev->ev_base == NULL)
        return (-1);

    EVENT_BASE_ASSERT_LOCKED(ev->ev_base);

    if (blocking != EVENT_DEL_EVEN_IF_FINALIZING) {
        if (ev->ev_flags & EVLIST_FINALIZING) {
            return 0;
        }
    }

    base = ev->ev_base;

    EVUTIL_ASSERT(!(ev->ev_flags & ~EVLIST_ALL));

    if (ev->ev_events & EV_SIGNAL) {
        if (ev->ev_ncalls && ev->ev_pncalls) {
            *ev->ev_pncalls = 0; // 退出 event_loop
        }
    }

    if (ev->ev_flags & EVLIST_TIMEOUT) {
        event_queue_remove_timeout(base, ev);
    }

    if (ev->ev_flags & EVLIST_ACTIVE)
        event_queue_remove_active(base, event_to_event_callback(ev));
    else if (ev->ev_flags & EVLIST_ACTIVE_LATER)
        event_queue_remove_active_later(base, event_to_event_callback(ev));

    if (ev->ev_flags & EVLIST_INSERTED) {
        event_queue_remove_inserted(base, ev);
        if (ev->ev_events & (EV_READ | EV_WRITE | EV_CLOSED))
            res = evmap_io_del_(base, ev->ev_fd, ev);
        else
            res = evmap_signal_del_(base, (int)ev->ev_fd, ev);
        if (res == 1) {
            /* evmap says we need to notify the main thread. */
            notify = 1;
            res = 0;
        }
        /* If we do not have events, let's notify event base so it can
         * exit without waiting */
        if (!event_haveevents(base) && !N_ACTIVE_CALLBACKS(base))
            notify = 1;
    }

    /* if we are not in the right thread, we need to wake up the loop */
    if (res != -1 && notify && EVBASE_NEED_NOTIFY(base))
        evthread_notify_base(base);

    event_debug_note_del_(ev);

    // 正在执行当前回调函数，等待
#ifndef EVENT__DISABLE_THREAD_SUPPORT
    if (blocking != EVENT_DEL_NOBLOCK &&
        base->current_event == event_to_event_callback(ev) &&
        !EVBASE_IN_THREAD(base) &&
        (blocking == EVENT_DEL_BLOCK || !(ev->ev_events & EV_FINALIZE))) {
        ++base->current_event_waiters;
        EVTHREAD_COND_WAIT(base->current_event_cond, base->th_base_lock);
    }
#endif

    return (res);
}
```

## event_active\*()
### event_active()
```
void
event_active(struct event *ev, int res, short ncalls)
{
    if (EVUTIL_FAILURE_CHECK(!ev->ev_base)) {
        event_warnx("%s: event has no event_base set.", __func__);
        return;
    }

    EVBASE_ACQUIRE_LOCK(ev->ev_base, th_base_lock);

    event_debug_assert_is_setup_(ev);

    event_active_nolock_(ev, res, ncalls);

    EVBASE_RELEASE_LOCK(ev->ev_base, th_base_lock);
}
```
### event_active_nolock_()
```
void
event_active_nolock_(struct event *ev, int res, short ncalls)
{
    struct event_base *base;

    event_debug(("event_active: %p (fd " EV_SOCK_FMT "), res %d, callback %p",
        ev, EV_SOCK_ARG(ev->ev_fd), (int)res, ev->ev_callback));

    base = ev->ev_base;
    EVENT_BASE_ASSERT_LOCKED(base);

    if (ev->ev_flags & EVLIST_FINALIZING) {
        /* XXXX debug */
        return;
    }

    switch ((ev->ev_flags & (EVLIST_ACTIVE | EVLIST_ACTIVE_LATER))) {
    default:
    case EVLIST_ACTIVE | EVLIST_ACTIVE_LATER:
        EVUTIL_ASSERT(0);
        break;
    case EVLIST_ACTIVE:
        /* We get different kinds of events, add them together */
        ev->ev_res |= res;
        return;
    case EVLIST_ACTIVE_LATER:
        ev->ev_res |= res;
        break;
    case 0:
        ev->ev_res = res;
        break;
    }

    if (ev->ev_pri < base->event_running_priority)
        base->event_continue = 1;

    if (ev->ev_events & EV_SIGNAL) {
#ifndef EVENT__DISABLE_THREAD_SUPPORT
        if (base->current_event == event_to_event_callback(ev) &&
            !EVBASE_IN_THREAD(base)) {
            ++base->current_event_waiters;
            EVTHREAD_COND_WAIT(base->current_event_cond, base->th_base_lock);
        }
#endif
        ev->ev_ncalls = ncalls;
        ev->ev_pncalls = NULL;
    }

    event_callback_activate_nolock_(base, event_to_event_callback(ev));
}
```
### event_callback_activate_nolock_()
```
int
event_callback_activate_nolock_(
    struct event_base *base, struct event_callback *evcb)
{
    int r = 1;

    if (evcb->evcb_flags & EVLIST_FINALIZING)
        return 0;

    switch (evcb->evcb_flags & (EVLIST_ACTIVE | EVLIST_ACTIVE_LATER)) {
    default:
        EVUTIL_ASSERT(0);
        EVUTIL_FALLTHROUGH;
    case EVLIST_ACTIVE_LATER:
        event_queue_remove_active_later(base, evcb);
        r = 0;
        break;
    case EVLIST_ACTIVE:
        return 0;
    case 0:
        break;
    }

    event_queue_insert_active(base, evcb);

    if (EVBASE_NEED_NOTIFY(base))
        evthread_notify_base(base);

    return r;
}
```
## event_base_once()
```
int
event_base_once(struct event_base *base, evutil_socket_t fd, short events,
    void (*callback)(evutil_socket_t, short, void *), void *arg,
    const struct timeval *tv)
{
    struct event_once *eonce;
    int res = 0;
    int activate = 0;

    if (!base)
        return (-1);

    /* We cannot support signals that just fire once, or persistent
     * events. */
    if (events & (EV_SIGNAL | EV_PERSIST))
        return (-1);

    if ((eonce = mm_calloc(1, sizeof(struct event_once))) == NULL)
        return (-1);

    eonce->cb = callback;
    eonce->arg = arg;

    if ((events & (EV_TIMEOUT | EV_SIGNAL | EV_READ | EV_WRITE | EV_CLOSED)) ==
        EV_TIMEOUT) {
        evtimer_assign(&eonce->ev, base, event_once_cb, eonce);

        if (tv == NULL || !evutil_timerisset(tv)) {
            /* If the event is going to become active immediately,
             * don't put it on the timeout queue.  This is one
             * idiom for scheduling a callback, so let's make
             * it fast (and order-preserving). */
            activate = 1;
        }
    } else if (events & (EV_READ | EV_WRITE | EV_CLOSED)) {
        events &= EV_READ | EV_WRITE | EV_CLOSED;

        event_assign(&eonce->ev, base, fd, events, event_once_cb, eonce);
    } else {
        /* Bad event combination */
        mm_free(eonce);
        return (-1);
    }

    if (res == 0) {
        EVBASE_ACQUIRE_LOCK(base, th_base_lock);
        if (activate)
            event_active_nolock_(&eonce->ev, EV_TIMEOUT, 1);
        else
            res = event_add_nolock_(&eonce->ev, tv, 0);

        if (res != 0) {
            mm_free(eonce);
            return (res);
        } else {
            LIST_INSERT_HEAD(&base->once_events, eonce, next_once);
        }
        EVBASE_RELEASE_LOCK(base, th_base_lock);
    }

    return (0);
}
```