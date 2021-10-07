# event_base 初探
event_base 是 Libevent 的核心，负责 event 的注册于派发。event_base 包含了一组 event 并且可以探知那些 event 已经就绪，可以将其设置为 ACTIVE 状态。

event_base 包含一个“method”，或者“backend”，用来探知那些 event 就绪。这些方法和平台有关，不同的平台有不同的实现，Libevent 包含：
- select：Linux
- poll：Linux
- epoll：Linux
- kqueue：MacOS
- devpoll：Solaris
- evport：Win

可以调用 event_get_supported_methods() 函数获取当前品台支持的后端方法。
```
const char **event_get_supported_methods(void);
```

## 创建 event_base 结构
使用 Libevent，首先就必须创建一个 evnet_base 结构。

### event_base_new()
调用 event_base_new() 函数可以创建一个默认配置的 event_base 结构：检查环境变量然后返回指向一个 event_base 结构的指针。如果发生错误，返回 NULL。对于大多数情况，使用一个默认配置的 event_base 已经满足需求。
```
struct event_base *event_base_new(void);

void event_base_free(struct event_base* base);
```

### event_base_new_with_config()
如果需要对 event_base 做自主化配置，可以使用 event_config 结构设置配置参数。

event_config 定义如下：
```
/// event-internal.h
struct event_config_entry {
	TAILQ_ENTRY(event_config_entry) next;

	const char *avoid_method;
};

struct event_config {
	TAILQ_HEAD(event_configq, event_config_entry) entries; // method 黑名单

	int n_cpus_hint;                            // Win 平台使用，配置 iocp
	struct timeval max_dispatch_interval;       // 一次事件派发最长耗时
	int max_dispatch_callbacks;                 // 一次事件派发，cb 调用最大次数
	int limit_callbacks_after_prio;             // 优先级低于此，限制添加生效
	enum event_method_feature require_features; // EV_FEATURE_*
	enum event_base_config_flag flags;          // EVENT_BASE_FLAG_*
};
``` 
event_config 定义不对外公开，Libevent 提供一系列的接口函数获取和设置 event_config 变量。
```
struct event_config *event_config_new(void);
void event_config_free(struct event_config *cfg);

int event_config_avoid_method(struct event_config *cfg, const char *method);

enum event_method_feature {
    EV_FEATURE_ET = 0x01,
    EV_FEATURE_O1 = 0x02,
    EV_FEATURE_FDS = 0x04,
};
int event_config_require_features(struct event_config *cfg,
                                  enum event_method_feature feature);

enum event_base_config_flag {
    EVENT_BASE_FLAG_NOLOCK = 0x01,
    EVENT_BASE_FLAG_IGNORE_ENV = 0x02,
    EVENT_BASE_FLAG_STARTUP_IOCP = 0x04,
    EVENT_BASE_FLAG_NO_CACHE_TIME = 0x08,
    EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST = 0x10,
    EVENT_BASE_FLAG_PRECISE_TIMER = 0x20
};
int event_config_set_flag(struct event_config *cfg,
    enum event_base_config_flag flag);

int event_config_set_num_cpus_hint(struct event_config *cfg, int cpus);

int event_config_set_max_dispatch_interval(struct event_config *cfg,
    const struct timeval *max_interval, int max_callbacks,
    int min_priority);
```
event_config_new() 返回默认值为：
```
struct event_config *
event_config_new(void)
{
  struct event_config *cfg = mm_calloc(1, sizeof(*cfg));

  if (cfg == NULL)
    return (NULL);

  TAILQ_INIT(&cfg->entries);
  cfg->max_dispatch_interval.tv_sec = -1;
  cfg->max_dispatch_callbacks = INT_MAX;
  cfg->limit_callbacks_after_prio = 1;

  return (cfg);
}
```
event_config_require_features() 函数可以限制 event_base 使用的后端方法，可以设置的属性是：
- EV_FEATURE_ET：支持边缘触发；
- EV_FEATURE_O1：添加或者删除，以及将其标记为就绪，具有 O(1) 复杂度；
- EV_FEATURE_FDS：支持普通的文件描述符，而仅仅是 socket

