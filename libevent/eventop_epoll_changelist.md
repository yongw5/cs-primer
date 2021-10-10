# eventop 之 epoll-changelist

## changelist
event_base 中有一个 evnet_changlist 类型的 chagnelist 成员，是为了将多个 event 的注册或者删除操作合并成一次处理，减少系统调用的次数。
```
struct event_base {
    // ...
    struct event_changelist changelist;
    // ...
```

### event_changelist
event_changelist 是一个保存 evnet_change 的数组，其定义如下：
```
struct event_changelist {
    struct event_change *changes;
    int n_changes;     // 元素个数
    int changes_size;  // 数组总长度
};
```

### event_change
event_change 表示对 event 一次操作，宏定义 EV_CHANGE_* 定义了操作类型。
```
#define EV_CHANGE_ADD     0x01       // 添加
#define EV_CHANGE_DEL     0x02       // 删除
#define EV_CHANGE_SIGNAL  EV_SIGNAL  // signal 事件
#define EV_CHANGE_PERSIST EV_PERSIST // 永久事件
#define EV_CHANGE_ET      EV_ET      // 边缘触发

struct event_change {
    evutil_socket_t fd;      // fd 或者 signo
    short old_events;        // 原始注册的事件，EV_READ 或者 EV_WRITE 等

    ev_uint8_t read_change;  // EV_CHANGE_* 集合
    ev_uint8_t write_change; // EV_CHANGE_* 集合
    ev_uint8_t close_change; // EV_CHANGE_* 集合
};
```
如果是 signal 事件，read_change 被设置 EV_CHANGE_SIGNAL，write_change 不设置。

### event_changelist_get_or_construct()
event_changelist_get_or_construct() 函数用于在 changelist 构建一个 event_change 对象。
```
static struct event_change *
event_changelist_get_or_construct(struct event_changelist *changelist,
    evutil_socket_t fd,
    short old_events,
    struct event_changelist_fdinfo *fdinfo)
{
    struct event_change *change;

    if (fdinfo->idxplus1 == 0) { // 不存在
        int idx;
        EVUTIL_ASSERT(changelist->n_changes <= changelist->changes_size);

        if (changelist->n_changes == changelist->changes_size) {
            if (event_changelist_grow(changelist) < 0)
                return NULL;
        }

        idx = changelist->n_changes++;
        change = &changelist->changes[idx];
        fdinfo->idxplus1 = idx + 1;

        memset(change, 0, sizeof(struct event_change));
        change->fd = fd;
        change->old_events = old_events;
    } else {
        change = &changelist->changes[fdinfo->idxplus1 - 1];
        EVUTIL_ASSERT(change->fd == fd);
    }
    return change;
}
```

## epollops_changelist
epollops_changelist 是 epoll 结合 event_base::changelist 的实现。有时候，可以将多个对 epoll 的操作，合并成一次操作，可以减少对 epoll 系统调用。
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

### event_changelist_add_()
```
int
event_changelist_add_(struct event_base *base, evutil_socket_t fd, short old, short events,
    void *p)
{
    struct event_changelist *changelist = &base->changelist;
    struct event_changelist_fdinfo *fdinfo = p;
    struct event_change *change;
    ev_uint8_t evchange = EV_CHANGE_ADD | (events & (EV_ET|EV_PERSIST|EV_SIGNAL));

    event_changelist_check(base); // do-nothing

    change = event_changelist_get_or_construct(changelist, fd, old, fdinfo);
    if (!change)
        return -1;

    if (events & (EV_READ|EV_SIGNAL))
        change->read_change = evchange;
    if (events & EV_WRITE)
        change->write_change = evchange;
    if (events & EV_CLOSED)
        change->close_change = evchange;

    event_changelist_check(base); // do-nothing
    return (0);
}
```

### event_changelist_del_()
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

### event_changelist_remove_all_()
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