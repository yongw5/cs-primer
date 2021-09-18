# exec_ctx

## ExecCtx
**ExecCtx** 是 Execution Context 的缩写，记录了调用栈相关信息。ExecCtx 在 iomgr 入口处创建，是一个 thread-local 变量。

通常，在 API 入口或者线程入口函数实例化一个 ExecCtx 对象。使用 ExecCtx，应当注意：
1. 始终在栈上实例化 ExecCtx 对象，永远不要在堆上实例化
```
grpc_core::ExecCtx exec_ctx;
```
2. 不要将 ExecCtx 当作函数参数，永远通过 **Get()** 函数访问
```
grpc_core::ExecCtx::Get()
```

ExecCtx 职责如下：
- TODO
- TODO

ExecCtx 定义如下：
```
class ExecCtx {
 public:
  /** Default Constructor */

  ExecCtx() : flags_(GRPC_EXEC_CTX_FLAG_IS_FINISHED) {
    grpc_core::Fork::IncExecCtxCount();
    Set(this);
  }

  /** Parameterised Constructor */
  explicit ExecCtx(uintptr_t fl) : flags_(fl) {
    if (!(GRPC_EXEC_CTX_FLAG_IS_INTERNAL_THREAD & flags_)) {
      grpc_core::Fork::IncExecCtxCount();
    }
    Set(this);
  }

  /** Destructor */
  virtual ~ExecCtx() {
    flags_ |= GRPC_EXEC_CTX_FLAG_IS_FINISHED;
    Flush();
    Set(last_exec_ctx_);
    if (!(GRPC_EXEC_CTX_FLAG_IS_INTERNAL_THREAD & flags_)) {
      grpc_core::Fork::DecExecCtxCount();
    }
  }

  /** Disallow copy and assignment operators */
  ExecCtx(const ExecCtx&) = delete;
  ExecCtx& operator=(const ExecCtx&) = delete;

  unsigned starting_cpu() {
    if (starting_cpu_ == std::numeric_limits<unsigned>::max()) {
      starting_cpu_ = gpr_cpu_current_cpu();
    }
    return starting_cpu_;
  }

  struct CombinerData {
    /* currently active combiner: updated only via combiner.c */
    Combiner* active_combiner;
    /* last active combiner in the active combiner list */
    Combiner* last_combiner;
  };

  /** Only to be used by grpc-combiner code */
  CombinerData* combiner_data() { return &combiner_data_; }

  /** Return pointer to grpc_closure_list */
  grpc_closure_list* closure_list() { return &closure_list_; }

  /** Return flags */
  uintptr_t flags() { return flags_; }

  /** Checks if there is work to be done */
  bool HasWork() {
    return combiner_data_.active_combiner != nullptr ||
           !grpc_closure_list_empty(closure_list_);
  }

  /** Flush any work that has been enqueued onto this grpc_exec_ctx.
   *  Caller must guarantee that no interfering locks are held.
   *  Returns true if work was performed, false otherwise.
   */
  bool Flush();

  /** Returns true if we'd like to leave this execution context as soon as
   *  possible: useful for deciding whether to do something more or not
   *  depending on outside context.
   */
  bool IsReadyToFinish() {
    if ((flags_ & GRPC_EXEC_CTX_FLAG_IS_FINISHED) == 0) {
      if (CheckReadyToFinish()) {
        flags_ |= GRPC_EXEC_CTX_FLAG_IS_FINISHED;
        return true;
      }
      return false;
    } else {
      return true;
    }
  }

  /** Returns the stored current time relative to start if valid,
   *  otherwise refreshes the stored time, sets it valid and returns the new
   *  value.
   */
  grpc_millis Now();

  /** Invalidates the stored time value. A new time value will be set on calling
   *  Now().
   */
  void InvalidateNow() { now_is_valid_ = false; }

  /** To be used only by shutdown code in iomgr */
  void SetNowIomgrShutdown() {
    now_ = GRPC_MILLIS_INF_FUTURE;
    now_is_valid_ = true;
  }

  /** To be used only for testing.
   *  Sets the now value.
   */
  void TestOnlySetNow(grpc_millis new_val) {
    now_ = new_val;
    now_is_valid_ = true;
  }

  static void TestOnlyGlobalInit(gpr_timespec new_val);

  /** Global initialization for ExecCtx. Called by iomgr. */
  static void GlobalInit(void);

  /** Global shutdown for ExecCtx. Called by iomgr. */
  static void GlobalShutdown(void) { gpr_tls_destroy(&exec_ctx_); }

  /** Gets pointer to current exec_ctx. */
  static ExecCtx* Get() {
    return reinterpret_cast<ExecCtx*>(gpr_tls_get(&exec_ctx_));
  }

  static void Set(ExecCtx* exec_ctx) {
    gpr_tls_set(&exec_ctx_, reinterpret_cast<intptr_t>(exec_ctx));
  }

  static void Run(const DebugLocation& location, grpc_closure* closure,
                  grpc_error_handle error);

  static void RunList(const DebugLocation& location, grpc_closure_list* list);

 protected:
  /** Check if ready to finish. */
  virtual bool CheckReadyToFinish() { return false; }

  /** Disallow delete on ExecCtx. */
  static void operator delete(void* /* p */) { abort(); }

 private:
  /** Set exec_ctx_ to exec_ctx. */

  grpc_closure_list closure_list_ = GRPC_CLOSURE_LIST_INIT;
  CombinerData combiner_data_ = {nullptr, nullptr};
  uintptr_t flags_;

  unsigned starting_cpu_ = std::numeric_limits<unsigned>::max();

  bool now_is_valid_ = false;
  grpc_millis now_ = 0;

  GPR_TLS_CLASS_DECL(exec_ctx_);
  ExecCtx* last_exec_ctx_ = Get();
};
```
ExecCtx 主要的接口是 Run() 和 Flush()

