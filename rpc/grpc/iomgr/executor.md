# executor
## ExecutorType
```
enum class ExecutorType {
  DEFAULT = 0,
  RESOLVER,

  NUM_EXECUTORS  // Add new values above this
};
```
## ExecutorJobType
```
enum class ExecutorJobType {
  SHORT = 0,
  LONG,
  NUM_JOB_TYPES  // Add new values above this
};
```
## ThreadState
ThreadState 用于记录单个线程的状态。
```
struct ThreadState {
  gpr_mu mu;
  size_t id;         // For debugging purposes
  const char* name;  // Thread state name
  gpr_cv cv;
  grpc_closure_list elems;
  size_t depth;  // Number of closures in the closure list
  bool shutdown;
  bool queued_long_job;
  grpc_core::Thread thd;
};
```
queued_long_job 记录当前线程是否有 LongJob，如果存在，就不再向该线程添加任何任务。因为 LongJob 可能执行很长时间，会阻塞后续任务的执行。
## Executor
Executor 用于异步执行 grpc_closure，外部使用的都是 Executor 静态成员函数。当前有两个全局的 executor：DEFAULT 和 RESOLVER 类型。Executor 静态成员函数都是操作这两个全局的 executor。
```
class Executor {
 public:
// ...

  // Initialize ALL the executors
  static void InitAll();

  static void Run(grpc_closure* closure, grpc_error_handle error,
                  ExecutorType executor_type = ExecutorType::DEFAULT,
                  ExecutorJobType job_type = ExecutorJobType::SHORT);

  // Shutdown ALL the executors
  static void ShutdownAll();

  // Set the threading mode for ALL the executors
  static void SetThreadingAll(bool enable);

  // Set the threading mode for ALL the executors
  static void SetThreadingDefault(bool enable);

  // Return if a given executor is running in threaded mode (i.e if
  // SetThreading(true) was called previously on that executor)
  static bool IsThreaded(ExecutorType executor_type);

  // Return if the DEFAULT executor is threaded
  static bool IsThreadedDefault();

// ...
};
```
Executor 定义如下：
```
class Executor {
 public:
  explicit Executor(const char* executor_name);

  void Init();

  /** Is the executor multi-threaded? */
  bool IsThreaded() const;

  /* Enable/disable threading - must be called after Init and Shutdown(). Never
   * call SetThreading(false) in the middle of an application */
  void SetThreading(bool threading);

  /** Shutdown the executor, running all pending work as part of the call */
  void Shutdown();

  /** Enqueue the closure onto the executor. is_short is true if the closure is
   * a short job (i.e expected to not block and complete quickly) */
  void Enqueue(grpc_closure* closure, grpc_error_handle error, bool is_short);

// static member functions ...

 private:
  static size_t RunClosures(const char* executor_name, grpc_closure_list list);
  static void ThreadMain(void* arg);

  const char* name_;
  ThreadState* thd_state_;
  size_t max_threads_;
  gpr_atm num_threads_;
  gpr_spinlock adding_thread_lock_;
};
```
### Executor()
构造函数简单初始化相关变量，其中，max_threads_ 设置为 CPU 核心数量的两倍。
```
Executor::Executor(const char* name) : name_(name) {
  adding_thread_lock_ = GPR_SPINLOCK_STATIC_INITIALIZER;
  gpr_atm_rel_store(&num_threads_, 0);
  max_threads_ = GPR_MAX(1, 2 * gpr_cpu_num_cores());
}
```
### SetThreading()
SetThreading() 用于设置 Executor 是否创建线程执行 grpc_closure。如果参数 threading 为 true，SetThreading() 会提前创建 max_threads_ 个 ThreadState 对象，但是只会创建一个线程。当 threading 为 false，创建的所有线程会被销毁。
```
void Executor::SetThreading(bool threading) {
  gpr_atm curr_num_threads = gpr_atm_acq_load(&num_threads_);
  EXECUTOR_TRACE("(%s) SetThreading(%d) begin", name_, threading);

  if (threading) {
    if (curr_num_threads > 0) {
      EXECUTOR_TRACE("(%s) SetThreading(true). curr_num_threads > 0", name_);
      return;
    }

    GPR_ASSERT(num_threads_ == 0);
    gpr_atm_rel_store(&num_threads_, 1);
    thd_state_ = static_cast<ThreadState*>(
        gpr_zalloc(sizeof(ThreadState) * max_threads_));

    for (size_t i = 0; i < max_threads_; i++) {
      gpr_mu_init(&thd_state_[i].mu);
      gpr_cv_init(&thd_state_[i].cv);
      thd_state_[i].id = i;
      thd_state_[i].name = name_;
      thd_state_[i].thd = grpc_core::Thread();
      thd_state_[i].elems = GRPC_CLOSURE_LIST_INIT;
    }
// 创建一个线程，执行 ThreadMain() 函数
    thd_state_[0].thd =
        grpc_core::Thread(name_, &Executor::ThreadMain, &thd_state_[0]);
    thd_state_[0].thd.Start();
  } else {  // !threading
    if (curr_num_threads == 0) {
      EXECUTOR_TRACE("(%s) SetThreading(false). curr_num_threads == 0", name_);
      return;
    }
// 标记所有线程状态 shutdown 为 true
    for (size_t i = 0; i < max_threads_; i++) {
      gpr_mu_lock(&thd_state_[i].mu);
      thd_state_[i].shutdown = true;
      gpr_cv_signal(&thd_state_[i].cv);
      gpr_mu_unlock(&thd_state_[i].mu);
    }
// 然后销毁创建的线程
    /* Ensure no thread is adding a new thread. Once this is past, then no
     * thread will try to add a new one either (since shutdown is true) */
    gpr_spinlock_lock(&adding_thread_lock_);
    gpr_spinlock_unlock(&adding_thread_lock_);

    curr_num_threads = gpr_atm_no_barrier_load(&num_threads_);
    for (gpr_atm i = 0; i < curr_num_threads; i++) {
      thd_state_[i].thd.Join();
      EXECUTOR_TRACE("(%s) Thread %" PRIdPTR " of %" PRIdPTR " joined", name_,
                     i + 1, curr_num_threads);
    }

    gpr_atm_rel_store(&num_threads_, 0);
    for (size_t i = 0; i < max_threads_; i++) {
      gpr_mu_destroy(&thd_state_[i].mu);
      gpr_cv_destroy(&thd_state_[i].cv);
      RunClosures(thd_state_[i].name, thd_state_[i].elems);
    }

    gpr_free(thd_state_);

    // grpc_iomgr_shutdown_background_closure() will close all the registered
    // fds in the background poller, and wait for all pending closures to
    // finish. Thus, never call Executor::SetThreading(false) in the middle of
    // an application.
    // TODO(guantaol): create another method to finish all the pending closures
    // registered in the background poller by grpc_core::Executor.
    grpc_iomgr_shutdown_background_closure();
  }

  EXECUTOR_TRACE("(%s) SetThreading(%d) done", name_, threading);
}
```
**IsThread()** 用于判断 Executor 是否创建了线程，num_threads_ 大于 0，返回 true。
```
bool Executor::IsThreaded() const {
  return gpr_atm_acq_load(&num_threads_) > 0;
}
```
**ShutDown()** 函数直接调用 SetThreading() 函数
```
void Executor::Shutdown() { SetThreading(false); }
```
### ThreadMain()
在 SetThreading() 函数中可以看到，创建的线程执行 ThreadMain() 函数。逻辑如下：
1. 没有 grpc_closure 并且 shutdown 为 false，等待；
1. shutdown 为 true，返回；
1. 执行所有 grpc_closure；

