# grpc_fd
## grpc_fd
```
struct grpc_fd {
  grpc_fd(int fd, const char* name, bool track_err)
      : fd(fd), track_err(track_err) {
    gpr_mu_init(&orphan_mu);
    gpr_mu_init(&pollable_mu);
    read_closure.InitEvent();
    write_closure.InitEvent();
    error_closure.InitEvent();

    std::string fd_name = absl::StrCat(name, " fd=", fd);
    grpc_iomgr_register_object(&iomgr_object, fd_name.c_str());
#ifndef NDEBUG
/// ...
#endif
  }

  // This is really the dtor, but the poller threads waking up from
  // epoll_wait() may access the (read|write|error)_closure after destruction.
  // Since the object will be added to the free pool, this behavior is
  // not going to cause issues, except spurious events if the FD is reused
  // while the race happens.
  void destroy() {
    grpc_iomgr_unregister_object(&iomgr_object);

    POLLABLE_UNREF(pollable_obj, "fd_pollable");

    // To clear out the allocations of pollset_fds, we need to swap its
    // contents with a newly-constructed (and soon to be destructed) local
    // variable of its same type. This is because InlinedVector::clear is _not_
    // guaranteed to actually free up allocations and this is important since
    // this object doesn't have a conventional destructor.
    absl::InlinedVector<int, 1> pollset_fds_tmp;
    pollset_fds_tmp.swap(pollset_fds);

    gpr_mu_destroy(&pollable_mu);
    gpr_mu_destroy(&orphan_mu);

    read_closure.DestroyEvent();
    write_closure.DestroyEvent();
    error_closure.DestroyEvent();

    invalidate();
  }

#ifndef NDEBUG
/// ...
#else
  void invalidate() {}
#endif

  int fd;

  // refst format:
  //     bit 0    : 1=Active / 0=Orphaned
  //     bits 1-n : refcount
  //  Ref/Unref by two to avoid altering the orphaned bit
  gpr_atm refst = 1;

  gpr_mu orphan_mu;

  // Protects pollable_obj and pollset_fds.
  gpr_mu pollable_mu;
  absl::InlinedVector<int, 1> pollset_fds;  // Used in PO_MULTI.
  pollable* pollable_obj = nullptr;         // Used in PO_FD.

  grpc_core::LockfreeEvent read_closure;
  grpc_core::LockfreeEvent write_closure;
  grpc_core::LockfreeEvent error_closure;

  struct grpc_fd* freelist_next = nullptr;
  grpc_closure* on_done_closure = nullptr;

  grpc_iomgr_object iomgr_object;

  // Do we need to track EPOLLERR events separately?
  bool track_err;
};
```
grpc_fd 封装了一个文件描述符，包含了在 fd 可读、可写已经出错的 closure。
### fd_global_init()
grpc_fd 对象的生命周期需要注意。grpc_fd 从堆空间上分配，但是 grpc_fd 不再使用时，并不会将其内存释放掉，而是将其放置 fd_freelist 链表。这么做不是为了 malloc 性能考量，而是为了简化多线程环境程序编写。在多线程环境下，除非使用复杂同步方法（性能开销大），否则很难确定一个 grpc_fd 是否可以安全地 free 掉。
```
static grpc_fd* fd_freelist = nullptr;
static gpr_mu fd_freelist_mu;

static void fd_global_init(void) { gpr_mu_init(&fd_freelist_mu); }
```
### fd_global_shutdown()
释放 fd_freelist 持有的 grpc_fd。
```
static void fd_global_shutdown(void) {
  gpr_mu_lock(&fd_freelist_mu);
  gpr_mu_unlock(&fd_freelist_mu);
  while (fd_freelist != nullptr) {
    grpc_fd* fd = fd_freelist;
    fd_freelist = fd_freelist->freelist_next;
    gpr_free(fd);
  }
  gpr_mu_destroy(&fd_freelist_mu);
}
```
### fd_create()
fd_create() 创建并返回一个 grpc_fd。fd_create() 首先在 fd_freelist 上查找有无空闲 grpc_fd，如果没有，再在堆空间上申请一个 grpc_fd 对象。
```
static grpc_fd* fd_create(int fd, const char* name, bool track_err) {
  grpc_fd* new_fd = nullptr;

  gpr_mu_lock(&fd_freelist_mu);
  if (fd_freelist != nullptr) {
    new_fd = fd_freelist;
    fd_freelist = fd_freelist->freelist_next;
  }
  gpr_mu_unlock(&fd_freelist_mu);

  if (new_fd == nullptr) {
    new_fd = static_cast<grpc_fd*>(gpr_malloc(sizeof(grpc_fd)));
  }

  return new (new_fd) grpc_fd(fd, name, track_err);
}
```
### fd_destroy()
grpc_fd 使用引用计数管理，fd_destroy() 并不直接被调用。通过宏 REF_BY 和 UNREF_BY 增加或者减少引用计数，当引用计数减少为 0，才将 fd_destroy() 添加到 ExecCtx。不过，grpc_fd 的引用计数 refst 具有特殊的格式，最低位 0 表示 Orphaned，1 表示 Active，其他位当作引用计数。refst 初始化为 1，REF_BY 和 UNREF_BY 通常都增加或者减少 2，避免破坏最低位数值。

