# event_base 如何管理 ACTIVE 事件
```
struct event_base {
    // ...
    struct event_io_map io; // fd=>event*
    // ...

    struct evcallback_list *activequeues;
    int nactivequeues;
    struct evcallback_list active_later_queue;
    // ...
};
```
## activequeues
event_base 将 ACTIVE 状态的 event 按照优先级组织成一个链表。activequeues 是一个数组，其元素是链表头结点，数组的下标表示也代表优先级，所有 ACTIVE 状态且优先级相同的 event 链接到一个链表上。

### event_base_priority_init()
event_base_priority_init() 设置 nactivequeues，其定义如下：
```
int
event_base_priority_init(struct event_base *base, int npriorities)
{
    int i, r;
    r = -1;

    EVBASE_ACQUIRE_LOCK(base, th_base_lock);

    if (N_ACTIVE_CALLBACKS(base) || npriorities < 1 ||
        npriorities >= EVENT_MAX_PRIORITIES) // 最大 256
        goto err;

    if (npriorities == base->nactivequeues)
        goto ok;

    if (base->nactivequeues) {
        mm_free(base->activequeues);
        base->nactivequeues = 0;
    }

    /* Allocate our priority queues */
    base->activequeues = (struct evcallback_list *)mm_calloc(
        npriorities, sizeof(struct evcallback_list));
    if (base->activequeues == NULL) {
        event_warn("%s: calloc", __func__);
        goto err;
    }
    base->nactivequeues = npriorities;

    for (i = 0; i < base->nactivequeues; ++i) {
        TAILQ_INIT(&base->activequeues[i]);
    }

ok:
    r = 0;
err:
    EVBASE_RELEASE_LOCK(base, th_base_lock);
    return (r);
}
```
event 的接口 event_priority_init() 也可以设置 nactivequeues
```
int
event_priority_init(int npriorities)
{
    return event_base_priority_init(current_base, npriorities);
}
```

## event_queue_insert_*()
### event_queue_insert_inserted()
```
static void
event_queue_insert_inserted(struct event_base *base, struct event *ev)
{
    EVENT_BASE_ASSERT_LOCKED(base);

    if (EVUTIL_FAILURE_CHECK(ev->ev_flags & EVLIST_INSERTED)) {
        event_errx(1, "%s: %p(fd " EV_SOCK_FMT ") already inserted", __func__,
            ev, EV_SOCK_ARG(ev->ev_fd));
        return;
    }

    INCR_EVENT_COUNT(base, ev->ev_flags);

    ev->ev_flags |= EVLIST_INSERTED;
}
```
### event_queue_insert_active
```
static void
event_queue_insert_active(struct event_base *base, struct event_callback *evcb)
{
    EVENT_BASE_ASSERT_LOCKED(base);

    if (evcb->evcb_flags & EVLIST_ACTIVE) {
        /* Double insertion is possible for active events */
        return;
    }

    INCR_EVENT_COUNT(base, evcb->evcb_flags);

    evcb->evcb_flags |= EVLIST_ACTIVE;

    base->event_count_active++;
    MAX_EVENT_COUNT(base->event_count_active_max, base->event_count_active);
    EVUTIL_ASSERT(evcb->evcb_pri < base->nactivequeues);
    TAILQ_INSERT_TAIL(
        &base->activequeues[evcb->evcb_pri], evcb, evcb_active_next);
}
```
### event_queue_insert_active_later()
```
static void
event_queue_insert_active_later(
    struct event_base *base, struct event_callback *evcb)
{
    EVENT_BASE_ASSERT_LOCKED(base);
    if (evcb->evcb_flags & (EVLIST_ACTIVE_LATER | EVLIST_ACTIVE)) {
        /* Double insertion is possible */
        return;
    }

    INCR_EVENT_COUNT(base, evcb->evcb_flags);
    evcb->evcb_flags |= EVLIST_ACTIVE_LATER;
    base->event_count_active++;
    MAX_EVENT_COUNT(base->event_count_active_max, base->event_count_active);
    EVUTIL_ASSERT(evcb->evcb_pri < base->nactivequeues);
    TAILQ_INSERT_TAIL(&base->active_later_queue, evcb, evcb_active_next);
}
```