event_config_set_flag() 函数可以对 event_base 做进一步限制，可以设置的值为：
- EVENT_BASE_FLAG_NOLOCK：不使用锁保护 event_base 的成员；
- EVENT_BASE_FLAG_IGNORE_ENV：不检查 EVENT_* 环境变量；
- EVENT_BASE_FLAG_STARTUP_IOCP：Win 平台使用，启用 IOCP
- EVENT_BASE_FLAG_NO_CACHE_TIME：不对时间进行缓存；
- EVENT_BASE_FLAG_EPOLL_USE_CHANGELIST：如果使用 epoll，使用 changelist 特性。epoll-changelist 尽量将多个 fd 操作合并为一个，可以避免不必要的系统调用。但是如果 fd 是使用 dup() 获取，会有 BUG；
- EVENT_BASE_FLAG_PRECISE_TIMER：使用精确时间，默认使用品台提供的最快时间接口；

当配置号 event_config，就可以调用 event_base_new_with_config() 函数创建 event_base 结构。
```
struct event_base *event_base_new_with_config(const struct event_config *cfg);
```
可以通过如下函数知道 event_base 使用的后端方法以及支持的特性。
```
const char *event_base_get_method(const struct event_base *base);
enum event_method_feature event_base_get_features(const struct event_base *base);
```

### event_reinit()
event_reinit() 用于解决 fork() 之后，event_base 重新初始化。主要 event_base 使用的所有 backend 在 fork() 调用后都会做清理，或者完全清理，有些重新初始化。
```
int event_reinit(struct event_base* base);
```

## 事件循环 evnet_loop
当 event_base 结构创建并且初始化，并且已经注册了某些 event，就可以开启事件循环，等待事件就绪。事件循环重复检查注册的 event 是否就绪（比如文件描述符可读或者可写，或者超时发生），一旦就绪，事件循环将 event 标记为 ACTIVE，然后执行其回调函数。

有关的接口如下：
```
#define EVLOOP_ONCE             0x01
#define EVLOOP_NONBLOCK         0x02
#define EVLOOP_NO_EXIT_ON_EMPTY 0x04
int event_base_loop(struct event_base *base, int flags);

int event_base_dispatch(struct event_base *base);

int event_base_loopexit(struct event_base *base, const struct timeval *tv);
int event_base_loopbreak(struct event_base *base);
int event_base_loopcontinue(struct event_base *base);
```
### event_base_loop()
默认情况下，event_base_loop() 执行事件循环直到没有 event。可以使用 EVLOOP_* 改变 event_base_loop() 的行为。EVLOOP_* 含义如下：
- EVLOOP_ONCE：事件循环等待 event 成为 ACTIVE，然后调用其回调函数，直到没有注册的 event 后返回；
- EVLOOP_NONBLOCK：事件循环不等待 event 成为 ACTIVE，而是检查是否有 event 已经成为 ACTIVE，如果有，就调用其回调函数，然后返回；
- EVLOOP_NO_EXIT_ON_EMPTY：即使没有注册的 event，事件循环也不停止；

事件循环处理逻辑如下：
```
while (any events are registered with the loop,
        or EVLOOP_NO_EXIT_ON_EMPTY was set) {

    if (EVLOOP_NONBLOCK was set, or any events are already active)
        If any registered events have triggered, mark them active.
    else
        Wait until at least one event has triggered, and mark it active.

    for (p = 0; p < n_priorities; ++p) {
       if (any event with priority of p is active) {
          Run all active events with priority of p.
          break; /* Do not run any events of a less important priority */
       }
    }

    if (EVLOOP_ONCE was set or EVLOOP_NONBLOCK was set)
       break;
}
```

### event_base_dispatch()
event_base_dispatch() 和 event_base_loop() 是相同的作用，只是不能配置 EVLOOP_*。因此，事件循环会在没有注册的 event 后，或者用户主动叫停后退出。
```
int
event_base_dispatch(struct event_base *event_base)
{
	return (event_base_loop(event_base, 0));
}
```

### event_base_loop{exit, break}
如果需要主动停止事件循环，可以使用如下两个接口。
```
int event_base_loopexit(struct event_base *base,
                        const struct timeval *tv);
int event_base_loopbreak(struct event_base *base);
```
event_base_loopexit() 告知 event_base 在 |tv| 时间后停止事件循环。如果 |tv| 为 NULL，事件循环会立即停止（TODODODO）。但是如果有 event 处于 ACTIVE 状态，并且事件循环正在调用其回调函数，则需要等待直到所有回调函数都执行才退出。

event_base_loopbreak() 告知 event_base 立即停止事件循环。和 event_base_loopexit(base, NULL) 不同的是：1）如果事件循环正在调用回调函数，event_base_loopbreak() 要求事件循环在处理完当前回调函数后，立即退出；2）TODO

### event_base_loopcontinue()
当终止了事件循环，event_base_loopcontinue() 可以是时间循环重新执行。