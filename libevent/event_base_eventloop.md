# event_base 之事件循环

## event_base_loop()
```
int
event_base_loop(struct event_base *base, int flags)
{
    const struct eventop *evsel = base->evsel;
    struct timeval tv;
    struct timeval *tv_p;
    int res, done, retval = 0;

    EVBASE_ACQUIRE_LOCK(base, th_base_lock);

    if (base->running_loop) { // 已经在 evnet_loop
        event_warnx("%s: reentrant invocation.  Only one event_base_loop"
                    " can run on each event_base at once.",
            __func__);
        EVBASE_RELEASE_LOCK(base, th_base_lock);
        return -1;
    }

    base->running_loop = 1;

    clear_time_cache(base);

    if (base->sig.ev_signal_added && base->sig.ev_n_signals_added)
        evsig_set_base_(base);

    done = 0;

#ifndef EVENT__DISABLE_THREAD_SUPPORT
    base->th_owner_id = EVTHREAD_GET_ID();
#endif

    base->event_gotterm = base->event_break = 0;

    while (!done) {
        base->event_continue = 0;
        base->n_deferreds_queued = 0;

        /* Terminate the loop if we have been asked to */
        if (base->event_gotterm) {
            break;
        }

        if (base->event_break) {
            break;
        }

        tv_p = &tv;
        if (!N_ACTIVE_CALLBACKS(base) && !(flags & EVLOOP_NONBLOCK)) {
            timeout_next(base, &tv_p);
        } else {
            /*
             * if we have active events, we just poll new events
             * without waiting.
             */
            evutil_timerclear(&tv);
        }

        /* If we have no events, we just exit */
        if (0 == (flags & EVLOOP_NO_EXIT_ON_EMPTY) && !event_haveevents(base) &&
            !N_ACTIVE_CALLBACKS(base)) {
            event_debug(("%s: no events registered.", __func__));
            retval = 1;
            goto done;
        }

        event_queue_make_later_events_active(base);

        clear_time_cache(base);

        res = evsel->dispatch(base, tv_p);

        if (res == -1) {
            event_debug(("%s: dispatch returned unsuccessfully.", __func__));
            retval = -1;
            goto done;
        }

        update_time_cache(base);

        timeout_process(base);

        if (N_ACTIVE_CALLBACKS(base)) {
            int n = event_process_active(base);
            if ((flags & EVLOOP_ONCE) && N_ACTIVE_CALLBACKS(base) == 0 &&
                n != 0)
                done = 1;
        } else if (flags & EVLOOP_NONBLOCK)
            done = 1;
    }
    event_debug(("%s: asked to terminate loop.", __func__));

done:
    clear_time_cache(base);
    base->running_loop = 0;

    EVBASE_RELEASE_LOCK(base, th_base_lock);

    return (retval);
}
```

## timeout_process()
timeout_process() 函数检查 timeheap 中的 event，如果某个 event 已经超时，将其标记为 ACTIVE 状态。
```
static void
timeout_process(struct event_base *base)
{
    /* Caller must hold lock. */
    struct timeval now;
    struct event *ev;

    if (min_heap_empty_(&base->timeheap)) {
        return;
    }

    gettime(base, &now);

    while ((ev = min_heap_top_(&base->timeheap))) {
        if (evutil_timercmp(&ev->ev_timeout, &now, >))
            break;

        /* delete this event from the I/O queues */
        event_del_nolock_(ev, EVENT_DEL_NOBLOCK);

        event_debug(
            ("timeout_process: event: %p, call %p", ev, ev->ev_callback));
        event_active_nolock_(ev, EV_TIMEOUT, 1);
    }
}
```

## event_process_active()
event_process_active() 函数处理所有 ACTIVE 事件。
```
static int
event_process_active(struct event_base *base)
{
    /* Caller must hold th_base_lock */
    struct evcallback_list *activeq = NULL;
    int i, c = 0;
    const struct timeval *endtime;
    struct timeval tv;
    const int maxcb = base->max_dispatch_callbacks;
    const int limit_after_prio = base->limit_callbacks_after_prio;
    if (base->max_dispatch_time.tv_sec >= 0) {
        update_time_cache(base);
        gettime(base, &tv);
        evutil_timeradd(&base->max_dispatch_time, &tv, &tv);
        endtime = &tv;
    } else {
        endtime = NULL;
    }

    for (i = 0; i < base->nactivequeues; ++i) {
        if (TAILQ_FIRST(&base->activequeues[i]) != NULL) {
            base->event_running_priority = i;
            activeq = &base->activequeues[i];
            if (i < limit_after_prio)
                c = event_process_active_single_queue(
                    base, activeq, INT_MAX, NULL);
            else
                c = event_process_active_single_queue(
                    base, activeq, maxcb, endtime);
            if (c < 0) {
                goto done;
            } else if (c > 0)
                break; /* Processed a real event; do not
                        * consider lower-priority events */
                       /* If we get here, all of the events we processed
                        * were internal.  Continue. */
        }
    }

done:
    base->event_running_priority = -1;

    return c;
}
```
## event_base_loop\*()
### event_base_loopexit()
```
int
event_base_loopexit(struct event_base *event_base, const struct timeval *tv)
{
    return (event_base_once(
        event_base, -1, EV_TIMEOUT, event_loopexit_cb, event_base, tv));
}
```
### event_base_loopbreak()
```
int
event_base_loopbreak(struct event_base *event_base)
{
    int r = 0;
    if (event_base == NULL)
        return (-1);

    EVBASE_ACQUIRE_LOCK(event_base, th_base_lock);
    event_base->event_break = 1;

    if (EVBASE_NEED_NOTIFY(event_base)) {
        r = evthread_notify_base(event_base);
    } else {
        r = (0);
    }
    EVBASE_RELEASE_LOCK(event_base, th_base_lock);
    return r;
}
```

### event_base_loopcontinue()
```
int
event_base_loopcontinue(struct event_base *event_base)
{
    int r = 0;
    if (event_base == NULL)
        return (-1);

    EVBASE_ACQUIRE_LOCK(event_base, th_base_lock);
    event_base->event_continue = 1;

    if (EVBASE_NEED_NOTIFY(event_base)) {
        r = evthread_notify_base(event_base);
    } else {
        r = (0);
    }
    EVBASE_RELEASE_LOCK(event_base, th_base_lock);
    return r;
}
```