## event_queue_remove_*()
### event_queue_remove_active()
```
static void
event_queue_remove_active(struct event_base *base, struct event_callback *evcb)
{
    EVENT_BASE_ASSERT_LOCKED(base);
    if (EVUTIL_FAILURE_CHECK(!(evcb->evcb_flags & EVLIST_ACTIVE))) {
        event_errx(1, "%s: %p not on queue %x", __func__, evcb, EVLIST_ACTIVE);
        return;
    }
    DECR_EVENT_COUNT(base, evcb->evcb_flags);
    evcb->evcb_flags &= ~EVLIST_ACTIVE;
    base->event_count_active--;

    TAILQ_REMOVE(&base->activequeues[evcb->evcb_pri], evcb, evcb_active_next);
}
```
### event_queue_remove_active_later()
```
static void
event_queue_remove_active_later(
    struct event_base *base, struct event_callback *evcb)
{
    EVENT_BASE_ASSERT_LOCKED(base);
    if (EVUTIL_FAILURE_CHECK(!(evcb->evcb_flags & EVLIST_ACTIVE_LATER))) {
        event_errx(
            1, "%s: %p not on queue %x", __func__, evcb, EVLIST_ACTIVE_LATER);
        return;
    }
    DECR_EVENT_COUNT(base, evcb->evcb_flags);
    evcb->evcb_flags &= ~EVLIST_ACTIVE_LATER;
    base->event_count_active--;

    TAILQ_REMOVE(&base->active_later_queue, evcb, evcb_active_next);
}
```
### event_queue_make_later_events_active()
```
static void
event_queue_make_later_events_active(struct event_base *base)
{
    struct event_callback *evcb;
    EVENT_BASE_ASSERT_LOCKED(base);

    while ((evcb = TAILQ_FIRST(&base->active_later_queue))) {
        TAILQ_REMOVE(&base->active_later_queue, evcb, evcb_active_next);
        evcb->evcb_flags =
            (evcb->evcb_flags & ~EVLIST_ACTIVE_LATER) | EVLIST_ACTIVE;
        EVUTIL_ASSERT(evcb->evcb_pri < base->nactivequeues);
        TAILQ_INSERT_TAIL(
            &base->activequeues[evcb->evcb_pri], evcb, evcb_active_next);
        base->n_deferreds_queued += (evcb->evcb_closure == EV_CLOSURE_CB_SELF);
    }
}
```

