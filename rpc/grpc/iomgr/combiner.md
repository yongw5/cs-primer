# combiner
## Combiner
Combiner 适用于需要顺序执行的 grpc_closure。具有如下性质：
1. 顺序执行 grpc_closure；
1. 添加 grpc_closure 不会阻塞；
1. 允许在其他线程执行 grpc_closure；
1. 调用 Run() 并不会立即执行 grpc_closure；

Combiner 定义如下：
```
namespace grpc_core {
// TODO(yashkt) : Remove this class and replace it with a class that does not
// use ExecCtx
class Combiner {
 public:
  void Run(grpc_closure* closure, grpc_error_handle error);
  // TODO(yashkt) : Remove this method
  void FinallyRun(grpc_closure* closure, grpc_error_handle error);
  Combiner* next_combiner_on_this_exec_ctx = nullptr;
  MultiProducerSingleConsumerQueue queue;
  // either:
  // a pointer to the initiating exec ctx if that is the only exec_ctx that has
  // ever queued to this combiner, or NULL. If this is non-null, it's not
  // dereferencable (since the initiating exec_ctx may have gone out of scope)
  gpr_atm initiating_exec_ctx_or_null;
  // state is:
  // lower bit - zero if orphaned (STATE_UNORPHANED)
  // other bits - number of items queued on the lock (STATE_ELEM_COUNT_LOW_BIT)
  gpr_atm state;
  bool time_to_execute_final_list = false;
  grpc_closure_list final_list;
  grpc_closure offload;
  gpr_refcount refs;
};
}  // namespace grpc_core
```
**Run() 和 FinallyRun()** 函数分别调用 **combiner_exec()** 和 **combiner_finally_exec()** 函数。
```
namespace grpc_core {
void Combiner::Run(grpc_closure* closure, grpc_error_handle error) {
  combiner_exec(this, closure, error);
}

void Combiner::FinallyRun(grpc_closure* closure, grpc_error_handle error) {
  combiner_finally_exec(this, closure, error);
}
}  // namespace grpc_core
```
## grpc_combiner_create()
Combiner 通过 grpc_combiner_create() 函数创建。
```
grpc_core::Combiner* grpc_combiner_create(void) {
  grpc_core::Combiner* lock = new grpc_core::Combiner();
  gpr_ref_init(&lock->refs, 1);
  gpr_atm_no_barrier_store(&lock->state, STATE_UNORPHANED);
  grpc_closure_list_init(&lock->final_list);
  GRPC_CLOSURE_INIT(&lock->offload, offload, lock, nullptr);
  GRPC_COMBINER_TRACE(gpr_log(GPR_INFO, "C:%p create", lock));
  return lock;
}
```
Combiner 通过引用计数管理
```
void grpc_combiner_unref(grpc_core::Combiner* lock GRPC_COMBINER_DEBUG_ARGS) {
  GRPC_COMBINER_DEBUG_SPAM("UNREF", -1);
  if (gpr_unref(&lock->refs)) {
    start_destroy(lock);
  }
}

grpc_core::Combiner* grpc_combiner_ref(
    grpc_core::Combiner* lock GRPC_COMBINER_DEBUG_ARGS) {
  GRPC_COMBINER_DEBUG_SPAM("  REF", 1);
  gpr_ref(&lock->refs);
  return lock;
}
```

