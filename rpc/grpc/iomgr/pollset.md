# pollset
## grpc_pollset_vtable
grpc_pollset_vtable 定义了一个 grpc_pollset 一组函数接口指针。grpc_pollset 在不同平台有不同的实现，通过 grpc_pollset_vtable 可以将所有平台的 grpc_pollset 实现统一起来。
```
typedef struct grpc_pollset_vtable {
  void (*global_init)(void);
  void (*global_shutdown)(void);
  void (*init)(grpc_pollset* pollset, gpr_mu** mu);
  void (*shutdown)(grpc_pollset* pollset, grpc_closure* closure);
  void (*destroy)(grpc_pollset* pollset);
  grpc_error_handle (*work)(grpc_pollset* pollset, grpc_pollset_worker** worker,
                            grpc_millis deadline);
  grpc_error_handle (*kick)(grpc_pollset* pollset,
                            grpc_pollset_worker* specific_worker);
  size_t (*pollset_size)(void);
} grpc_pollset_vtable;
```
通过调用 **grpc_set_pollset_vtable()** 函数，为全局变量 grpc_pollset_impl 指定具体的实现。
```
void grpc_set_pollset_vtable(grpc_pollset_vtable* vtable) {
  grpc_pollset_impl = vtable;
}
```
当 grpc_pollset_impl 赋值了具体的实现后，grpc_pollset 的操作函数调用 grpc_pollset_impl 指向的实现。
```
void grpc_pollset_global_init() { grpc_pollset_impl->global_init(); }

void grpc_pollset_global_shutdown() { grpc_pollset_impl->global_shutdown(); }

void grpc_pollset_init(grpc_pollset* pollset, gpr_mu** mu) {
  grpc_pollset_impl->init(pollset, mu);
}

void grpc_pollset_shutdown(grpc_pollset* pollset, grpc_closure* closure) {
  grpc_pollset_impl->shutdown(pollset, closure);
}

void grpc_pollset_destroy(grpc_pollset* pollset) {
  grpc_pollset_impl->destroy(pollset);
}

grpc_error_handle grpc_pollset_work(grpc_pollset* pollset,
                                    grpc_pollset_worker** worker,
                                    grpc_millis deadline) {
  return grpc_pollset_impl->work(pollset, worker, deadline);
}

grpc_error_handle grpc_pollset_kick(grpc_pollset* pollset,
                                    grpc_pollset_worker* specific_worker) {
  return grpc_pollset_impl->kick(pollset, specific_worker);
}

size_t grpc_pollset_size(void) { return grpc_pollset_impl->pollset_size(); }
```
grpc 并没有公开 grpc_pollset 的实现，只能结合 grpc_pollset_init() 和 grpc_pollset_size() 两个函数创建并初始化一个 grpc_pollset 对象。
```
/// example
grpc_mu* g_mu;
g_pollset = static_cast<grpc_pollset*>(gpr_malloc(grpc_pollset_size()));
grpc_pollset_init(g_pollset, &g_mu);
```
## grpc_pollset_worker
```
/// ev_epollex_linux.cc
struct pwlink {
  grpc_pollset_worker* next;
  grpc_pollset_worker* prev;
};
typedef enum { PWLINK_POLLABLE = 0, PWLINK_POLLSET, PWLINK_COUNT } pwlinks;

struct grpc_pollset_worker {
  bool kicked;
  bool initialized_cv;
#ifndef NDEBUG
/// ...
#endif
  gpr_cv cv;
  grpc_pollset* pollset;
  pollable* pollable_obj;

  pwlink links[PWLINK_COUNT];
};

struct grpc_pollset {
  gpr_mu mu;
  gpr_atm worker_count;
  gpr_atm active_pollable_type;
  pollable* active_pollable;
  bool kicked_without_poller;
  grpc_closure* shutdown_closure;
  bool already_shutdown;
  grpc_pollset_worker* root_worker;
  int containing_pollset_set_count;
};
```
### begin_worker()
```
/// ev_epollex_linux.cc
/* Return true if this thread should poll */
static bool begin_worker(grpc_pollset* pollset, grpc_pollset_worker* worker,
                         grpc_pollset_worker** worker_hdl,
                         grpc_millis deadline) {
  GPR_TIMER_SCOPE("begin_worker", 0);
  bool do_poll =
      (pollset->shutdown_closure == nullptr && !pollset->already_shutdown);
  gpr_atm_no_barrier_fetch_add(&pollset->worker_count, 1);
  if (worker_hdl != nullptr) *worker_hdl = worker;
  worker->initialized_cv = false;
  worker->kicked = false;
  worker->pollset = pollset;
  worker->pollable_obj =
      POLLABLE_REF(pollset->active_pollable, "pollset_worker");
  worker_insert(&pollset->root_worker, worker, PWLINK_POLLSET);
  gpr_mu_lock(&worker->pollable_obj->mu);
  if (!worker_insert(&worker->pollable_obj->root_worker, worker,
                     PWLINK_POLLABLE)) {
    worker->initialized_cv = true;
    gpr_cv_init(&worker->cv);
    gpr_mu_unlock(&pollset->mu);
/// trace...
    while (do_poll && worker->pollable_obj->root_worker != worker) {
      if (gpr_cv_wait(&worker->cv, &worker->pollable_obj->mu,
                      grpc_millis_to_timespec(deadline, GPR_CLOCK_REALTIME))) {
/// trace...
        do_poll = false;
      } else if (worker->kicked) {
/// trace...
        do_poll = false;
      } else if (GRPC_TRACE_FLAG_ENABLED(grpc_polling_trace) &&
                 worker->pollable_obj->root_worker != worker) {
/// trace...
      }
    }
    grpc_core::ExecCtx::Get()->InvalidateNow();
  } else {
    gpr_mu_unlock(&pollset->mu);
  }
  gpr_mu_unlock(&worker->pollable_obj->mu);

  return do_poll;
}
```
### end_worker()
```
/// ev_epollex_linux.cc
static void end_worker(grpc_pollset* pollset, grpc_pollset_worker* worker,
                       grpc_pollset_worker** /*worker_hdl*/) {
  GPR_TIMER_SCOPE("end_worker", 0);
  gpr_mu_lock(&pollset->mu);
  gpr_mu_lock(&worker->pollable_obj->mu);
  switch (worker_remove(&worker->pollable_obj->root_worker, worker,
                        PWLINK_POLLABLE)) {
    case WRR_NEW_ROOT: {
      // wakeup new poller
      grpc_pollset_worker* new_root = worker->pollable_obj->root_worker;
      GPR_ASSERT(new_root->initialized_cv);
      gpr_cv_signal(&new_root->cv);
      break;
    }
    case WRR_EMPTIED:
      if (pollset->active_pollable != worker->pollable_obj) {
        // pollable no longer being polled: flush events
        pollable_process_events(pollset, worker->pollable_obj, true);
      }
      break;
    case WRR_REMOVED:
      break;
  }
  gpr_mu_unlock(&worker->pollable_obj->mu);
  POLLABLE_UNREF(worker->pollable_obj, "pollset_worker");
  if (worker_remove(&pollset->root_worker, worker, PWLINK_POLLSET) ==
      WRR_EMPTIED) {
    pollset_maybe_finish_shutdown(pollset);
  }
  if (worker->initialized_cv) {
    gpr_cv_destroy(&worker->cv);
  }
  gpr_atm_no_barrier_fetch_add(&pollset->worker_count, -1);
}
```
## grpc_pollset
grpc_pollset 是一组 fd 的集合，在 Linux 平台上，grpc_pollset 定义如下：
```
/// ev_epollex_linux.cc
struct grpc_pollset {
  gpr_mu mu;
  gpr_atm worker_count;
  gpr_atm active_pollable_type;
  pollable* active_pollable;
  bool kicked_without_poller;
  grpc_closure* shutdown_closure;
  bool already_shutdown;
  grpc_pollset_worker* root_worker;
  int containing_pollset_set_count;
};
```
### pollset_init()
```
/// ev_epollex_linux.cc
static void pollset_init(grpc_pollset* pollset, gpr_mu** mu) {
  gpr_mu_init(&pollset->mu);
  gpr_atm_no_barrier_store(&pollset->worker_count, 0);
  gpr_atm_no_barrier_store(&pollset->active_pollable_type, PO_EMPTY);
  pollset->active_pollable = POLLABLE_REF(g_empty_pollable, "pollset");
  pollset->kicked_without_poller = false;
  pollset->shutdown_closure = nullptr;
  pollset->already_shutdown = false;
  pollset->root_worker = nullptr;
  pollset->containing_pollset_set_count = 0;
  *mu = &pollset->mu;
}
```
### pollset_shutdown()
```
/// ev_epollex_linux.cc
/* pollset->po.mu lock must be held by the caller before calling this */
static void pollset_shutdown(grpc_pollset* pollset, grpc_closure* closure) {
  GPR_TIMER_SCOPE("pollset_shutdown", 0);
  GPR_ASSERT(pollset->shutdown_closure == nullptr);
  pollset->shutdown_closure = closure;
  GRPC_LOG_IF_ERROR("pollset_shutdown", pollset_kick_all(pollset));
  pollset_maybe_finish_shutdown(pollset);
}
```
**pollset_maybe_finish_shutdown()** 将调用 ExecCtx::Run() 调度 shutdown_closure。
```
/// ev_epollex_linux.cc
/* pollset->mu must be held while calling this function */
static void pollset_maybe_finish_shutdown(grpc_pollset* pollset) {
/// trace...
  if (pollset->shutdown_closure != nullptr && pollset->root_worker == nullptr &&
      pollset->containing_pollset_set_count == 0) {
    GPR_TIMER_MARK("pollset_finish_shutdown", 0);
    grpc_core::ExecCtx::Run(DEBUG_LOCATION, pollset->shutdown_closure,
                            GRPC_ERROR_NONE);
    pollset->shutdown_closure = nullptr;
    pollset->already_shutdown = true;
  }
}
```
### pollset_destroy()
```
/// ev_epollex_linux.cc
/* pollset_shutdown is guaranteed to be called before pollset_destroy. */
static void pollset_destroy(grpc_pollset* pollset) {
  POLLABLE_UNREF(pollset->active_pollable, "pollset");
  pollset->active_pollable = nullptr;
  gpr_mu_destroy(&pollset->mu);
}
```
### pollset_work()
```
/// ev_epollex_linux.cc
/* pollset->mu lock must be held by the caller before calling this.
   The function pollset_work() may temporarily release the lock (pollset->po.mu)
   during the course of its execution but it will always re-acquire the lock and
   ensure that it is held by the time the function returns */
static grpc_error_handle pollset_work(grpc_pollset* pollset,
                                      grpc_pollset_worker** worker_hdl,
                                      grpc_millis deadline) {
  GPR_TIMER_SCOPE("pollset_work", 0);
#ifdef GRPC_EPOLLEX_CREATE_WORKERS_ON_HEAP
  grpc_pollset_worker* worker =
      (grpc_pollset_worker*)gpr_malloc(sizeof(*worker));
#define WORKER_PTR (worker)
#else
  grpc_pollset_worker worker;
#define WORKER_PTR (&worker)
#endif
#ifndef NDEBUG
  WORKER_PTR->originator = sys_gettid();
#endif
  if (GRPC_TRACE_FLAG_ENABLED(grpc_polling_trace)) {
    gpr_log(GPR_INFO,
            "PS:%p work hdl=%p worker=%p now=%" PRId64 " deadline=%" PRId64
            " kwp=%d pollable=%p",
            pollset, worker_hdl, WORKER_PTR, grpc_core::ExecCtx::Get()->Now(),
            deadline, pollset->kicked_without_poller, pollset->active_pollable);
  }
  static const char* err_desc = "pollset_work";
  grpc_error_handle error = GRPC_ERROR_NONE;
  if (pollset->kicked_without_poller) {
    pollset->kicked_without_poller = false;
  } else {
    if (begin_worker(pollset, WORKER_PTR, worker_hdl, deadline)) {
      gpr_tls_set(&g_current_thread_pollset, (intptr_t)pollset);
      gpr_tls_set(&g_current_thread_worker, (intptr_t)WORKER_PTR);
      if (WORKER_PTR->pollable_obj->event_cursor ==
          WORKER_PTR->pollable_obj->event_count) {
        append_error(&error, pollable_epoll(WORKER_PTR->pollable_obj, deadline),
                     err_desc);
      }
      append_error(
          &error,
          pollable_process_events(pollset, WORKER_PTR->pollable_obj, false),
          err_desc);
      grpc_core::ExecCtx::Get()->Flush();
      gpr_tls_set(&g_current_thread_pollset, 0);
      gpr_tls_set(&g_current_thread_worker, 0);
    }
    end_worker(pollset, WORKER_PTR, worker_hdl);
  }
#ifdef GRPC_EPOLLEX_CREATE_WORKERS_ON_HEAP
  gpr_free(worker);
#endif
#undef WORKER_PTR
  return error;
}
```
### pollset_kick()
```
/// ev_epollex_linux.cc
static grpc_error_handle pollset_kick(grpc_pollset* pollset,
                                      grpc_pollset_worker* specific_worker) {
  GPR_TIMER_SCOPE("pollset_kick", 0);
  GRPC_STATS_INC_POLLSET_KICK();
/// trace...
  if (specific_worker == nullptr) {
    if (gpr_tls_get(&g_current_thread_pollset) !=
        reinterpret_cast<intptr_t>(pollset)) {
      if (pollset->root_worker == nullptr) {
        if (GRPC_TRACE_FLAG_ENABLED(grpc_polling_trace)) {
          gpr_log(GPR_INFO, "PS:%p kicked_any_without_poller", pollset);
        }
        GRPC_STATS_INC_POLLSET_KICKED_WITHOUT_POLLER();
        pollset->kicked_without_poller = true;
        return GRPC_ERROR_NONE;
      } else {
        // We've been asked to kick a poller, but we haven't been told which one
        // ... any will do
        // We look at the pollset worker list because:
        // 1. the pollable list may include workers from other pollers, so we'd
        //    need to do an O(N) search
        // 2. we'd additionally need to take the pollable lock, which we've so
        //    far avoided
        // Now, we would prefer to wake a poller in cv_wait, and not in
        // epoll_wait (since the latter would imply the need to do an additional
        // wakeup)
        // We know that if a worker is at the root of a pollable, it's (likely)
        // also the root of a pollset, and we know that if a worker is NOT at
        // the root of a pollset, it's (likely) not at the root of a pollable,
        // so we take our chances and choose the SECOND worker enqueued against
        // the pollset as a worker that's likely to be in cv_wait
        return kick_one_worker(
            pollset->root_worker->links[PWLINK_POLLSET].next);
      }
    } else {
/// trace...
      GRPC_STATS_INC_POLLSET_KICK_OWN_THREAD();
      return GRPC_ERROR_NONE;
    }
  } else {
    return kick_one_worker(specific_worker);
  }
}
```
## grpc_pollset_set
```
/// ev_epollex_linux.cc
struct grpc_pollset_set {
  grpc_core::RefCount refs;
  gpr_mu mu;
  grpc_pollset_set* parent;

  size_t pollset_count;
  size_t pollset_capacity;
  grpc_pollset** pollsets;

  size_t fd_count;
  size_t fd_capacity;
  grpc_fd** fds;
};
```
## pollable
pollable 表示一个可以 poll 的对象，其核心拥有一个用于 poll 的 epoll_event 集合和一个用于唤醒 wakeup_fd。grpc 一共有三种 pollable：
- PO_EMPTY：fd 未添加到 pollset 的状态
- PO_FD：只包含一个 fd
- PO_MULTI：包含多个 fd

```
typedef enum { PO_MULTI, PO_FD, PO_EMPTY } pollable_type;

typedef struct pollable pollable;

/// A pollable is something that can be polled: it has an epoll set to poll on,
/// and a wakeup fd for kicks
/// There are three broad types:
///  - PO_EMPTY - the empty pollable, used before file descriptors are added to
///               a pollset
///  - PO_FD - a pollable containing only one FD - used to optimize single-fd
///            pollsets (which are common with synchronous api usage)
///  - PO_MULTI - a pollable containing many fds
struct pollable {
  pollable_type type;  // immutable
  grpc_core::RefCount refs;

  int epfd;
  grpc_wakeup_fd wakeup;

  // The following are relevant only for type PO_FD
  grpc_fd* owner_fd;       // Set to the owner_fd if the type is PO_FD
  gpr_mu owner_orphan_mu;  // Synchronizes access to owner_orphaned field
  bool owner_orphaned;     // Is the owner fd orphaned

  grpc_pollset_set* pollset_set;
  pollable* next;
  pollable* prev;

  gpr_mu mu;
  grpc_pollset_worker* root_worker;

  int event_cursor;
  int event_count;
  struct epoll_event events[MAX_EPOLL_EVENTS];
};
```