## event_process_active_single_queue()
```
static int
event_process_active_single_queue(struct event_base *base,
    struct evcallback_list *activeq, int max_to_process,
    const struct timeval *endtime)
{
    struct event_callback *evcb;
    int count = 0;

    EVUTIL_ASSERT(activeq != NULL);

    for (evcb = TAILQ_FIRST(activeq); evcb; evcb = TAILQ_FIRST(activeq)) {
        struct event *ev = NULL;
        if (evcb->evcb_flags & EVLIST_INIT) {
            ev = event_callback_to_event(evcb);

            if (ev->ev_events & EV_PERSIST || ev->ev_flags & EVLIST_FINALIZING)
                event_queue_remove_active(base, evcb);
            else
                event_del_nolock_(ev, EVENT_DEL_NOBLOCK);
            event_debug(("event_process_active: event: %p, %s%s%scall %p", ev,
                ev->ev_res & EV_READ ? "EV_READ " : " ",
                ev->ev_res & EV_WRITE ? "EV_WRITE " : " ",
                ev->ev_res & EV_CLOSED ? "EV_CLOSED " : " ", ev->ev_callback));
        } else {
            event_queue_remove_active(base, evcb);
            event_debug(("event_process_active: event_callback %p, "
                         "closure %d, call %p",
                evcb, evcb->evcb_closure, evcb->evcb_cb_union.evcb_callback));
        }

        if (!(evcb->evcb_flags & EVLIST_INTERNAL))
            ++count;


        base->current_event = evcb;
#ifndef EVENT__DISABLE_THREAD_SUPPORT
        base->current_event_waiters = 0;
#endif

        switch (evcb->evcb_closure) {
        case EV_CLOSURE_EVENT_SIGNAL:
            EVUTIL_ASSERT(ev != NULL);
            event_signal_closure(base, ev);
            break;
        case EV_CLOSURE_EVENT_PERSIST:
            EVUTIL_ASSERT(ev != NULL);
            event_persist_closure(base, ev);
            break;
        case EV_CLOSURE_EVENT: {
            void (*evcb_callback)(evutil_socket_t, short, void *);
            short res;
            EVUTIL_ASSERT(ev != NULL);
            evcb_callback = *ev->ev_callback;
            res = ev->ev_res;
            EVBASE_RELEASE_LOCK(base, th_base_lock);
            evcb_callback(ev->ev_fd, res, ev->ev_arg);
        } break;
        case EV_CLOSURE_CB_SELF: {
            void (*evcb_selfcb)(struct event_callback *, void *) =
                evcb->evcb_cb_union.evcb_selfcb;
            EVBASE_RELEASE_LOCK(base, th_base_lock);
            evcb_selfcb(evcb, evcb->evcb_arg);
        } break;
        case EV_CLOSURE_EVENT_FINALIZE:
        case EV_CLOSURE_EVENT_FINALIZE_FREE: {
            void (*evcb_evfinalize)(struct event *, void *);
            int evcb_closure = evcb->evcb_closure;
            EVUTIL_ASSERT(ev != NULL);
            base->current_event = NULL;
            evcb_evfinalize = ev->ev_evcallback.evcb_cb_union.evcb_evfinalize;
            EVUTIL_ASSERT((evcb->evcb_flags & EVLIST_FINALIZING));
            EVBASE_RELEASE_LOCK(base, th_base_lock);
            event_debug_note_teardown_(ev);
            evcb_evfinalize(ev, ev->ev_arg);
            if (evcb_closure == EV_CLOSURE_EVENT_FINALIZE_FREE)
                mm_free(ev);
        } break;
        case EV_CLOSURE_CB_FINALIZE: {
            void (*evcb_cbfinalize)(struct event_callback *, void *) =
                evcb->evcb_cb_union.evcb_cbfinalize;
            base->current_event = NULL;
            EVUTIL_ASSERT((evcb->evcb_flags & EVLIST_FINALIZING));
            EVBASE_RELEASE_LOCK(base, th_base_lock);
            evcb_cbfinalize(evcb, evcb->evcb_arg);
        } break;
        default:
            EVUTIL_ASSERT(0);
        }

        EVBASE_ACQUIRE_LOCK(base, th_base_lock);
        base->current_event = NULL;
#ifndef EVENT__DISABLE_THREAD_SUPPORT
        if (base->current_event_waiters) {
            base->current_event_waiters = 0;
            EVTHREAD_COND_BROADCAST(base->current_event_cond);
        }
#endif

        if (base->event_break)
            return -1;
        if (count >= max_to_process)
            return count;
        if (count && endtime) {
            struct timeval now;
            update_time_cache(base);
            gettime(base, &now);
            if (evutil_timercmp(&now, endtime, >=))
                return count;
        }
        if (base->event_continue)
            break;
    }
    return count;
}
```

## event_base_free_queues_()
```
static int
event_base_free_queues_(struct event_base *base, int run_finalizers)
{
    int deleted = 0, i;

    for (i = 0; i < base->nactivequeues; ++i) {
        struct event_callback *evcb, *next;
        for (evcb = TAILQ_FIRST(&base->activequeues[i]); evcb;) {
            next = TAILQ_NEXT(evcb, evcb_active_next);
            deleted +=
                event_base_cancel_single_callback_(base, evcb, run_finalizers);
            evcb = next;
        }
    }

    {
        struct event_callback *evcb;
        while ((evcb = TAILQ_FIRST(&base->active_later_queue))) {
            deleted +=
                event_base_cancel_single_callback_(base, evcb, run_finalizers);
        }
    }

    return deleted;
}
```