# call_combiner
## CallCombiner
CallCombiner 是轻量化的 Combiner，两者功能相似。CallCombiner 需要某个 callback 明确（通过 GRPC_CALL_COMBINER_STOP）指示何时启动。其定义如下：
```
class CallCombiner {
 public:
  CallCombiner();
  ~CallCombiner();

#ifndef NDEBUG
/// ...
#else
#define GRPC_CALL_COMBINER_START(call_combiner, closure, error, reason) \
  (call_combiner)->Start((closure), (error), (reason))
#define GRPC_CALL_COMBINER_STOP(call_combiner, reason) \
  (call_combiner)->Stop((reason))
  /// Starts processing \a closure.
  void Start(grpc_closure* closure, grpc_error_handle error,
             const char* reason);
  /// Yields the call combiner to the next closure in the queue, if any.
  void Stop(const char* reason);
#endif

  void SetNotifyOnCancel(grpc_closure* closure);

  /// Indicates that the call has been cancelled.
  void Cancel(grpc_error_handle error);

 private:
  void ScheduleClosure(grpc_closure* closure, grpc_error_handle error);
#ifdef GRPC_TSAN_ENABLED
  static void TsanClosure(void* arg, grpc_error_handle error);
#endif

  gpr_atm size_ = 0;  // size_t, num closures in queue or currently executing
  MultiProducerSingleConsumerQueue queue_;
  // Either 0 (if not cancelled and no cancellation closure set),
  // a grpc_closure* (if the lowest bit is 0),
  // or a grpc_error_handle (if the lowest bit is 1).
  gpr_atm cancel_state_ = 0;
#ifdef GRPC_TSAN_ENABLED
/// ...
#endif
};
```
cancel_state_ 成员变量记录的是 CallCombiner 的状态，有三种情况：
- 值为 0 表示初始化状态（构造函数将其初始化为 0），没有存储任何信息，表示没有处于 cancel；
- 最低位为 0，表示 grpc_closure 指针，指向 cancel_closure；
- 最低位为 1，表示 grpc_error_handle；

有两个相关的函数用于编码或者解码 error 信息
```
namespace {
grpc_error_handle DecodeCancelStateError(gpr_atm cancel_state) {
  if (cancel_state & 1) {
    return reinterpret_cast<grpc_error_handle>(cancel_state &
                                               ~static_cast<gpr_atm>(1));
  }
  return GRPC_ERROR_NONE;
}

gpr_atm EncodeCancelStateError(grpc_error_handle error) {
  return static_cast<gpr_atm>(1) | reinterpret_cast<gpr_atm>(error);
}
}  // namespace
```
### Start()
如果 CallCombiner 没有 closure，Start() 将调用 ScheduleClosure() 函数，否则将 closure 添加到多生产单消费的队列 queue_ 里面。
```
void CallCombiner::Start(grpc_closure* closure, grpc_error_handle error,
                         DEBUG_ARGS const char* reason) {
  GPR_TIMER_SCOPE("CallCombiner::Start", 0);
/// trace...
  size_t prev_size =
      static_cast<size_t>(gpr_atm_full_fetch_add(&size_, (gpr_atm)1));
/// trace...
  GRPC_STATS_INC_CALL_COMBINER_LOCKS_SCHEDULED_ITEMS();
  if (prev_size == 0) {
    GRPC_STATS_INC_CALL_COMBINER_LOCKS_INITIATED();
    GPR_TIMER_MARK("call_combiner_initiate", 0);
/// trace...
    // Queue was empty, so execute this closure immediately.
    ScheduleClosure(closure, error);
  } else {
    if (GRPC_TRACE_FLAG_ENABLED(grpc_call_combiner_trace)) {
      gpr_log(GPR_INFO, "  QUEUING");
    }
    // Queue was not empty, so add closure to queue.
    closure->error_data.error = error;
    queue_.Push(
        reinterpret_cast<MultiProducerSingleConsumerQueue::Node*>(closure));
  }
}
```
**ScheduleClosure()** 函数直接调用 ExecCtx::Run() 函数
```
oid CallCombiner::ScheduleClosure(grpc_closure* closure,
                                   grpc_error_handle error) {
#ifdef GRPC_TSAN_ENABLED
/// ...
#else
  ExecCtx::Run(DEBUG_LOCATION, closure, error);
#endif
}
```
### Stop()
Stop() 函数，将 queue_ 中保存的 closure 逐一出队，调用 ScheduleClosure() 函数。
```
void CallCombiner::Stop(DEBUG_ARGS const char* reason) {
  GPR_TIMER_SCOPE("CallCombiner::Stop", 0);
/// trace...
  size_t prev_size =
      static_cast<size_t>(gpr_atm_full_fetch_add(&size_, (gpr_atm)-1));
/// trace...
  GPR_ASSERT(prev_size >= 1);
  if (prev_size > 1) {
    while (true) {
/// trace...
      bool empty;
      grpc_closure* closure =
          reinterpret_cast<grpc_closure*>(queue_.PopAndCheckEnd(&empty));
      if (closure == nullptr) {
        // This can happen either due to a race condition within the mpscq
        // code or because of a race with Start().
/// trace...
        continue;
      }
/// trace...
      ScheduleClosure(closure, closure->error_data.error);
      break;
    }
  } else if (GRPC_TRACE_FLAG_ENABLED(grpc_call_combiner_trace)) {
    gpr_log(GPR_INFO, "  queue empty");
  }
}
```
### SetNotifyOnCancel()
SetNotifyOnCancel() 函数用于设置 Cancel() 调用的 grpc_closure，其注册的 grpc_closure 肯定会被调用一次并且只会调用一次。这使得注册的 grpc_closure 可以含有需要内存释放的引用。如果 cancel 确实发生，grpc_closure 被调度时传入相应的错误信息；相反，如果 cancel 没有发生，grpc_closure 依然被调度，只是传入 GRPC_ERROR_NONE。

