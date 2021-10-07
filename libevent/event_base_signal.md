# event_base 如何管理 signal 事件
如 [signal 事件](./signal_event.md) 章节所所述，Libevent 将 signal 事件转换为 IO 事件处理 signal 事件。event_base 使用 event_signal_map 管理所有注册的 signal event。
```
struct event_base {
    // ...
    const struct eventop *evsigsel; // 信号 backend
    struct evsig_info sig; // signal 事件管理器
    // ...
    struct event_signal_map sigmap; // signo=>event*
    // ...
};
```

## event_signal_map
event_signal_map 用于 signo 到 event 的映射，其实是一个动态数组，数组下标就是 signo，其定义如下：
```
/// event_struct.h
LIST_HEAD (event_dlist, event); 

/// event-internal.h
/// signo -> list_of_events
struct event_signal_map {
    void **entries; // evmap_io* 或者 evmap_signal* 数组
    int nentries;   // 数组长度
};

struct evmap_signal {
    struct event_dlist events;
};

void
evmap_signal_initmap_(struct event_signal_map *ctx)
{
    ctx->nentries = 0;
    ctx->entries = NULL;
}
```
数组 entries 的元素为 evmap_signal 指针，是单向链表的头结点。

### GET_SIGNAL_SLOT\*()
GET_SIGNAL_SLOT() 用于访问 event_signal_map::entries[slot] 的元素。
```
#define GET_SIGNAL_SLOT(x, map, slot, type)         \
    (x) = (struct type *)((map)->entries[slot])
```
GET_SIGNAL_SLOT_AND_CTOR() 用于初始化 event_signal_map::entries[slot] 的元素。
```
#define GET_SIGNAL_SLOT_AND_CTOR(x, map, slot, type, ctor, fdinfo_len) \
    do {                                                               \
        if ((map)->entries[slot] == NULL) {                            \
            (map)->entries[slot] =                                     \
                mm_calloc(1,sizeof(struct type)+fdinfo_len);           \
            if (EVUTIL_UNLIKELY((map)->entries[slot] == NULL))         \
                return (-1);                                           \
            (ctor)((struct type *)(map)->entries[slot]);               \
        }                                                              \
        (x) = (struct type *)((map)->entries[slot]);                   \
    } while (0)
```

### evmap_make_space()
event_signal_map::entries 的下标代表 signo，当新增的 signo 大于已有所有的 signo，就必须扩充内存。evmap_signal_initmap_() 函数将 event_signal_map::nentries 初始化为 0，因此在第一次调用 evmap_make_space()，从 32 开始，以 2 倍增长方式，找到一个大于 slot 的值。
```
static int
evmap_make_space(struct event_signal_map *map, int slot, int msize)
{
    if (map->nentries <= slot) {
        int nentries = map->nentries ? map->nentries : 32;
        void **tmp;

        if (slot > INT_MAX / 2)
            return (-1);

        while (nentries <= slot)
            nentries <<= 1;

        if (nentries > INT_MAX / msize)
            return (-1);

        tmp = (void **)mm_realloc(map->entries, nentries * msize);
        if (tmp == NULL)
            return (-1);

        memset(&tmp[map->nentries], 0,
            (nentries - map->nentries) * msize);

        map->nentries = nentries;
        map->entries = tmp;
    }

    return (0);
}
```

## evmap_signal_*()
### evmap_signal_init()
evmap_signal_init() 初始化头结点 evmap_signal。
```
static void
evmap_signal_init(struct evmap_signal *entry)
{
    LIST_INIT(&entry->events);
}
```

### evmap_signal_add_()
evmap_signal_add_() 向 event_signal_map 添加一个 event。如果是一个全新的 signal 事件，调用 signal 后端。
```
int
evmap_signal_add_(struct event_base *base, int sig, struct event *ev)
{
    const struct eventop *evsel = base->evsigsel;
    struct event_signal_map *map = &base->sigmap;
    struct evmap_signal *ctx = NULL;

    if (sig < 0 || sig >= NSIG)
        return (-1);

    if (sig >= map->nentries) {
        if (evmap_make_space(
            map, sig, sizeof(struct evmap_signal *)) == -1)
            return (-1);
    }
    GET_SIGNAL_SLOT_AND_CTOR(ctx, map, sig, evmap_signal, evmap_signal_init,
        base->evsigsel->fdinfo_len);

    if (LIST_EMPTY(&ctx->events)) {
        if (evsel->add(base, ev->ev_fd, 0, EV_SIGNAL, NULL) == -1)
            return (-1);
    }

    LIST_INSERT_HEAD(&ctx->events, ev, ev_signal_next);

    return (1);
}
```

### evmap_signal_del_()
```
int
evmap_signal_del_(struct event_base *base, int sig, struct event *ev)
{
    const struct eventop *evsel = base->evsigsel;
    struct event_signal_map *map = &base->sigmap;
    struct evmap_signal *ctx;

    if (sig < 0 || sig >= map->nentries)
        return (-1);

    GET_SIGNAL_SLOT(ctx, map, sig, evmap_signal);

    LIST_REMOVE(ev, ev_signal_next);

    if (LIST_FIRST(&ctx->events) == NULL) {
        if (evsel->del(base, ev->ev_fd, 0, EV_SIGNAL, NULL) == -1)
            return (-1);
    }

    return (1);
}
```

### evmap_signal_clear_()
```
void
evmap_signal_clear_(struct event_signal_map *ctx)
{
    if (ctx->entries != NULL) {
        int i;
        for (i = 0; i < ctx->nentries; ++i) {
            if (ctx->entries[i] != NULL)
                mm_free(ctx->entries[i]);
        }
        mm_free(ctx->entries);
        ctx->entries = NULL;
    }
    ctx->nentries = 0;
}
```

### evmap_signal_active_()
```
void
evmap_signal_active_(struct event_base *base, evutil_socket_t sig, int ncalls)
{
    struct event_signal_map *map = &base->sigmap;
    struct evmap_signal *ctx;
    struct event *ev;

    if (sig < 0 || sig >= map->nentries)
        return;
    GET_SIGNAL_SLOT(ctx, map, sig, evmap_signal);

    if (!ctx)
        return;
    LIST_FOREACH(ev, &ctx->events, ev_signal_next)
        event_active_nolock_(ev, EV_SIGNAL, ncalls);
}
```