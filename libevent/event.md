# event 为何物
event 是 Libevent 的核心组建之一，表示一个事件。每个 event 都表示一组特定条件的集合，包括如下几种：
- 一个文件描述符可读或者可写；
- 一个文件描述符变得可读或者可写（边缘触发模式）；
- 计时器超时
- 信号发生
- 用户触发事件

## event 状态
Event 具有不同的状态。当用户调用 Libevent 的某个函数构建一个 event 对象，然后将其和一个 event_base 绑定后，event 变成 INITIALIZED 状态。此时，用户将 event 添加至事件循环中，event 变成 PENDING 状态；当 event 变成 PENDING 状态，一旦某个条件触发，event 变成 ACTIVE 状态，用户定义的回调函数将会被调用；如果 event 被配置成 PERSIST，event 继续是 PENDING 状态，否则其将被删除。

## event 接口
```
#define EV_TIMEOUT      0x01
#define EV_READ         0x02
#define EV_WRITE        0x04
#define EV_SIGNAL       0x08
#define EV_PERSIST      0x10
#define EV_ET           0x20
#define EV_FINALIZE     0x40
#define EV_CLOSED       0x80

typedef void (*event_callback_fn)(evutil_socket_t, short, void *);

struct event *event_new(struct event_base *base, evutil_socket_t fd,
    short what, event_callback_fn cb, void *arg);

void event_free(struct event *event);

void *event_self_cbarg();
```
### event_new()
event_new() 函数申请并且构造一个 event 变量，参数含义如下：
- base：需要关联的 event_base 变量；
- what：表示关注的事件，是上述 EV_* 宏的集合；
- fd：监控的文件描述符
- cb：事件发生后调用的回调函数；
- arg：调用 cb 时需要传入的参数；

如果参数不正确或者 Libevent 内部发生了错误，evnet_new() 返回 NULL。event_free() 用于释放 event 变量，并且在 event 处于 PENDING 或者 ACTIVE 状态时，调用 event_free() 也是安全的。

event_new() 返回的 event 变量处于 INITIALIZED 状态，如果需要将其设置为 PENDING 状态，可以调用 event_add() 函数。

### EV_* 宏定义
EV_* 宏定义了事件发生的条件：
- EV_TIMEOUT：表示一段时间后，事件成为 ACTIVE。不过，在 event_new() 函数中，EV_TIMEOUT 并没有使用：用户如果在调用 event_add() 时设置了超时时间，event_add() 函数自动设置 EV_TIMEOUT 标志。
- EV_READ：当文件描述符 fd 可读时，event 变成 ACTIVE 状态；
- EV_WRITE：当文件描述符 fd 可写时，event 变成 ACTIVE 状态；
- EV_SIGNAL：当信号发生，event 变成 ACTIVE 状态；
- EV_PERSIST：标志 event 为持久事件，即使 ACTIVE 后其回调函数被执行，event 不会被删除；
- EV_ET：边缘触发事件（需要后端支持）；
- EV_FINALIZE：event_del() 函数不阻塞；
- EV_CLOSED：连接 close 事件；

有关 EV_PERSIST 需要进一步说明：
1. 在默认情况下，当一个 event 成为 ACTIVE 后，其回调函数将被调用。在调用回调函数之前，evnet 就不再是 PENDING 状态（也不是 ACTIVE）。如果用户需要其再次成为 PENDING，需要在回调函数中调用 event_add() 函数。
1. 如果某个 event 被设置为 EV_PERSIST，即使其成为 ACTIVE 后其回调函数被调用，event 仍然处于 PENDING 状态。所以如果需要将其设置为 NON-PENDING，需要在回调函数中调用 event_del() 函数。
1. EV_TIMEOUT 和 EV_PERSIST 结合，超时时间会在回调函数调用时重置。比如有一个 event 被设置为 EV_READ|EV_PERSIST，并且超时时间为 5 秒，那么：1）fd 可读时成为 ACTIVE；2）上次 ACTIVE 后 5 秒。

### event_self_cbarg()
event_self_cbarg() 用于解决需要将 event 自身传入回调函数的情况。 event_self_cbarg() 返回一个 magic 指针（&event_self_cbarg_ptr_），指示 event_new() 函数，cb 的参数 arg 为创建的 event 变量。比如
```
static int n_calls = 0;

void cb_func(evutil_socket_t fd, short what, void *arg)
{
    struct event *me = arg;

    printf("cb_func called %d times so far.\n", ++n_calls);

    if (n_calls > 100)
       event_del(me);
}

void run(struct event_base *base)
{
    struct timeval one_sec = { 1, 0 };
    struct event *ev;
    /* We're going to set up a repeating timer to get called called 100
       times. */
    ev = event_new(base, -1, EV_PERSIST, cb_func, event_self_cbarg());
    event_add(ev, &one_sec);
    event_base_dispatch(base);
}
```

## 在栈上创建 event 变量
出于性能或者其他考虑，用户可能需要在栈空间创建 event 变量，Libevent 提供 event_assign() 函数，用于初始化 event 变量。
```
int event_assign(struct event *event, struct event_base *base,
    evutil_socket_t fd, short what,
    void (*callback)(evutil_socket_t, short, void *), void *arg);
```
除了第一个参数，event_assign() 和 event_new() 参数完全一样。

## 设置 event 状态
```
/// => PENDING
int event_add(struct event *ev, const struct timeval *tv);
/// => INITIALIZED
int event_del(struct event *ev);
/// => ACTIVE
void event_active(struct event *ev, int what, short ncalls);
```
event_add() 函数将 event 设置 PENDING 状态。如果参数 tv 为 NULL，event 将没有超时时间，否则，超时时间为 tv 表示的值。注意，tv 表示的是一段时间，而不是一个时间点。

如果 event 已经处于 PENDING 状态，event_add() 不改变 event 的状态，而是设置超时时间。如果 tv 为 NULL，event_add() 对 event 没有任何作用。

event_del() 函数移除 event 的 PENDING 状态和 ACTIVE 状态。如果 event 不是 PENDING 或者 ACTIVE 状态，没有作用。

event_active() 函数用于用户手动将 event 设置为 ACTIVE 状态，并且指定发生的条件。event_active() 不要求 event 首先处于 PENDING 状态，调用 event_active() 也不会将 event 设置为 PENDING 状态。

## event_remove_timer()
移除 event 的超时事件
```
int event_remove_timer(struct event *ev);
```