注册的 grpc_closure 在如下情况被调用：
- 如果在调用 SetNotifyOnCancel() 函数之前，Cancel() 函数已经被调用，grpc_closure 将立即被调度；
- 调用 Cancel() 时，grpc_closure 被调度；
- 如果 SetNotifyOnCancel() 再次被调用，之前注册的 grpc_closure 被调度，传入 GRPC_ERROR_NONE。

```
void CallCombiner::SetNotifyOnCancel(grpc_closure* closure) {
  GRPC_STATS_INC_CALL_COMBINER_SET_NOTIFY_ON_CANCEL();
  while (true) {
    // Decode original state.
    gpr_atm original_state = gpr_atm_acq_load(&cancel_state_);
    grpc_error_handle original_error = DecodeCancelStateError(original_state);
    // If error is set, invoke the cancellation closure immediately.
    // Otherwise, store the new closure.
    if (original_error != GRPC_ERROR_NONE) {
/// trace...
      ExecCtx::Run(DEBUG_LOCATION, closure, GRPC_ERROR_REF(original_error));
      break;
    } else {
      if (gpr_atm_full_cas(&cancel_state_, original_state,
                           reinterpret_cast<gpr_atm>(closure))) {
/// trace...
        // If we replaced an earlier closure, invoke the original
        // closure with GRPC_ERROR_NONE.  This allows callers to clean
        // up any resources they may be holding for the callback.
        if (original_state != 0) {
          closure = reinterpret_cast<grpc_closure*>(original_state);
/// trace...
          ExecCtx::Run(DEBUG_LOCATION, closure, GRPC_ERROR_NONE);
        }
        break;
      }
    }
    // cas failed, try again.
  }
}
```
### Cancel()
Cancel() 标记这个 CallCombiner 被取消。当 cancel_state_ 保存了 error，Cancel() 立即返回，否则调用其保存的 grpc_closure。
```
void CallCombiner::Cancel(grpc_error_handle error) {
  GRPC_STATS_INC_CALL_COMBINER_CANCELLED();
  while (true) {
    gpr_atm original_state = gpr_atm_acq_load(&cancel_state_);
    grpc_error_handle original_error = DecodeCancelStateError(original_state);
    if (original_error != GRPC_ERROR_NONE) {
      GRPC_ERROR_UNREF(error);
      break;
    }
    if (gpr_atm_full_cas(&cancel_state_, original_state,
                         EncodeCancelStateError(error))) {
      if (original_state != 0) {
        grpc_closure* notify_on_cancel =
            reinterpret_cast<grpc_closure*>(original_state);
/// trace...
        ExecCtx::Run(DEBUG_LOCATION, notify_on_cancel, GRPC_ERROR_REF(error));
      }
      break;
    }
    // cas failed, try again.
  }
}
```
## CallCombinerClosureList
```
// Helper for running a list of closures in a call combiner.
//
// Each callback running in the call combiner will eventually be
// returned to the surface, at which point the surface will yield the
// call combiner.  So when we are running in the call combiner and have
// more than one callback to return to the surface, we need to re-enter
// the call combiner for all but one of those callbacks.
class CallCombinerClosureList {
 public:
  CallCombinerClosureList() {}

  // Adds a closure to the list.  The closure must eventually result in
  // the call combiner being yielded.
  void Add(grpc_closure* closure, grpc_error_handle error, const char* reason) {
    closures_.emplace_back(closure, error, reason);
  }

  // Runs all closures in the call combiner and yields the call combiner.
  //
  // All but one of the closures in the list will be scheduled via
  // GRPC_CALL_COMBINER_START(), and the remaining closure will be
  // scheduled via ExecCtx::Run(), which will eventually result
  // in yielding the call combiner.  If the list is empty, then the call
  // combiner will be yielded immediately.
  void RunClosures(CallCombiner* call_combiner) {
    if (closures_.empty()) {
      GRPC_CALL_COMBINER_STOP(call_combiner, "no closures to schedule");
      return;
    }
    for (size_t i = 1; i < closures_.size(); ++i) {
      auto& closure = closures_[i];
      GRPC_CALL_COMBINER_START(call_combiner, closure.closure, closure.error,
                               closure.reason);
    }
/// trace...
    // This will release the call combiner.
    ExecCtx::Run(DEBUG_LOCATION, closures_[0].closure, closures_[0].error);
    closures_.clear();
  }

  // Runs all closures in the call combiner, but does NOT yield the call
  // combiner.  All closures will be scheduled via GRPC_CALL_COMBINER_START().
  void RunClosuresWithoutYielding(CallCombiner* call_combiner) {
    for (size_t i = 0; i < closures_.size(); ++i) {
      auto& closure = closures_[i];
      GRPC_CALL_COMBINER_START(call_combiner, closure.closure, closure.error,
                               closure.reason);
    }
    closures_.clear();
  }

  size_t size() const { return closures_.size(); }

 private:
  struct CallCombinerClosure {
    grpc_closure* closure;
    grpc_error_handle error;
    const char* reason;

    CallCombinerClosure(grpc_closure* closure, grpc_error_handle error,
                        const char* reason)
        : closure(closure), error(error), reason(reason) {}
  };

  // There are generally a maximum of 6 closures to run in the call
  // combiner, one for each pending op.
  absl::InlinedVector<CallCombinerClosure, 6> closures_;
};
```