### Run()
Run() 函数直接调用 exec_ctx_sched() 函数。
```
void ExecCtx::Run(const DebugLocation& location, grpc_closure* closure,
                  grpc_error_handle error) {
  (void)location;
  if (closure == nullptr) {
    GRPC_ERROR_UNREF(error);
    return;
  }
#ifndef NDEBUG
// ...
#endif
  exec_ctx_sched(closure, error);
}
```
exec_ctx_shed() 函数定义如下所示。
```
static void exec_ctx_sched(grpc_closure* closure, grpc_error_handle error) {
#if defined(GRPC_USE_EVENT_ENGINE) && \
// ...
#else
  grpc_closure_list_append(grpc_core::ExecCtx::Get()->closure_list(), closure,
                           error);
#endif
}
```
可以看到，Run() 函数并没有执行 grpc_closure，仅仅是将其加入 closure_list() 链表上。

### Flush()
Flush() 函数遍历 closure_list() 链表上所有，依次调用 exec_ctx_run() 函数。
```
bool ExecCtx::Flush() {
  bool did_something = false;
  GPR_TIMER_SCOPE("grpc_exec_ctx_flush", 0);
  for (;;) {
    if (!grpc_closure_list_empty(closure_list_)) {
      grpc_closure* c = closure_list_.head;
      closure_list_.head = closure_list_.tail = nullptr;
      while (c != nullptr) {
        grpc_closure* next = c->next_data.next;
        grpc_error_handle error = c->error_data.error;
        did_something = true;
        exec_ctx_run(c, error);
        c = next;
      }
    } else if (!grpc_combiner_continue_exec_ctx()) {
      break;
    }
  }
  GPR_ASSERT(combiner_data_.active_combiner == nullptr);
  return did_something;
}
```
exec_ctx_run() 函数直接调用 grpc_closure 中 cb 指向的函数。
```
static void exec_ctx_run(grpc_closure* closure, grpc_error_handle error) {
#ifndef NDEBUG
// ...
#endif
  closure->cb(closure->cb_arg, error);
#ifndef NDEBUG
// ...
#endif
  GRPC_ERROR_UNREF(error);
}
```
因此，调用 Flush()，ExecCtx 中所有 grpc_closure，都被执行。

