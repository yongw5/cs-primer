# closure
## grpc_closure
grpc_closure 封装了回调函数以及其参数
```
struct grpc_closure {
  union {
    grpc_closure* next;
    grpc_core::ManualConstructor<
        grpc_core::MultiProducerSingleConsumerQueue::Node>
        mpscq_node;
    uintptr_t scratch;
  } next_data;

  /** Bound callback. */
  grpc_iomgr_cb_func cb;

  /** Arguments to be passed to "cb". */
  void* cb_arg;

  union {
    grpc_error_handle error;
    uintptr_t scratch;
  } error_data;

#ifndef NDEBUG
// ...
#endif
};
```
grpc_closure 初始化函数 **grpc_closure_init()** 也相当简单，只是给 grpc_closure 的 cb、cb_arg 和 error_data 赋值。其定义如下
```
#ifndef NDEBUG
// ...
#else
inline grpc_closure* grpc_closure_init(grpc_closure* closure,
                                       grpc_iomgr_cb_func cb, void* cb_arg) {
#endif
  closure->cb = cb;
  closure->cb_arg = cb_arg;
  closure->error_data.error = GRPC_ERROR_NONE;
#ifndef NDEBUG
// ...
#endif
  return closure;
}
```
iomgr 定义了宏 **GRPC_CLOSURE_INIT** 封装 grpc_closure_init() 函数，供外部使用。
```
#ifndef NDEBUG
// ...
#else
#define GRPC_CLOSURE_INIT(closure, cb, cb_arg, scheduler) \
  grpc_closure_init(closure, cb, cb_arg)
#endif
```
## wrapped_closure
iomgr 内部并不直接使用 grpc_closure，而是使用 wrapped_closure。其定义如下：
```
struct wrapped_closure {
  grpc_iomgr_cb_func cb;
  void* cb_arg;
  grpc_closure wrapper;
};
```
从 **grpc_closure_create()** 函数可以看出 grpc_closure 和 wrapped_closure 两者之间的联系。其定义如下：
```
#ifndef NDEBUG
// ...
#else
inline grpc_closure* grpc_closure_create(grpc_iomgr_cb_func cb, void* cb_arg) {
#endif
  closure_impl::wrapped_closure* wc =
      static_cast<closure_impl::wrapped_closure*>(gpr_malloc(sizeof(*wc)));
  wc->cb = cb;
  wc->cb_arg = cb_arg;
#ifndef NDEBUG
// ...
#else
  grpc_closure_init(&wc->wrapper, closure_impl::closure_wrapper, wc);
#endif
  return &wc->wrapper;
}
```
grpc_closure_create() 函数首先在**堆**上申请一个 wrapped_closure 对象，然后将传入的参数 cb 和 cb_arg 赋值给 wrapped_closure 的 cb 和 cb_arg 成员，最后调用 grpc_closure_init() 函数设置其 grpc_closure 类型的成员的 cb 为 **closure_wrapper()** 函数。closure_wrapper() 函数定义如下：
```
inline void closure_wrapper(void* arg, grpc_error_handle error) {
  wrapped_closure* wc = static_cast<wrapped_closure*>(arg);
  grpc_iomgr_cb_func cb = wc->cb;
  void* cb_arg = wc->cb_arg;
  gpr_free(wc);
  cb(cb_arg, error);
}
```
可以看到，closure_wrapper() 函数将 wrapped_closure 成员 cb 和 cb_arg 保存，然后释放 wrapped_closure 对象，最后调用 cb 执行的函数。

因此，grpc_closure_create() 函数在堆上申请一个 wrapped_closure 对象，其在 iomgr 内部使用。wrapped_closure 有一个 grpc_closure 成员，并且 grpc_closure 成员的 cb 被初始化为指向 closure_wrapper() 函数。wrapped_closure 中 cb 和 cb_arg 保存了外部传入的回调函数及其参数。closure_wrapper() 函数被调用时，堆上申请的 wrapped_closure 对象被释放掉，最后 wrapped_closure 中 cb 指向的函数被调用。

iomgr 定义了宏 **GRPC_CLOSURE_CREATE** 封装 grpc_closure_create() 函数，供外部使用。
```
#ifndef NDEBUG
// ...
#else
#define GRPC_CLOSURE_CREATE(cb, cb_arg, scheduler) \
  grpc_closure_create(cb, cb_arg)
#endif
```
## Closure
grpc_closure 有多种执行方式，其中最简单的是 grpc_core::Closure::Run() 方法。
```
namespace grpc_core {
class Closure {
 public:
  static void Run(const DebugLocation& location, grpc_closure* closure,
                  grpc_error_handle error) {
    (void)location;
    if (closure == nullptr) {
      GRPC_ERROR_UNREF(error);
      return;
    }
#ifndef NDEBUG
// ...
#endif
    closure->cb(closure->cb_arg, error);
#ifndef NDEBUG
// ...
#endif
    GRPC_ERROR_UNREF(error);
  }
};
}  // namespace grpc_core
```