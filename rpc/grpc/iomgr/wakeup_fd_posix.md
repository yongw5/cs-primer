# wakeup_fd_posix
## grpc_wakeup_fd
grpc_wakeup_fd 用于线程间的唤醒
```
typedef struct grpc_wakeup_fd grpc_wakeup_fd;

struct grpc_wakeup_fd {
  int read_fd;
  int write_fd;
};
```
## grpc_wakeup_fd_vtable
grpc_wakeup_fd_vtable 定义了 grpc_wakeup_fd 的接口，不同平台有不同的实现。本文会介绍两种：一种基于 pipe 的实现，另一种基于 eventfd 的实现。
```
typedef struct grpc_wakeup_fd_vtable {
  grpc_error_handle (*init)(grpc_wakeup_fd* fd_info);
  grpc_error_handle (*consume)(grpc_wakeup_fd* fd_info);
  grpc_error_handle (*wakeup)(grpc_wakeup_fd* fd_info);
  void (*destroy)(grpc_wakeup_fd* fd_info);
  /* Must be called before calling any other functions */
  int (*check_availability)(void);
} grpc_wakeup_fd_vtable;
```
## grpc_pipe_wakeup_fd_vtable
```
/// wakeup_fd_pipe.cc
const grpc_wakeup_fd_vtable grpc_pipe_wakeup_fd_vtable = {
    pipe_init, pipe_consume, pipe_wakeup, pipe_destroy,
    pipe_check_availability};
```
### pipe_init()
```
static grpc_error_handle pipe_init(grpc_wakeup_fd* fd_info) {
  int pipefd[2];
  int r = pipe(pipefd);
  if (0 != r) {
    gpr_log(GPR_ERROR, "pipe creation failed (%d): %s", errno, strerror(errno));
    return GRPC_OS_ERROR(errno, "pipe");
  }
  grpc_error_handle err;
  err = grpc_set_socket_nonblocking(pipefd[0], 1);
  if (err != GRPC_ERROR_NONE) return err;
  err = grpc_set_socket_nonblocking(pipefd[1], 1);
  if (err != GRPC_ERROR_NONE) return err;
  fd_info->read_fd = pipefd[0];
  fd_info->write_fd = pipefd[1];
  return GRPC_ERROR_NONE;
}
```
### pipe_destroy()
```
static void pipe_destroy(grpc_wakeup_fd* fd_info) {
  if (fd_info->read_fd != 0) close(fd_info->read_fd);
  if (fd_info->write_fd != 0) close(fd_info->write_fd);
}
```
### pipe_wakeup()
pipe_wakeup() 函数向 write_fd 写入一个字符
```
static grpc_error_handle pipe_wakeup(grpc_wakeup_fd* fd_info) {
  char c = 0;
  while (write(fd_info->write_fd, &c, 1) != 1 && errno == EINTR) {
  }
  return GRPC_ERROR_NONE;
}
```
### pipe_consume()
pipe_consume() 函数从 read_fd 中读取所有数据
```
static grpc_error_handle pipe_consume(grpc_wakeup_fd* fd_info) {
  char buf[128];
  ssize_t r;

  for (;;) {
    r = read(fd_info->read_fd, buf, sizeof(buf));
    if (r > 0) continue;
    if (r == 0) return GRPC_ERROR_NONE;
    switch (errno) {
      case EAGAIN:
        return GRPC_ERROR_NONE;
      case EINTR:
        continue;
      default:
        return GRPC_OS_ERROR(errno, "read");
    }
  }
}
```
## grpc_specialized_wakeup_fd_vtable
借用 eventfd 实现，和 pipe 相比，eventfd 只消耗一个文件描述符，读写都在同一个文件描述符。
```
const grpc_wakeup_fd_vtable grpc_specialized_wakeup_fd_vtable = {
    eventfd_create, eventfd_consume, eventfd_wakeup, eventfd_destroy,
    eventfd_check_availability};
```
### eventfd_create
```
static grpc_error_handle eventfd_create(grpc_wakeup_fd* fd_info) {
  fd_info->read_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  fd_info->write_fd = -1;
  if (fd_info->read_fd < 0) {
    return GRPC_OS_ERROR(errno, "eventfd");
  }
  return GRPC_ERROR_NONE;
}
```
### event_destroy()
```
static void eventfd_destroy(grpc_wakeup_fd* fd_info) {
  if (fd_info->read_fd != 0) close(fd_info->read_fd);
}
```
### event_wakeup()
一次可以向 eventfd 写入一个 eventfd_t（uint64_t）类型的值。
```
static grpc_error_handle eventfd_wakeup(grpc_wakeup_fd* fd_info) {
  GPR_TIMER_SCOPE("eventfd_wakeup", 0);
  int err;
  do {
    err = eventfd_write(fd_info->read_fd, 1);
  } while (err < 0 && errno == EINTR);
  if (err < 0) {
    return GRPC_OS_ERROR(errno, "eventfd_write");
  }
  return GRPC_ERROR_NONE;
}
```
### eventfd_consume()
```
static grpc_error_handle eventfd_consume(grpc_wakeup_fd* fd_info) {
  eventfd_t value;
  int err;
  do {
    err = eventfd_read(fd_info->read_fd, &value);
  } while (err < 0 && errno == EINTR);
  if (err < 0 && errno != EAGAIN) {
    return GRPC_OS_ERROR(errno, "eventfd_read");
  }
  return GRPC_ERROR_NONE;
}
```