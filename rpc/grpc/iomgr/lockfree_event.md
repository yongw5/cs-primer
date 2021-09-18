# lockfree_event
## LockfreeEvent
LockfreeEvent 是一个无锁事件类，主要用于读、写和错误事件的触发，其定义如下：
```
class LockfreeEvent {
 public:
  LockfreeEvent();

  LockfreeEvent(const LockfreeEvent&) = delete;
  LockfreeEvent& operator=(const LockfreeEvent&) = delete;

  // These methods are used to initialize and destroy the internal state. These
  // cannot be done in constructor and destructor because SetReady may be called
  // when the event is destroyed and put in a freelist.
  void InitEvent();
  void DestroyEvent();

  // Returns true if fd has been shutdown, false otherwise.
  bool IsShutdown() const {
    return (gpr_atm_no_barrier_load(&state_) & kShutdownBit) != 0;
  }

  // Schedules \a closure when the event is received (see SetReady()) or the
  // shutdown state has been set. Note that the event may have already been
  // received, in which case the closure would be scheduled immediately.
  // If the shutdown state has already been set, then \a closure is scheduled
  // with the shutdown error.
  void NotifyOn(grpc_closure* closure);

  // Sets the shutdown state. If a closure had been provided by NotifyOn and has
  // not yet been scheduled, it will be scheduled with \a shutdown_error.
  bool SetShutdown(grpc_error_handle shutdown_error);

  // Signals that the event has been received.
  void SetReady();

 private:
  enum State { kClosureNotReady = 0, kClosureReady = 2, kShutdownBit = 1 };

  gpr_atm state_;
};
```
LockfreeEvent 只有一个成员变量 state_，其值可以是如下几种：
- kClosureNotReady：fd 没有目标 IO 事件；
- kClosureReady：fd 目标 IO 事件已经发生，但没有 closure 执行；
- closure ptr：目标 IO 事件发生时，执行的 closure；
- shutdown_error | kShutDownBit：fd 已经 shutdown，保存错误信息；

这些状态转移如下：
```
 <closure ptr> <----3----- kClosureNotReady -----1------> kClosureReady
   |  |                       ^   |    ^                       |  |
   |  |                       |   |    |                       |  |
   |  +-------------4---------+   6    +---------2-------------+  |
   |                              |                               |
   |                              v                               |
   +-----5------>  [shutdown_error | kShutdownBit] <------7-------+

For 1, 4 : See SetReady() function
For 2, 3 : See NotifyOn() function
For 5,6,7: See SetShutdown() function
```
### InitEvent()
```
void LockfreeEvent::InitEvent() {
  /* Perform an atomic store to start the state machine.
  gpr_atm_no_barrier_store(&state_, kClosureNotReady);
}

LockfreeEvent::LockfreeEvent() { InitEvent(); }
```
### DestroyEvent()
```
void LockfreeEvent::DestroyEvent() {
  gpr_atm curr;
  do {
    curr = gpr_atm_no_barrier_load(&state_);
    if (curr & kShutdownBit) {
      GRPC_ERROR_UNREF((grpc_error_handle)(curr & ~kShutdownBit));
    } else {
      GPR_ASSERT(curr == kClosureNotReady || curr == kClosureReady);
    }
  } while (!gpr_atm_no_barrier_cas(&state_, curr,
                                   kShutdownBit /* shutdown, no error */));
}
```
### NotifyOn()
NotifyOn() 函数用于设置 fd 目标 IO 事件发生时调用的 closure。分如下 4 中情况处理：
1. fd 还未设置 IO 事件（state_ 值为 kClosureNotReady），将传入的 closure 保存到 state_；
1. fd 目标 IO 事件已经发生（state_ 值为 kClosureReady），将 state_ 值修改为 kClosureNotReady，然后立即调度 closure；
1. fd 已经 shutdown，用 shutdown_error 调度 closure；
1. 已经设置了 closure，终止程序；

