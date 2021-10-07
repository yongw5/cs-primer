# event_base 如何管理 io 事件
event_base 使用 event_io_map 管理所有的 IO 有关的 event，使用 evcallback_list 管理 ACTIVE 状态的 event。
```
struct event_base {
    // ...
    struct event_io_map io; // fd=>event*
    // ...
};
```

## event_io_map
在 Linux 系统中，event_io_map 就是 event_signal_map
```
#ifdef _WIN32
#define EVMAP_USE_HT
#endif

#ifdef EVMAP_USE_HT
#define HT_NO_CACHE_HASH_VALUES
#include "ht-internal.h"
struct event_map_entry;
HT_HEAD(event_io_map, event_map_entry);
#else
#define event_io_map event_signal_map // Linux
#endif
```
entries 保存的是指向 evmap_io 或者 evmap_signal 的指针。
```
/// event_struct.h
LIST_HEAD (event_dlist, event); 

/// evmap.c
struct evmap_io {
    struct event_dlist events;
    ev_uint16_t nread;
    ev_uint16_t nwrite;
    ev_uint16_t nclose;
};

#ifndef EVMAP_USE_HT
#define GET_IO_SLOT(x,map,slot,type) GET_SIGNAL_SLOT(x,map,slot,type)
#define GET_IO_SLOT_AND_CTOR(x,map,slot,type,ctor,fdinfo_len)   \
    GET_SIGNAL_SLOT_AND_CTOR(x,map,slot,type,ctor,fdinfo_len)
#define FDINFO_OFFSET sizeof(struct evmap_io)

void
evmap_io_initmap_(struct event_io_map* ctx)
{
    evmap_signal_initmap_(ctx);
}
#endif
```

## evmap_io_*()
### evmap_io_init()
```
static void
evmap_io_init(struct evmap_io *entry)
{
    LIST_INIT(&entry->events);
    entry->nread = 0;
    entry->nwrite = 0;
    entry->nclose = 0;
}
```

### evmap_io_add_()
```
int
evmap_io_add_(struct event_base *base, evutil_socket_t fd, struct event *ev)
{
    const struct eventop *evsel = base->evsel;
    struct event_io_map *io = &base->io;
    struct evmap_io *ctx = NULL;
    int nread, nwrite, nclose, retval = 0;
    short res = 0, old = 0;
    struct event *old_ev;

    EVUTIL_ASSERT(fd == ev->ev_fd);

    if (fd < 0)
        return 0;

#ifndef EVMAP_USE_HT
    if (fd >= io->nentries) {
        if (evmap_make_space(io, fd, sizeof(struct evmap_io *)) == -1)
            return (-1);
    }
#endif
    GET_IO_SLOT_AND_CTOR(ctx, io, fd, evmap_io, evmap_io_init,
                         evsel->fdinfo_len);

    nread = ctx->nread;
    nwrite = ctx->nwrite;
    nclose = ctx->nclose;

    if (nread)
        old |= EV_READ;
    if (nwrite)
        old |= EV_WRITE;
    if (nclose)
        old |= EV_CLOSED;

    if (ev->ev_events & EV_READ) {
        if (++nread == 1)
            res |= EV_READ;
    }
    if (ev->ev_events & EV_WRITE) {
        if (++nwrite == 1)
            res |= EV_WRITE;
    }
    if (ev->ev_events & EV_CLOSED) {
        if (++nclose == 1)
            res |= EV_CLOSED;
    }
    if (EVUTIL_UNLIKELY(nread > 0xffff || nwrite > 0xffff || nclose > 0xffff)) {
        // ...
    }
    if (EVENT_DEBUG_MODE_IS_ON() &&
        (old_ev = LIST_FIRST(&ctx->events)) &&
        // ...
    }

    if (res) {
        void *extra = ((char*)ctx) + sizeof(struct evmap_io);
        if (evsel->add(base, ev->ev_fd,
            old, (ev->ev_events & EV_ET) | res, extra) == -1)
            return (-1);
        retval = 1;
    }

    ctx->nread = (ev_uint16_t) nread;
    ctx->nwrite = (ev_uint16_t) nwrite;
    ctx->nclose = (ev_uint16_t) nclose;
    LIST_INSERT_HEAD(&ctx->events, ev, ev_io_next);

    return (retval);
}
```
### evmap_io_del_()
```
int
evmap_io_del_(struct event_base *base, evutil_socket_t fd, struct event *ev)
{
    const struct eventop *evsel = base->evsel;
    struct event_io_map *io = &base->io;
    struct evmap_io *ctx;
    int nread, nwrite, nclose, retval = 0;
    short res = 0, old = 0;

    if (fd < 0)
        return 0;

    EVUTIL_ASSERT(fd == ev->ev_fd);

#ifndef EVMAP_USE_HT
    if (fd >= io->nentries)
        return (-1);
#endif

    GET_IO_SLOT(ctx, io, fd, evmap_io);

    nread = ctx->nread;
    nwrite = ctx->nwrite;
    nclose = ctx->nclose;

    if (nread)
        old |= EV_READ;
    if (nwrite)
        old |= EV_WRITE;
    if (nclose)
        old |= EV_CLOSED;

    if (ev->ev_events & EV_READ) {
        if (--nread == 0)
            res |= EV_READ;
        EVUTIL_ASSERT(nread >= 0);
    }
    if (ev->ev_events & EV_WRITE) {
        if (--nwrite == 0)
            res |= EV_WRITE;
        EVUTIL_ASSERT(nwrite >= 0);
    }
    if (ev->ev_events & EV_CLOSED) {
        if (--nclose == 0)
            res |= EV_CLOSED;
        EVUTIL_ASSERT(nclose >= 0);
    }

    if (res) {
        void *extra = ((char*)ctx) + sizeof(struct evmap_io);
        if (evsel->del(base, ev->ev_fd,
            old, (ev->ev_events & EV_ET) | res, extra) == -1) {
            retval = -1;
        } else {
            retval = 1;
        }
    }

    ctx->nread = nread;
    ctx->nwrite = nwrite;
    ctx->nclose = nclose;
    LIST_REMOVE(ev, ev_io_next);

    return (retval);
}
```

### evmap_io_clear_()
```
void
evmap_io_clear_(struct event_io_map* ctx)
{
    evmap_signal_clear_(ctx);
}
#endif
```

### evmap_io_active_()
```
void
evmap_io_active_(struct event_base *base, evutil_socket_t fd, short events)
{
    struct event_io_map *io = &base->io;
    struct evmap_io *ctx;
    struct event *ev;

#ifndef EVMAP_USE_HT
    if (fd < 0 || fd >= io->nentries)
        return;
#endif
    GET_IO_SLOT(ctx, io, fd, evmap_io);

    if (NULL == ctx)
        return;
    LIST_FOREACH(ev, &ctx->events, ev_io_next) {
        if (ev->ev_events & (events & ~EV_ET))
            event_active_nolock_(ev, ev->ev_events & events, 1);
    }
}
```