// refst format:
  //     bit 0    : 1=Active / 0=Orphaned
  //     bits 1-n : refcount
  //  Ref/Unref by two to avoid altering the orphaned bit
```
#define REF_BY(fd, n, reason) \
  do {                        \
    ref_by(fd, n);            \
    (void)(reason);           \
  } while (0)
#define UNREF_BY(fd, n, reason) \
  do {                          \
    unref_by(fd, n);            \
    (void)(reason);             \
  } while (0)

static void ref_by(grpc_fd* fd, int n) {
  GPR_ASSERT(gpr_atm_no_barrier_fetch_add(&fd->refst, n) > 0);
}

static void unref_by(grpc_fd* fd, int n) {
  gpr_atm old = gpr_atm_full_fetch_add(&fd->refst, -n);
  if (old == n) {
    grpc_core::ExecCtx::Run(
        DEBUG_LOCATION,
        GRPC_CLOSURE_CREATE(fd_destroy, fd, grpc_schedule_on_exec_ctx),
        GRPC_ERROR_NONE);
  } else {
    GPR_ASSERT(old > n);
  }
}
```
fd_destroy() 定义如下：
```
static void fd_destroy(void* arg, grpc_error_handle /*error*/) {
  grpc_fd* fd = static_cast<grpc_fd*>(arg);
  fd->destroy();

  /* Add the fd to the freelist */
  gpr_mu_lock(&fd_freelist_mu);
  fd->freelist_next = fd_freelist;
  fd_freelist = fd;
  gpr_mu_unlock(&fd_freelist_mu);
}
```
### fd_orphan()
fd_orphan() 函数释放 fd
```
static void fd_orphan(grpc_fd* fd, grpc_closure* on_done, int* release_fd,
                      const char* reason) {
  bool is_fd_closed = false;

  gpr_mu_lock(&fd->orphan_mu);

  // Get the fd->pollable_obj and set the owner_orphaned on that pollable to
  // true so that the pollable will no longer access its owner_fd field.
  gpr_mu_lock(&fd->pollable_mu);
  pollable* pollable_obj = fd->pollable_obj;

  if (pollable_obj) {
    gpr_mu_lock(&pollable_obj->owner_orphan_mu);
    pollable_obj->owner_orphaned = true;
  }

  fd->on_done_closure = on_done;

  /* If release_fd is not NULL, we should be relinquishing control of the file
     descriptor fd->fd (but we still own the grpc_fd structure). */
  if (release_fd != nullptr) {
    // Remove the FD from all epolls sets, before releasing it.
    // Otherwise, we will receive epoll events after we release the FD.
    epoll_event ev_fd;
    memset(&ev_fd, 0, sizeof(ev_fd));
    if (pollable_obj != nullptr) {  // For PO_FD.
      epoll_ctl(pollable_obj->epfd, EPOLL_CTL_DEL, fd->fd, &ev_fd);
    }
    for (size_t i = 0; i < fd->pollset_fds.size(); ++i) {  // For PO_MULTI.
      const int epfd = fd->pollset_fds[i];
      epoll_ctl(epfd, EPOLL_CTL_DEL, fd->fd, &ev_fd);
    }
    *release_fd = fd->fd;
  } else {
    close(fd->fd);
    is_fd_closed = true;
  }

  // TODO(sreek): handle fd removal (where is_fd_closed=false)
  if (!is_fd_closed) {
    GRPC_FD_TRACE("epoll_fd %p (%d) was orphaned but not closed.", fd, fd->fd);
  }

  /* Remove the active status but keep referenced. We want this grpc_fd struct
     to be alive (and not added to freelist) until the end of this function */
  REF_BY(fd, 1, reason);

  grpc_core::ExecCtx::Run(DEBUG_LOCATION, fd->on_done_closure, GRPC_ERROR_NONE);

  if (pollable_obj) {
    gpr_mu_unlock(&pollable_obj->owner_orphan_mu);
  }

  gpr_mu_unlock(&fd->pollable_mu);
  gpr_mu_unlock(&fd->orphan_mu);

  UNREF_BY(fd, 2, reason); /* Drop the reference */
}
```