```
void Executor::ThreadMain(void* arg) {
  ThreadState* ts = static_cast<ThreadState*>(arg);
  gpr_tls_set(&g_this_thread_state, reinterpret_cast<intptr_t>(ts));

  grpc_core::ExecCtx exec_ctx(GRPC_EXEC_CTX_FLAG_IS_INTERNAL_THREAD);

  size_t subtract_depth = 0;
  for (;;) {
    EXECUTOR_TRACE("(%s) [%" PRIdPTR "]: step (sub_depth=%" PRIdPTR ")",
                   ts->name, ts->id, subtract_depth);

    gpr_mu_lock(&ts->mu);
    ts->depth -= subtract_depth;
    // Wait for closures to be enqueued or for the executor to be shutdown
    while (grpc_closure_list_empty(ts->elems) && !ts->shutdown) {
      ts->queued_long_job = false;
      gpr_cv_wait(&ts->cv, &ts->mu, gpr_inf_future(GPR_CLOCK_MONOTONIC));
    }

    if (ts->shutdown) {
      EXECUTOR_TRACE("(%s) [%" PRIdPTR "]: shutdown", ts->name, ts->id);
      gpr_mu_unlock(&ts->mu);
      break;
    }

    GRPC_STATS_INC_EXECUTOR_QUEUE_DRAINED();
    grpc_closure_list closures = ts->elems;
    ts->elems = GRPC_CLOSURE_LIST_INIT;
    gpr_mu_unlock(&ts->mu);

    EXECUTOR_TRACE("(%s) [%" PRIdPTR "]: execute", ts->name, ts->id);

    grpc_core::ExecCtx::Get()->InvalidateNow();
    subtract_depth = RunClosures(ts->name, closures);
  }

  gpr_tls_set(&g_this_thread_state, reinterpret_cast<intptr_t>(nullptr));
}
```
**RunClosures()** 将逐一调用 Executor 中所有的 grpc_closure。
```
size_t Executor::RunClosures(const char* executor_name,
                             grpc_closure_list list) {
  size_t n = 0;

  // In the executor, the ExecCtx for the thread is declared in the executor
  // thread itself, but this is the point where we could start seeing
  // application-level callbacks. No need to create a new ExecCtx, though,
  // since there already is one and it is flushed (but not destructed) in this
  // function itself. The ApplicationCallbackExecCtx will have its callbacks
  // invoked on its destruction, which will be after completing any closures in
  // the executor's closure list (which were explicitly scheduled onto the
  // executor).
  grpc_core::ApplicationCallbackExecCtx callback_exec_ctx(
      GRPC_APP_CALLBACK_EXEC_CTX_FLAG_IS_INTERNAL_THREAD);

  grpc_closure* c = list.head;
  while (c != nullptr) {
    grpc_closure* next = c->next_data.next;
    grpc_error_handle error = c->error_data.error;
#ifndef NDEBUG
// ...
#else
    EXECUTOR_TRACE("(%s) run %p", executor_name, c);
#endif
    c->cb(c->cb_arg, error);
    GRPC_ERROR_UNREF(error);
    c = next;
    n++;
    grpc_core::ExecCtx::Get()->Flush();
  }

  return n;
}
```
### Enqueue()
Enqueue() 函数向 Executor 添加一个 grpc_closure，逻辑如下：
1. 如果 num_threads_ 为 0，添加 grpc_closure 到 ExecCtx；
1. 添加 grpc_closure 到后台 poller；
1. 从当前线程开始遍历，选择一个合适的线程将 grpc_closure 分配给该线程；
1. 尝试创建一个新的线程；