```
void LockfreeEvent::NotifyOn(grpc_closure* closure) {
  while (true) {
    gpr_atm curr = gpr_atm_acq_load(&state_);
/// traces...
    switch (curr) {
      case kClosureNotReady: {
        /* kClosureNotReady -> <closure>.

           We're guaranteed by API that there's an acquire barrier before here,
           so there's no need to double-dip and this can be a release-only.

           The release itself pairs with the acquire half of a set_ready full
           barrier. */
        if (gpr_atm_rel_cas(&state_, kClosureNotReady,
                            reinterpret_cast<gpr_atm>(closure))) {
          return; /* Successful. Return */
        }

        break; /* retry */
      }

      case kClosureReady: {
        /* Change the state to kClosureNotReady. Schedule the closure if
           successful. If not, the state most likely transitioned to shutdown.
           We should retry.

           This can be a no-barrier cas since the state is being transitioned to
           kClosureNotReady; set_ready and set_shutdown do not schedule any
           closure when transitioning out of CLOSURE_NO_READY state (i.e there
           is no other code that needs to 'happen-after' this) */
        if (gpr_atm_no_barrier_cas(&state_, kClosureReady, kClosureNotReady)) {
          ExecCtx::Run(DEBUG_LOCATION, closure, GRPC_ERROR_NONE);
          return; /* Successful. Return */
        }

        break; /* retry */
      }

      default: {
        /* 'curr' is either a closure or the fd is shutdown(in which case 'curr'
           contains a pointer to the shutdown-error). If the fd is shutdown,
           schedule the closure with the shutdown error */
        if ((curr & kShutdownBit) > 0) {
          grpc_error_handle shutdown_err =
              reinterpret_cast<grpc_error_handle>(curr & ~kShutdownBit);
          ExecCtx::Run(DEBUG_LOCATION, closure,
                       GRPC_ERROR_CREATE_REFERENCING_FROM_STATIC_STRING(
                           "FD Shutdown", &shutdown_err, 1));
          return;
        }

        /* There is already a closure!. This indicates a bug in the code */
        gpr_log(GPR_ERROR,
                "LockfreeEvent::NotifyOn: notify_on called with a previous "
                "callback still pending");
        abort();
      }
    }
  }

  GPR_UNREACHABLE_CODE(return );
}
```
### SetReady()
SetReady() 标记 fd 目标 IO 事件已经发生。如果 LockfreeEvent 已经设置了 closure，则调度执行。
```
void LockfreeEvent::SetReady() {
  while (true) {
    gpr_atm curr = gpr_atm_no_barrier_load(&state_);
/// traces...
    switch (curr) {
      case kClosureReady: {
        /* Already ready. We are done here */
        return;
      }

      case kClosureNotReady: {
        /* No barrier required as we're transitioning to a state that does not
           involve a closure */
        if (gpr_atm_no_barrier_cas(&state_, kClosureNotReady, kClosureReady)) {
          return; /* early out */
        }
        break; /* retry */
      }

      default: {
        /* 'curr' is either a closure or the fd is shutdown */
        if ((curr & kShutdownBit) > 0) {
          /* The fd is shutdown. Do nothing */
          return;
        }
        /* Full cas: acquire pairs with this cas' release in the event of a
           spurious set_ready; release pairs with this or the acquire in
           notify_on (or set_shutdown) */
        else if (gpr_atm_full_cas(&state_, curr, kClosureNotReady)) {
          ExecCtx::Run(DEBUG_LOCATION, reinterpret_cast<grpc_closure*>(curr),
                       GRPC_ERROR_NONE);
          return;
        }
        /* else the state changed again (only possible by either a racing
           set_ready or set_shutdown functions. In both these cases, the closure
           would have been scheduled for execution. So we are done here */
        return;
      }
    }
  }
}
```
### SetShutDown()
设置 fd 为 shutdown 状态，如果设置了 closure，调度执行。
```
bool LockfreeEvent::SetShutdown(grpc_error_handle shutdown_error) {
  gpr_atm new_state = reinterpret_cast<gpr_atm>(shutdown_error) | kShutdownBit;

  while (true) {
    gpr_atm curr = gpr_atm_no_barrier_load(&state_);
    if (GRPC_TRACE_FLAG_ENABLED(grpc_polling_trace)) {
      gpr_log(GPR_DEBUG,
              "LockfreeEvent::SetShutdown: %p curr=%" PRIxPTR " err=%s",
              &state_, curr, grpc_error_std_string(shutdown_error).c_str());
    }
    switch (curr) {
      case kClosureReady:
      case kClosureNotReady:
        /* Need a full barrier here so that the initial load in notify_on
           doesn't need a barrier */
        if (gpr_atm_full_cas(&state_, curr, new_state)) {
          return true; /* early out */
        }
        break; /* retry */

      default: {
        /* 'curr' is either a closure or the fd is already shutdown */

        /* If fd is already shutdown, we are done */
        if ((curr & kShutdownBit) > 0) {
          GRPC_ERROR_UNREF(shutdown_error);
          return false;
        }

        /* Fd is not shutdown. Schedule the closure and move the state to
           shutdown state.
           Needs an acquire to pair with setting the closure (and get a
           happens-after on that edge), and a release to pair with anything
           loading the shutdown state. */
        if (gpr_atm_full_cas(&state_, curr, new_state)) {
          ExecCtx::Run(DEBUG_LOCATION, reinterpret_cast<grpc_closure*>(curr),
                       GRPC_ERROR_CREATE_REFERENCING_FROM_STATIC_STRING(
                           "FD Shutdown", &shutdown_error, 1));
          return true;
        }

        /* 'curr' was a closure but now changed to a different state. We will
          have to retry */
        break;
      }
    }
  }

  GPR_UNREACHABLE_CODE(return false);
}
```