# eventop 之 epoll
## epollop
epollop 结构封装了 epoll，主要包括注册的 epoll_event 以及 epfd。
```
struct epollop {
    struct epoll_event *events;
    int nevents;
    int epfd;
#ifdef USING_TIMERFD
    int timerfd;
#endif
};
```

## epollops
epollops 是 eventop 类型的变量，定义了 epoll 的接口。
```
/// epoll.c
const struct eventop epollops = {
    "epoll",
    epoll_init,
    epoll_nochangelist_add,
    epoll_nochangelist_del,
    epoll_dispatch,
    epoll_dealloc,
    1, /* need reinit */
    EV_FEATURE_ET|EV_FEATURE_O1|EV_FEATURE_EARLY_CLOSE,
    0
};
```
### epoll_init()
```
static void *
epoll_init(struct event_base *base)
{
    int epfd = -1;
    struct epollop *epollop;

#ifdef EVENT__HAVE_EPOLL_CREATE1
    /* First, try the shiny new epoll_create1 interface, if we have it. */
    epfd = epoll_create1(EPOLL_CLOEXEC);
#endif
    if (epfd == -1) {
        /* Initialize the kernel queue using the old interface.  (The
        size field is ignored   since 2.6.8.) */
        if ((epfd = epoll_create(32000)) == -1) {
            if (errno != ENOSYS)
                event_warn("epoll_create");
            return (NULL);
        }
        evutil_make_socket_closeonexec(epfd);
    }

    if (!(epollop = mm_calloc(1, sizeof(struct epollop)))) {
        close(epfd);
        return (NULL);
    }
    epollop->epfd = epfd;
    epollop->events = mm_calloc(INITIAL_NEVENT, sizeof(struct epoll_event));
    if (epollop->events == NULL) {
        mm_free(epollop);
        close(epfd);
        return (NULL);
    }
    epollop->nevents = INITIAL_NEVENT;

    if ((base->flags & EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST) != 0 ||
        ((base->flags & EVENT_BASE_FLAG_IGNORE_ENV) == 0 &&
        evutil_getenv_("EVENT_EPOLL_USE_CHANGELIST") != NULL)) {
        // 使用 epoll-changelist
        base->evsel = &epollops_changelist;
    }

#ifdef USING_TIMERFD
    if ((base->flags & EVENT_BASE_FLAG_PRECISE_TIMER) &&
        base->monotonic_timer.monotonic_clock == CLOCK_MONOTONIC) {
        int fd;
        fd = epollop->timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK|TFD_CLOEXEC);
        if (epollop->timerfd >= 0) {
            struct epoll_event epev;
            memset(&epev, 0, sizeof(epev));
            epev.data.fd = epollop->timerfd;
            epev.events = EPOLLIN;
            if (epoll_ctl(epollop->epfd, EPOLL_CTL_ADD, fd, &epev) < 0) {
                event_warn("epoll_ctl(timerfd)");
                close(fd);
                epollop->timerfd = -1;
            }
        } else {
            if (errno != EINVAL && errno != ENOSYS) {
                event_warn("timerfd_create");
            }
            epollop->timerfd = -1;
        }
    } else {
        epollop->timerfd = -1;
    }
#endif

    evsig_init_(base);

    return (epollop);
}
```
### epoll_nochangelist_add()
```
static int
epoll_nochangelist_add(struct event_base *base, evutil_socket_t fd,
    short old, short events, void *p)
{
    struct event_change ch;
    ch.fd = fd;
    ch.old_events = old;
    ch.read_change = ch.write_change = ch.close_change = 0;
    if (events & EV_WRITE)
        ch.write_change = EV_CHANGE_ADD |
            (events & EV_ET);
    if (events & EV_READ)
        ch.read_change = EV_CHANGE_ADD |
            (events & EV_ET);
    if (events & EV_CLOSED)
        ch.close_change = EV_CHANGE_ADD |
            (events & EV_ET);

    return epoll_apply_one_change(base, base->evbase, &ch);
}
```
### epoll_nochangelist_del()
```
static int
epoll_nochangelist_del(struct event_base *base, evutil_socket_t fd,
    short old, short events, void *p)
{
    struct event_change ch;
    ch.fd = fd;
    ch.old_events = old;
    ch.read_change = ch.write_change = ch.close_change = 0;
    if (events & EV_WRITE)
        ch.write_change = EV_CHANGE_DEL |
            (events & EV_ET);
    if (events & EV_READ)
        ch.read_change = EV_CHANGE_DEL |
            (events & EV_ET);
    if (events & EV_CLOSED)
        ch.close_change = EV_CHANGE_DEL |
            (events & EV_ET);

    return epoll_apply_one_change(base, base->evbase, &ch);
}
```
### epoll_dispatch()
```
static int
epoll_dispatch(struct event_base *base, struct timeval *tv)
{
    struct epollop *epollop = base->evbase;
    struct epoll_event *events = epollop->events;
    int i, res;
    long timeout = -1;

#ifdef USING_TIMERFD
    if (epollop->timerfd >= 0) {
        struct itimerspec is;
        is.it_interval.tv_sec = 0;
        is.it_interval.tv_nsec = 0;
        if (tv == NULL) {
            /* No timeout; disarm the timer. */
            is.it_value.tv_sec = 0;
            is.it_value.tv_nsec = 0;
        } else {
            if (tv->tv_sec == 0 && tv->tv_usec == 0) {
                /* we need to exit immediately; timerfd can't
                 * do that. */
                timeout = 0;
            }
            is.it_value.tv_sec = tv->tv_sec;
            is.it_value.tv_nsec = tv->tv_usec * 1000;
        }
        if (timerfd_settime(epollop->timerfd, 0, &is, NULL) < 0) {
            event_warn("timerfd_settime");
        }
    } else
#endif
    if (tv != NULL) {
        timeout = evutil_tv_to_msec_(tv);
        if (timeout < 0 || timeout > MAX_EPOLL_TIMEOUT_MSEC) {
            timeout = MAX_EPOLL_TIMEOUT_MSEC;
        }
    }

    epoll_apply_changes(base);
    event_changelist_remove_all_(&base->changelist, base);

    EVBASE_RELEASE_LOCK(base, th_base_lock);

    res = epoll_wait(epollop->epfd, events, epollop->nevents, timeout);

    EVBASE_ACQUIRE_LOCK(base, th_base_lock);

    if (res == -1) {
        if (errno != EINTR) {
            event_warn("epoll_wait");
            return (-1);
        }
        return (0);
    }

    event_debug(("%s: epoll_wait reports %d", __func__, res));
    EVUTIL_ASSERT(res <= epollop->nevents);

    for (i = 0; i < res; i++) {
        int what = events[i].events;
        short ev = 0;
#ifdef USING_TIMERFD
        if (events[i].data.fd == epollop->timerfd)
            continue;
#endif

        if (what & EPOLLERR) {
            ev = EV_READ | EV_WRITE;
        } else if ((what & EPOLLHUP) && !(what & EPOLLRDHUP)) {
            ev = EV_READ | EV_WRITE;
        } else {
            if (what & EPOLLIN)
                ev |= EV_READ;
            if (what & EPOLLOUT)
                ev |= EV_WRITE;
            if (what & EPOLLRDHUP)
                ev |= EV_CLOSED;
        }

        if (!ev)
            continue;

        evmap_io_active_(base, events[i].data.fd, ev | EV_ET);
    }

    if (res == epollop->nevents && epollop->nevents < MAX_NEVENT) {
        int new_nevents = epollop->nevents * 2;
        struct epoll_event *new_events;

        new_events = mm_realloc(epollop->events,
            new_nevents * sizeof(struct epoll_event));
        if (new_events) {
            epollop->events = new_events;
            epollop->nevents = new_nevents;
        }
    }

    return (0);
}
```
## epoll_apply_one_change()
```
static int
epoll_apply_one_change(struct event_base *base,
    struct epollop *epollop,
    const struct event_change *ch)
{
    struct epoll_event epev;
    int op, events = 0;
    int idx;

    idx = EPOLL_OP_TABLE_INDEX(ch);
    op = epoll_op_table[idx].op;
    events = epoll_op_table[idx].events;

    if (!events) {
        EVUTIL_ASSERT(op == 0);
        return 0;
    }

    if ((ch->read_change|ch->write_change|ch->close_change) & EV_CHANGE_ET)
        events |= EPOLLET;

    memset(&epev, 0, sizeof(epev));
    epev.data.fd = ch->fd;
    epev.events = events;
    if (epoll_ctl(epollop->epfd, op, ch->fd, &epev) == 0) {
        event_debug((PRINT_CHANGES(op, epev.events, ch, "okay")));
        return 0;
    }

    switch (op) {
    case EPOLL_CTL_MOD:
        if (errno == ENOENT) {
            /* If a MOD operation fails with ENOENT, the
             * fd was probably closed and re-opened.  We
             * should retry the operation as an ADD.
             */
            if (epoll_ctl(epollop->epfd, EPOLL_CTL_ADD, ch->fd, &epev) == -1) {
                event_warn("Epoll MOD(%d) on %d retried as ADD; that failed too",
                    (int)epev.events, ch->fd);
                return -1;
            } else {
                event_debug(("Epoll MOD(%d) on %d retried as ADD; succeeded.",
                    (int)epev.events,
                    ch->fd));
                return 0;
            }
        }
        break;
    case EPOLL_CTL_ADD:
        if (errno == EEXIST) {
            /* If an ADD operation fails with EEXIST,
             * either the operation was redundant (as with a
             * precautionary add), or we ran into a fun
             * kernel bug where using dup*() to duplicate the
             * same file into the same fd gives you the same epitem
             * rather than a fresh one.  For the second case,
             * we must retry with MOD. */
            if (epoll_ctl(epollop->epfd, EPOLL_CTL_MOD, ch->fd, &epev) == -1) {
                event_warn("Epoll ADD(%d) on %d retried as MOD; that failed too",
                    (int)epev.events, ch->fd);
                return -1;
            } else {
                event_debug(("Epoll ADD(%d) on %d retried as MOD; succeeded.",
                    (int)epev.events,
                    ch->fd));
                return 0;
            }
        }
        break;
    case EPOLL_CTL_DEL:
        if (errno == ENOENT || errno == EBADF || errno == EPERM) {
            /* If a delete fails with one of these errors,
             * that's fine too: we closed the fd before we
             * got around to calling epoll_dispatch. */
            event_debug(("Epoll DEL(%d) on fd %d gave %s: DEL was unnecessary.",
                (int)epev.events,
                ch->fd,
                strerror(errno)));
            return 0;
        }
        break;
    default:
        break;
    }

    event_warn(PRINT_CHANGES(op, epev.events, ch, "failed"));
    return -1;
}
```
epoll_apply_changes() 直接调用 epoll_apply_one_change() 函数。
```
static int
epoll_apply_changes(struct event_base *base)
{
    struct event_changelist *changelist = &base->changelist;
    struct epollop *epollop = base->evbase;
    struct event_change *ch;

    int r = 0;
    int i;

    for (i = 0; i < changelist->n_changes; ++i) {
        ch = &changelist->changes[i];
        if (epoll_apply_one_change(base, epollop, ch) < 0)
            r = -1;
    }

    return (r);
}
```