## combiner_exec()
combiner_exec() 函数将一个 grpc_closure 添加到 Combiner 的 queue 中，如果是首次添加，还需要将 Combiner 添加到 ExecCtx 中。
```
static void combiner_exec(grpc_core::Combiner* lock, grpc_closure* cl,
                          grpc_error_handle error) {
  GPR_TIMER_SCOPE("combiner.execute", 0);
  GRPC_STATS_INC_COMBINER_LOCKS_SCHEDULED_ITEMS();
  gpr_atm last = gpr_atm_full_fetch_add(&lock->state, STATE_ELEM_COUNT_LOW_BIT);
  GRPC_COMBINER_TRACE(gpr_log(GPR_INFO,
                              "C:%p grpc_combiner_execute c=%p last=%" PRIdPTR,
                              lock, cl, last));
  if (last == 1) {
    GRPC_STATS_INC_COMBINER_LOCKS_INITIATED();
    GPR_TIMER_MARK("combiner.initiated", 0);
    gpr_atm_no_barrier_store(&lock->initiating_exec_ctx_or_null,
                             (gpr_atm)grpc_core::ExecCtx::Get());
    // first element on this list: add it to the list of combiner locks
    // executing within this exec_ctx
    push_last_on_exec_ctx(lock);
  } else {
    // there may be a race with setting here: if that happens, we may delay
    // offload for one or two actions, and that's fine
    gpr_atm initiator =
        gpr_atm_no_barrier_load(&lock->initiating_exec_ctx_or_null);
    if (initiator != 0 &&
        initiator != reinterpret_cast<gpr_atm>(grpc_core::ExecCtx::Get())) {
      gpr_atm_no_barrier_store(&lock->initiating_exec_ctx_or_null, 0);
    }
  }
  GPR_ASSERT(last & STATE_UNORPHANED);  // ensure lock has not been destroyed
  assert(cl->cb);
  cl->error_data.error = error;
  lock->queue.Push(cl->next_data.mpscq_node.get());
}
```
## combiner_finally_exec()
combiner_finally_exec() 负责将 grpc_closure 添加到 Combiner::final_list 中。
```
static void combiner_finally_exec(grpc_core::Combiner* lock,
                                  grpc_closure* closure,
                                  grpc_error_handle error) {
  GPR_ASSERT(lock != nullptr);
  GPR_TIMER_SCOPE("combiner.execute_finally", 0);
  GRPC_STATS_INC_COMBINER_LOCKS_SCHEDULED_FINAL_ITEMS();
  GRPC_COMBINER_TRACE(gpr_log(
      GPR_INFO, "C:%p grpc_combiner_execute_finally c=%p; ac=%p", lock, closure,
      grpc_core::ExecCtx::Get()->combiner_data()->active_combiner));
  if (grpc_core::ExecCtx::Get()->combiner_data()->active_combiner != lock) {
    GPR_TIMER_MARK("slowpath", 0);
    // Using error_data.scratch to store the combiner so that it can be accessed
    // in enqueue_finally.
    closure->error_data.scratch = reinterpret_cast<uintptr_t>(lock);
    lock->Run(GRPC_CLOSURE_CREATE(enqueue_finally, closure, nullptr), error);
    return;
  }

  if (grpc_closure_list_empty(lock->final_list)) {
    gpr_atm_full_fetch_add(&lock->state, STATE_ELEM_COUNT_LOW_BIT);
  }
  grpc_closure_list_append(&lock->final_list, closure, error);
}

static void enqueue_finally(void* closure, grpc_error_handle error) {
  grpc_closure* cl = static_cast<grpc_closure*>(closure);
  combiner_finally_exec(
      reinterpret_cast<grpc_core::Combiner*>(cl->error_data.scratch), cl,
      GRPC_ERROR_REF(error));
}
```
## grpc_combiner_continue_exec_ctx()
grpc_combiner_continue_exec_ctx() 在 ExecCtx::Flush() 函数中被调用，该函数负责执行 grpc_closure。
```
bool grpc_combiner_continue_exec_ctx() {
  GPR_TIMER_SCOPE("combiner.continue_exec_ctx", 0);
  grpc_core::Combiner* lock =
      grpc_core::ExecCtx::Get()->combiner_data()->active_combiner;
  if (lock == nullptr) {
    return false;
  }

  bool contended =
      gpr_atm_no_barrier_load(&lock->initiating_exec_ctx_or_null) == 0;

// TRACE...

  // offload only if all the following conditions are true:
  // 1. the combiner is contended and has more than one closure to execute
  // 2. the current execution context needs to finish as soon as possible
  // 3. the current thread is not a worker for any background poller
  // 4. the DEFAULT executor is threaded
  if (contended && grpc_core::ExecCtx::Get()->IsReadyToFinish() &&
      !grpc_iomgr_is_any_background_poller_thread() &&
      grpc_core::Executor::IsThreadedDefault()) {
    GPR_TIMER_MARK("offload_from_finished_exec_ctx", 0);
    // this execution context wants to move on: schedule remaining work to be
    // picked up on the executor
    queue_offload(lock);
    return true;
  }

  if (!lock->time_to_execute_final_list ||
      // peek to see if something new has shown up, and execute that with
      // priority
      (gpr_atm_acq_load(&lock->state) >> 1) > 1) {
    grpc_core::MultiProducerSingleConsumerQueue::Node* n = lock->queue.Pop();
    GRPC_COMBINER_TRACE(
        gpr_log(GPR_INFO, "C:%p maybe_finish_one n=%p", lock, n));
    if (n == nullptr) {
      // queue is in an inconsistent state: use this as a cue that we should
      // go off and do something else for a while (and come back later)
      GPR_TIMER_MARK("delay_busy", 0);
      queue_offload(lock);
      return true;
    }
    GPR_TIMER_SCOPE("combiner.exec1", 0);
    grpc_closure* cl = reinterpret_cast<grpc_closure*>(n);
    grpc_error_handle cl_err = cl->error_data.error;
#ifndef NDEBUG
    cl->scheduled = false;
#endif
    cl->cb(cl->cb_arg, cl_err);
    GRPC_ERROR_UNREF(cl_err);
  } else {
    grpc_closure* c = lock->final_list.head;
    GPR_ASSERT(c != nullptr);
    grpc_closure_list_init(&lock->final_list);
    int loops = 0;
    while (c != nullptr) {
      GPR_TIMER_SCOPE("combiner.exec_1final", 0);
      GRPC_COMBINER_TRACE(
          gpr_log(GPR_INFO, "C:%p execute_final[%d] c=%p", lock, loops, c));
      grpc_closure* next = c->next_data.next;
      grpc_error_handle error = c->error_data.error;
#ifndef NDEBUG
      c->scheduled = false;
#endif
      c->cb(c->cb_arg, error);
      GRPC_ERROR_UNREF(error);
      c = next;
    }
  }

  GPR_TIMER_MARK("unref", 0);
  move_next();
  lock->time_to_execute_final_list = false;
  gpr_atm old_state =
      gpr_atm_full_fetch_add(&lock->state, -STATE_ELEM_COUNT_LOW_BIT);
  GRPC_COMBINER_TRACE(
      gpr_log(GPR_INFO, "C:%p finish old_state=%" PRIdPTR, lock, old_state));
// Define a macro to ease readability of the following switch statement.
#define OLD_STATE_WAS(orphaned, elem_count) \
  (((orphaned) ? 0 : STATE_UNORPHANED) |    \
   ((elem_count)*STATE_ELEM_COUNT_LOW_BIT))
  // Depending on what the previous state was, we need to perform different
  // actions.
  switch (old_state) {
    default:
      // we have multiple queued work items: just continue executing them
      break;
    case OLD_STATE_WAS(false, 2):
    case OLD_STATE_WAS(true, 2):
      // we're down to one queued item: if it's the final list we should do that
      if (!grpc_closure_list_empty(lock->final_list)) {
        lock->time_to_execute_final_list = true;
      }
      break;
    case OLD_STATE_WAS(false, 1):
      // had one count, one unorphaned --> unlocked unorphaned
      return true;
    case OLD_STATE_WAS(true, 1):
      // and one count, one orphaned --> unlocked and orphaned
      really_destroy(lock);
      return true;
    case OLD_STATE_WAS(false, 0):
    case OLD_STATE_WAS(true, 0):
      // these values are illegal - representing an already unlocked or
      // deleted lock
      GPR_UNREACHABLE_CODE(return true);
  }
  push_first_on_exec_ctx(lock);
  return true;
}
```