```
void Executor::Enqueue(grpc_closure* closure, grpc_error_handle error,
                       bool is_short) {
  bool retry_push;
  if (is_short) {
    GRPC_STATS_INC_EXECUTOR_SCHEDULED_SHORT_ITEMS();
  } else {
    GRPC_STATS_INC_EXECUTOR_SCHEDULED_LONG_ITEMS();
  }

  do {
    retry_push = false;
    size_t cur_thread_count =
        static_cast<size_t>(gpr_atm_acq_load(&num_threads_));

    // If the number of threads is zero(i.e either the executor is not threaded
    // or already shutdown), then queue the closure on the exec context itself
    if (cur_thread_count == 0) {
#ifndef NDEBUG
// ...
#else
      EXECUTOR_TRACE("(%s) schedule %p inline", name_, closure);
#endif
// 添加到 ExecCtx
      grpc_closure_list_append(grpc_core::ExecCtx::Get()->closure_list(),
                               closure, error);
      return;
    }
// 添加到后台 poller
    if (grpc_iomgr_add_closure_to_background_poller(closure, error)) {
      return;
    }

    ThreadState* ts =
        reinterpret_cast<ThreadState*>(gpr_tls_get(&g_this_thread_state));
    if (ts == nullptr) {
      ts = &thd_state_[GPR_HASH_POINTER(grpc_core::ExecCtx::Get(),
                                        cur_thread_count)];
    } else {
      GRPC_STATS_INC_EXECUTOR_SCHEDULED_TO_SELF();
    }

    ThreadState* orig_ts = ts;
    bool try_new_thread = false;
// 遍历所有线程，选择一个合适的线程
    for (;;) {
#ifndef NDEBUG
// ... 
#else
      EXECUTOR_TRACE("(%s) try to schedule %p (%s) to thread %" PRIdPTR, name_,
                     closure, is_short ? "short" : "long", ts->id);
#endif

      gpr_mu_lock(&ts->mu);
      if (ts->queued_long_job) {
        // if there's a long job queued, we never queue anything else to this
        // queue (since long jobs can take 'infinite' time and we need to
        // guarantee no starvation). Spin through queues and try again
        gpr_mu_unlock(&ts->mu);
        size_t idx = ts->id;
        ts = &thd_state_[(idx + 1) % cur_thread_count];
        if (ts == orig_ts) {
          // We cycled through all the threads. Retry enqueue again by creating
          // a new thread
          //
          // TODO (sreek): There is a potential issue here. We are
          // unconditionally setting try_new_thread to true here. What if the
          // executor is shutdown OR if cur_thread_count is already equal to
          // max_threads ?
          // (Fortunately, this is not an issue yet (as of july 2018) because
          // there is only one instance of long job in gRPC and hence we will
          // not hit this code path)
          retry_push = true;
          try_new_thread = true;
          break;
        }

        continue;  // Try the next thread-state
      }

      // == Found the thread state (i.e thread) to enqueue this closure! ==

      // Also, if this thread has been waiting for closures, wake it up.
      // - If grpc_closure_list_empty() is true and the Executor is not
      //   shutdown, it means that the thread must be waiting in ThreadMain()
      // - Note that gpr_cv_signal() won't immediately wakeup the thread. That
      //   happens after we release the mutex &ts->mu a few lines below
      if (grpc_closure_list_empty(ts->elems) && !ts->shutdown) {
        GRPC_STATS_INC_EXECUTOR_WAKEUP_INITIATED();
        gpr_cv_signal(&ts->cv);
      }

      grpc_closure_list_append(&ts->elems, closure, error);

      // If we already queued more than MAX_DEPTH number of closures on this
      // thread, use this as a hint to create more threads
      ts->depth++;
      try_new_thread = ts->depth > MAX_DEPTH &&
                       cur_thread_count < max_threads_ && !ts->shutdown;

      ts->queued_long_job = !is_short;

      gpr_mu_unlock(&ts->mu);
      break;
    }
// 尝试创建一个新的线程
    if (try_new_thread && gpr_spinlock_trylock(&adding_thread_lock_)) {
      cur_thread_count = static_cast<size_t>(gpr_atm_acq_load(&num_threads_));
      if (cur_thread_count < max_threads_) {
        // Increment num_threads (safe to do a store instead of a cas because we
        // always increment num_threads under the 'adding_thread_lock')
        gpr_atm_rel_store(&num_threads_, cur_thread_count + 1);

        thd_state_[cur_thread_count].thd = grpc_core::Thread(
            name_, &Executor::ThreadMain, &thd_state_[cur_thread_count]);
        thd_state_[cur_thread_count].thd.Start();
      }
      gpr_spinlock_unlock(&adding_thread_lock_);
    }

    if (retry_push) {
      GRPC_STATS_INC_EXECUTOR_PUSH_RETRIES();
    }
  } while (retry_push);
}
```
## global executors
```
namespace {

GPR_TLS_DECL(g_this_thread_state);

Executor* executors[static_cast<size_t>(ExecutorType::NUM_EXECUTORS)];

void default_enqueue_short(grpc_closure* closure, grpc_error_handle error) {
  executors[static_cast<size_t>(ExecutorType::DEFAULT)]->Enqueue(
      closure, error, true /* is_short */);
}

void default_enqueue_long(grpc_closure* closure, grpc_error_handle error) {
  executors[static_cast<size_t>(ExecutorType::DEFAULT)]->Enqueue(
      closure, error, false /* is_short */);
}

void resolver_enqueue_short(grpc_closure* closure, grpc_error_handle error) {
  executors[static_cast<size_t>(ExecutorType::RESOLVER)]->Enqueue(
      closure, error, true /* is_short */);
}

void resolver_enqueue_long(grpc_closure* closure, grpc_error_handle error) {
  executors[static_cast<size_t>(ExecutorType::RESOLVER)]->Enqueue(
      closure, error, false /* is_short */);
}

using EnqueueFunc = void (*)(grpc_closure* closure, grpc_error_handle error);

const EnqueueFunc
    executor_enqueue_fns_[static_cast<size_t>(ExecutorType::NUM_EXECUTORS)]
                         [static_cast<size_t>(ExecutorJobType::NUM_JOB_TYPES)] =
                             {{default_enqueue_short, default_enqueue_long},
                              {resolver_enqueue_short, resolver_enqueue_long}};

}  // namespace
```
### Run()
```
void Executor::Run(grpc_closure* closure, grpc_error_handle error,
                   ExecutorType executor_type, ExecutorJobType job_type) {
  executor_enqueue_fns_[static_cast<size_t>(executor_type)]
                       [static_cast<size_t>(job_type)](closure, error);
}
```