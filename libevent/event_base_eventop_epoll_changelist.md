# event_base 后端 eventop 之 epoll-changelist

## epollops_changelist
```
static const struct eventop epollops_changelist = {
    "epoll (with changelist)",
    epoll_init,
    event_changelist_add_,
    event_changelist_del_,
    epoll_dispatch,
    epoll_dealloc,
    1, /* need reinit */
    EV_FEATURE_ET|EV_FEATURE_O1| EARLY_CLOSE_IF_HAVE_RDHUP,
    EVENT_CHANGELIST_FDINFO_SIZE
};
```
## event_changelist_add_()
```
int
event_changelist_add_(struct event_base *base, evutil_socket_t fd, short old, short events,
    void *p)
{
    struct event_changelist *changelist = &base->changelist;
    struct event_changelist_fdinfo *fdinfo = p;
    struct event_change *change;
    ev_uint8_t evchange = EV_CHANGE_ADD | (events & (EV_ET|EV_PERSIST|EV_SIGNAL));

    event_changelist_check(base);

    change = event_changelist_get_or_construct(changelist, fd, old, fdinfo);
    if (!change)
        return -1;

    /* An add replaces any previous delete, but doesn't result in a no-op,
     * since the delete might fail (because the fd had been closed since
     * the last add, for instance. */

    if (events & (EV_READ|EV_SIGNAL))
        change->read_change = evchange;
    if (events & EV_WRITE)
        change->write_change = evchange;
    if (events & EV_CLOSED)
        change->close_change = evchange;

    event_changelist_check(base);
    return (0);
}
```
## event_changelist_del_()
```
int
event_changelist_del_(struct event_base *base, evutil_socket_t fd, short old, short events,
    void *p)
{
    struct event_changelist *changelist = &base->changelist;
    struct event_changelist_fdinfo *fdinfo = p;
    struct event_change *change;
    ev_uint8_t del = EV_CHANGE_DEL | (events & EV_ET);

    event_changelist_check(base);
    change = event_changelist_get_or_construct(changelist, fd, old, fdinfo);
    event_changelist_check(base);
    if (!change)
        return -1;

    /* A delete on an event set that doesn't contain the event to be
       deleted produces a no-op.  This effectively emoves any previous
       uncommitted add, rather than replacing it: on those platforms where
       "add, delete, dispatch" is not the same as "no-op, dispatch", we
       want the no-op behavior.

       If we have a no-op item, we could remove it it from the list
       entirely, but really there's not much point: skipping the no-op
       change when we do the dispatch later is far cheaper than rejuggling
       the array now.

       As this stands, it also lets through deletions of events that are
       not currently set.
     */

    if (events & (EV_READ|EV_SIGNAL)) {
        if (!(change->old_events & (EV_READ | EV_SIGNAL)))
            change->read_change = 0;
        else
            change->read_change = del;
    }
    if (events & EV_WRITE) {
        if (!(change->old_events & EV_WRITE))
            change->write_change = 0;
        else
            change->write_change = del;
    }
    if (events & EV_CLOSED) {
        if (!(change->old_events & EV_CLOSED))
            change->close_change = 0;
        else
            change->close_change = del;
    }

    event_changelist_check(base);
    return (0);
}
```

```

void
event_changelist_remove_all_(struct event_changelist *changelist,
    struct event_base *base)
{
    int i;

    event_changelist_check(base);

    for (i = 0; i < changelist->n_changes; ++i) {
        struct event_change *ch = &changelist->changes[i];
        struct event_changelist_fdinfo *fdinfo =
            event_change_get_fdinfo(base, ch);
        EVUTIL_ASSERT(fdinfo->idxplus1 == i + 1);
        fdinfo->idxplus1 = 0;
    }

    changelist->n_changes = 0;

    event_changelist_check(base);
}
```

```
int
event_changelist_add_(struct event_base *base, evutil_socket_t fd, short old, short events,
    void *p)
{
    struct event_changelist *changelist = &base->changelist;
    struct event_changelist_fdinfo *fdinfo = p;
    struct event_change *change;
    ev_uint8_t evchange = EV_CHANGE_ADD | (events & (EV_ET|EV_PERSIST|EV_SIGNAL));

    event_changelist_check(base);

    change = event_changelist_get_or_construct(changelist, fd, old, fdinfo);
    if (!change)
        return -1;

    /* An add replaces any previous delete, but doesn't result in a no-op,
     * since the delete might fail (because the fd had been closed since
     * the last add, for instance. */

    if (events & (EV_READ|EV_SIGNAL))
        change->read_change = evchange;
    if (events & EV_WRITE)
        change->write_change = evchange;
    if (events & EV_CLOSED)
        change->close_change = evchange;

    event_changelist_check(base);
    return (0);
}

int
event_changelist_del_(struct event_base *base, evutil_socket_t fd, short old, short events,
    void *p)
{
    struct event_changelist *changelist = &base->changelist;
    struct event_changelist_fdinfo *fdinfo = p;
    struct event_change *change;
    ev_uint8_t del = EV_CHANGE_DEL | (events & EV_ET);

    event_changelist_check(base);
    change = event_changelist_get_or_construct(changelist, fd, old, fdinfo);
    event_changelist_check(base);
    if (!change)
        return -1;

    /* A delete on an event set that doesn't contain the event to be
       deleted produces a no-op.  This effectively emoves any previous
       uncommitted add, rather than replacing it: on those platforms where
       "add, delete, dispatch" is not the same as "no-op, dispatch", we
       want the no-op behavior.

       If we have a no-op item, we could remove it it from the list
       entirely, but really there's not much point: skipping the no-op
       change when we do the dispatch later is far cheaper than rejuggling
       the array now.

       As this stands, it also lets through deletions of events that are
       not currently set.
     */

    if (events & (EV_READ|EV_SIGNAL)) {
        if (!(change->old_events & (EV_READ | EV_SIGNAL)))
            change->read_change = 0;
        else
            change->read_change = del;
    }
    if (events & EV_WRITE) {
        if (!(change->old_events & EV_WRITE))
            change->write_change = 0;
        else
            change->write_change = del;
    }
    if (events & EV_CLOSED) {
        if (!(change->old_events & EV_CLOSED))
            change->close_change = 0;
        else
            change->close_change = del;
    }

    event_changelist_check(base);
    return (0);
}
```