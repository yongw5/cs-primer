# event 庐山真面目

## struct event
event 的定义对外可见，但是一般不直接使用。以下是 event 的完整定义：
```
// event_struct.h
#define EVLIST_TIMEOUT      0x01
#define EVLIST_INSERTED     0x02
#define EVLIST_SIGNAL       0x04
#define EVLIST_ACTIVE       0x08
#define EVLIST_INTERNAL     0x10
#define EVLIST_ACTIVE_LATER 0x20
#define EVLIST_FINALIZING   0x40
#define EVLIST_INIT         0x80
#define EVLIST_ALL          0xff

struct event_callback {
    TAILQ_ENTRY(event_callback) evcb_active_next;
    short evcb_flags;        // EVLIST_* 集合
    ev_uint8_t evcb_pri;     // 优先级，越小优先级越高
    ev_uint8_t evcb_closure; // cb 类型，EV_CLOSURE_*
        union {
        void (*evcb_callback)(evutil_socket_t, short, void *);
        void (*evcb_selfcb)(struct event_callback *, void *);
        void (*evcb_evfinalize)(struct event *, void *);
        void (*evcb_cbfinalize)(struct event_callback *, void *);
    } evcb_cb_union;
    void *evcb_arg;
};

struct event_base;
struct event {
    struct event_callback ev_evcallback;

    union {
        TAILQ_ENTRY(event) ev_next_with_common_timeout;
        int min_heap_idx;
    } ev_timeout_pos;
    evutil_socket_t ev_fd;

    struct event_base *ev_base;

    union {
        struct {
            LIST_ENTRY (event) ev_io_next;
            struct timeval ev_timeout; // 超时时间
        } ev_io;

        struct {
            LIST_ENTRY (event) ev_signal_next;
            short ev_ncalls; // 需要调用的次数
            short *ev_pncalls;
        } ev_signal;
    } ev_;

    short ev_events; // 关注的事件
    short ev_res;    // 发生的事件
    struct timeval ev_timeout; // 截止时间（deadline）
};
```
为了方便引用 event 的成员，Libevent 内部定义了一些辅助的宏定义（不对外开放）
```
// event-internal.h
#define ev_signal_next ev_.ev_signal.ev_signal_next
#define ev_io_next ev_.ev_io.ev_io_next
#define ev_io_timeout ev_.ev_io.ev_timeout

#define ev_ncalls ev_.ev_signal.ev_ncalls
#define ev_pncalls ev_.ev_signal.ev_pncalls

#define ev_pri ev_evcallback.evcb_pri
#define ev_flags ev_evcallback.evcb_flags
#define ev_closure ev_evcallback.evcb_closure
#define ev_callback ev_evcallback.evcb_cb_union.evcb_callback
#define ev_arg ev_evcallback.evcb_arg
```

### EVLIST_* 宏定义
EVLIST_* 标志 event 的状态或者属性，各个宏定义含义如下：
- EVLIST_TIMEOUT：表示 event 具有超时时间；
- EVLIST_INSERTED：表示 event 已经在 event_base 中，处于 PENDING 状态；
- EVLIST_SIGNAL：表示是个信号事件，libevent 中没有使用；
- EVLIST_ACTIVE：表示 event 处于 ACTIVE 状态；
- EVLIST_INTERNAL：内部使用
- EVLIST_ACTIVE_LATER：表示延迟激活；
- EVLIST_FINALIZING：表示 event 正在 finalzie；
- EVLIST_INIT：表示 event 处于 INITIALIZED 状态；

### EV_CLOSURE_* 宏定义
EV_CLOSURE_* 宏定义在内部使用，用户不能访问，表示 cb 的类型。可以是：
- EV_CLOSURE_EVENT：普通 io 或者 timeout 事件，使用 evcb_callback
- EV_CLOSURE_EVENT_SIGNAL：信号事件，使用 evcb_callback
- EV_CLOSURE_EVENT_PERSIST：永久，使用 evcb_callback
- EV_CLOSURE_CB_SELF：使用 evcb_selfcb
- EV_CLOSURE_CB_FINALIZE：使用 evcb_cbfinalize
- EV_CLOSURE_EVENT_FINALIZE：使用 evcb_evfinalize
- EV_CLOSURE_EVENT_FINALIZE_FREE：使用 evcb_evfinalize

### ev_timeout
event 中有两个 timeout 相关的参数。
```
event::ev_timeout           // ev_timeout
event::ev_.ev_io.ev_timeout // ev_io_timeout
```
ev_timeout 是普通超时时间，相当于 deadline；而 ev_io_timeout 用于 PERSIST 事件，是一段时间，表示经过该段时间后，超时发生，是真正的 timeout。