## ApplicationCallbackExecCtx
ApplicationCallbackExecCtx 和 ExecCtx 功能相同，两种不同点在于：
- ApplicationCallbackExecCtx 保存用户级回调函数，而 ExecCtx 保存 grpc_closure。
- ApplicationCallbackExecCtx 没有类似于 ExecCtx::Flush() 方法，只有在析构时，其保存回调函数才被调用执行。
- 如果在线程栈空间创建了多个 ApplicationCallbackExecCtx，只有位于栈空间底部的哪个是有效的。

ApplicationCallbackExecCtx 定义如下：
```
class ApplicationCallbackExecCtx {
 public:
  /** Default Constructor */
  ApplicationCallbackExecCtx() { Set(this, flags_); }

  /** Parameterised Constructor */
  explicit ApplicationCallbackExecCtx(uintptr_t fl) : flags_(fl) {
    Set(this, flags_);
  }

  ~ApplicationCallbackExecCtx() {
    if (reinterpret_cast<ApplicationCallbackExecCtx*>(
            gpr_tls_get(&callback_exec_ctx_)) == this) {
      while (head_ != nullptr) {
        auto* f = head_;
        head_ = f->internal_next;
        if (f->internal_next == nullptr) {
          tail_ = nullptr;
        }
        (*f->functor_run)(f, f->internal_success);
      }
      gpr_tls_set(&callback_exec_ctx_, reinterpret_cast<intptr_t>(nullptr));
      if (!(GRPC_APP_CALLBACK_EXEC_CTX_FLAG_IS_INTERNAL_THREAD & flags_)) {
        grpc_core::Fork::DecExecCtxCount();
      }
    } else {
      GPR_DEBUG_ASSERT(head_ == nullptr);
      GPR_DEBUG_ASSERT(tail_ == nullptr);
    }
  }

  uintptr_t Flags() { return flags_; }

  static ApplicationCallbackExecCtx* Get() {
    return reinterpret_cast<ApplicationCallbackExecCtx*>(
        gpr_tls_get(&callback_exec_ctx_));
  }

  static void Set(ApplicationCallbackExecCtx* exec_ctx, uintptr_t flags) {
    if (Get() == nullptr) {
      if (!(GRPC_APP_CALLBACK_EXEC_CTX_FLAG_IS_INTERNAL_THREAD & flags)) {
        grpc_core::Fork::IncExecCtxCount();
      }
      gpr_tls_set(&callback_exec_ctx_, reinterpret_cast<intptr_t>(exec_ctx));
    }
  }

  static void Enqueue(grpc_completion_queue_functor* functor, int is_success) {
    functor->internal_success = is_success;
    functor->internal_next = nullptr;

    ApplicationCallbackExecCtx* ctx = Get();

    if (ctx->head_ == nullptr) {
      ctx->head_ = functor;
    }
    if (ctx->tail_ != nullptr) {
      ctx->tail_->internal_next = functor;
    }
    ctx->tail_ = functor;
  }

  /** Global initialization for ApplicationCallbackExecCtx. Called by init. */
  static void GlobalInit(void) { gpr_tls_init(&callback_exec_ctx_); }

  /** Global shutdown for ApplicationCallbackExecCtx. Called by init. */
  static void GlobalShutdown(void) { gpr_tls_destroy(&callback_exec_ctx_); }

  static bool Available() { return Get() != nullptr; }

 private:
  uintptr_t flags_{0u};
  grpc_completion_queue_functor* head_{nullptr};
  grpc_completion_queue_functor* tail_{nullptr};
  GPR_TLS_CLASS_DECL(callback_exec_ctx_);
};
```