## event_new()
event_new() 函数在堆上申请一个 event 变量，然后调用 event_assign() 进一步处理。
```
struct event *
event_new(struct event_base *base, evutil_socket_t fd, short events,
    void (*cb)(evutil_socket_t, short, void *), void *arg)
{
    struct event *ev;
    ev = mm_malloc(sizeof(struct event));
    if (ev == NULL)
        return (NULL);
    if (event_assign(ev, base, fd, events, cb, arg) < 0) {
        mm_free(ev);
        return (NULL);
    }

    return (ev);
}
```
event_assign() 将完成 event 的初始化。
```
int
event_assign(struct event *ev, struct event_base *base, evutil_socket_t fd,
    short events, void (*callback)(evutil_socket_t, short, void *), void *arg)
{
    if (!base)
        base = current_base;
    if (arg == &event_self_cbarg_ptr_)
        arg = ev; // cb 的参数是 evnet 自己

    if (!(events & EV_SIGNAL))
        event_debug_assert_socket_nonblocking_(fd);
    event_debug_assert_not_added_(ev);

    ev->ev_base = base;

    ev->ev_callback = callback;
    ev->ev_arg = arg;
    ev->ev_fd = fd;
    ev->ev_events = events;
    ev->ev_res = 0;
    ev->ev_flags = EVLIST_INIT;
    ev->ev_ncalls = 0;
    ev->ev_pncalls = NULL;

    if (events & EV_SIGNAL) {
        if ((events & (EV_READ | EV_WRITE | EV_CLOSED)) != 0) {
            event_warnx("%s: EV_SIGNAL is not compatible with "
                        "EV_READ, EV_WRITE or EV_CLOSED",
                __func__);
            return -1;
        }
        ev->ev_closure = EV_CLOSURE_EVENT_SIGNAL;
    } else {
        if (events & EV_PERSIST) {
            evutil_timerclear(&ev->ev_io_timeout);
            ev->ev_closure = EV_CLOSURE_EVENT_PERSIST;
        } else {
            ev->ev_closure = EV_CLOSURE_EVENT;
        }
    }

    min_heap_elem_init_(ev);

    if (base != NULL) {
        /* by default, we put new events into the middle priority */
        ev->ev_pri = base->nactivequeues / 2;
    }

    event_debug_note_setup_(ev);

    return 0;
}
```

## event_free()
event_free() 用于释放 evnet 变量，首先调用 evnet_del() 将其从事件循环中删除，然后释放其内存。
```
void
event_free(struct event *ev)
{
    /* make sure that this event won't be coming back to haunt us. */
    event_del(ev);
    event_debug_note_teardown_(ev);
    mm_free(ev);
}
```

## event_{free_}finalize()
在创建 event 时，可以指定 EV_FINALIZE 标志避免死锁，因此也需要一个接口安全地删除一个 event，使得其回调函数不会在 event 销毁后被调用，event_finalize() 用于多线程环境中安全销毁一个 evnet 对象。

event_finalize() 被调用后，event_add() 和 event_active() 都不会在处理该 event，event_del() 也是如此。当然，不建议用于改变 event 的成员。
```
typedef void (*event_finalize_callback_fn)(struct event *, void *);

int
event_finalize(unsigned flags, struct event *ev, event_finalize_callback_fn cb)
{
    return event_finalize_impl_(flags, ev, cb);
}

int
event_free_finalize(
    unsigned flags, struct event *ev, event_finalize_callback_fn cb)
{
    return event_finalize_impl_(flags | EVENT_FINALIZE_FREE_, ev, cb);
}
```
两者都调用 event_finalize_impl_() 函数进行处理，其直接调用 event_finalize_nolock_() 函数。
```
static int
event_finalize_impl_(
    unsigned flags, struct event *ev, event_finalize_callback_fn cb)
{
    int r;
    struct event_base *base = ev->ev_base;
    if (EVUTIL_FAILURE_CHECK(!base)) {
        event_warnx("%s: event has no event_base set.", __func__);
        return -1;
    }

    EVBASE_ACQUIRE_LOCK(base, th_base_lock);
    r = event_finalize_nolock_(base, flags, ev, cb);
    EVBASE_RELEASE_LOCK(base, th_base_lock);
    return r;
}
```
event_callback_finalize_nolock_() 函数首先将 event 从事件循环中移除，然后修改回调函数为 EV_CLOSURE_CB_FINALIZE，然后将其设置为 ACTIVE 状态，使事件循环调用设置的回调函数。
```
void
event_callback_finalize_nolock_(struct event_base *base, unsigned flags,
    struct event_callback *evcb, void (*cb)(struct event_callback *, void *))
{
    struct event *ev = NULL;
    if (evcb->evcb_flags & EVLIST_INIT) {
        ev = event_callback_to_event(evcb);
        event_del_nolock_(ev, EVENT_DEL_NOBLOCK);
    } else {
        event_callback_cancel_nolock_(base, evcb, 0); /*XXX can this fail?*/
    }

    evcb->evcb_closure = EV_CLOSURE_CB_FINALIZE;
    evcb->evcb_cb_union.evcb_cbfinalize = cb;
    event_callback_activate_nolock_(base, evcb); /* XXX can this really fail?*/
    evcb->evcb_flags |= EVLIST_FINALIZING;
}
```