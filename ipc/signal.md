## 概述
信号本质
- 信号是在软件层次上对中断机制的一种模拟，是一种异步通信方式，信号可以在用户空间进程和内核之间直接交互。在原理上，一个进程收到一个信号与处理器收到一个中断请求可以说是一样的。
- 信号可以在任何时候发送给某一进程，而无须知道该进程的状态。如果该进程并未处于执行状态，则信号就由内核保存起来，直到该进程恢复执行并传递给它为止。如果一个信号被进程设置为阻塞，则该信号的传递被延迟，直到其阻塞被取消时才被传递给进程。

信号来源
- 硬件来源：比如按下键盘或者其他硬件故障
- 软件来源：最常用发送信号的系统函数是 kill()，raise()，alarm() 和 settimer() 以及 sigqueue() 函数，软件来源还包括一些非法运算等操作。

进程对信号的响应
- 忽略信号。大多数信号都可以使用这种方式进行处理，但有两种信号不能被忽略。它们是 SGIKILL 和 SIGSTOP。这两种信号不能忽略的原因是：它们向内核和超级用户提供了进程终止的可靠方法。
- 捕获信号。定义信号处理函数，当信号发生时，执行相应的处理函数。不能捕获 SGIKILL 和 SIGSTOP 信号。
- 执行系统默认动作。Linux 对每种信号都规定了默认操作。进程对实时信号的缺省反应是进程终止。

常用信号含义及默认操作

| 信号名  | 含义                       |  默认操作   |
| :-----: | :------------------------- | :---------: |
| SIGABRT | 异常终止 abort             | 终止 + core |
| SIGALRM | 定时器超时 alarm()         |    终止     |
| SIGCHLD | 子进程结束时，向父进程发出 |    忽略     |
| SIGEMT  | 硬件故障                   | 终止 + core |
| SIGFPE  | 算术异常                   | 终止 + core |
| SIGHUP  | 用户终端连接结束时发出     |    终止     |
| SIGILL  | 非法硬件指令               | 终止 + core |
| SIGINT  | 用户按下中断键 Ctrl + C    |    终止     |
| SIGKILL | 终止                       |    终止     |
| SIGQUIT | 用户按下退出键 Ctrl + \    | 终止 + core |
| SIGSTOP | 停止                       |  停止进程   |
| SIGSTP  | 用户按下挂起键 Ctrl + Z    |  停止进程   |
| SIGURG  | 紧急情况（套接字）         |    忽略     |
| SIGUSR1 | 用户定义信号               |    终止     |
| SIGUSR2 | 用户定义信号               |    终止     |

## 信号的种类
可以从两个不同的分类角度对信号进行分类
- 可靠性
  - 可靠信号与不可靠信号
- 实时性
  - 实时信号与非实时信号
  
### 可靠信号与不可靠信号
不可靠信号（信号值小于 SIGRTMIN）
- 进程每次处理信号后，就将对信号的响应设置为默认动作
- 信号可能丢失

可靠信号（信号值位于 SIGRTMIN 与 SIGRTMAX 之间）
- 克服了信号可能丢失问题
- 支持新版本的 sigaction() 信号安装函数

### [实时信号与非实时信号](##Posix实时信号)
Posix 标准的一部分

## 信号的发送
发送信号的函数主要有 kill()、raise()、sigqueue()、alarm()、pause()、setitimer() 以及 abort()

### kill() 和 raise() 函数
```
#include <signal.h>
/* @param
 * pid：进程 ID
 *    pid >  0：将该信号发送给进程 ID 为 pid 的进程
 *    pid == 0：将该信号发送给与发送进程属于同一进程组的所有进程，而且发送进程具有权限向这些进程发送信号
 *    pid <  0：将该信号发送给其进程组 ID 等于 pid 绝对值，而且发送进程具有权限向这些进程发送信号的所有进程
 *    pid == -1：将该信号发送给发送进程有权限向它们发送信号的所有进程
 * signo：信号值，为 0 不发送任何信号，但照常进行错误检查
 * return：成功返回 0，失败返回- 1
 */
int kill(pid_t pid, int signo);

/* @param
 * signo：信号值，为 0 不发送任何信号，但照常进行错误检查
 * return：成功返回 0，失败返回 -1
 */
inti raise(int signo); /* 等同于 kill(getpid(), signo);*/
```

### sigqueue() 函数
使用排队信号必须做以下几个操作
- 使用 sigaction() 函数安装信号处理程序时指定 SA_SIGINFO 标志。如果没有这个标志，信号会延迟，但信号是否进入队列要取决于具体实现
- 在 sigaction() 结构的 sa_sigaction 成员中（而不是通常的 sa_handler 字段）提供信号处理程序。实现可能允许用户使用 sa_handler 字段，但不能获取 sigqueue() 函数发送的额外信息
- 使用 sigqueue() 函数发送信号

sigqueue() 只能把信号发送给单个进程，可以使用 value 参数向信号处理程序传递整数和指针值。除此之外，sigqueue() 函数与 kill() 函数类似。
```
#include <signal.h>
/* @param
 * pid：接收进程 ID
 * signo：信号值，0 不会发送任何信号，但会执行错误检查
 * value：向信号处理程序传递整数和指针值
 * return：成功返回 0，失败返回 -1
 */
int sigqueue(pid_t pid, int signo, const union sigval value);

typedef union sigval {
  int   sival_int;
  void *sival_ptr;
}sigval_t;
```

### alarm() 和 pause() 函数
使用 alarm() 函数可以设置一个定时器，在将来的某个时刻该定时器会超时。当定时器超时时，产生 SIGALRM 信号。pause() 函数使调用进程挂起直至捕获到一个信号。调用进程只有执行了一个信号处理程序并从其返回时，pause() 才返回。在这种情况下，pause() 返回 -1，errno 设置为 EINTR。
```
#include <unistd.h>
/* @param
 * seconds：产生 SIGALRM 需要经过的时钟秒数，0 将取消以前的闹钟时间
 * return：0 或以前设置闹钟时间的余留秒数
 */
unsigned int alarm(unsigned int seconds);

int pause(void);
```

### setitimer() 函数
setitimer() 比 alarm() 功能强大，支持 3 种类型的定时器
- ITIMER_REAL：设定绝对时间
- ITIMER_VIRTUAL：设定程序执行时间
- ITIMER_PROF：设定进程执行以及内核因本进程而消耗的时间和

```
#include <sys/time.h>
int getitimer(int which, struct itimerval *cur_value); /* 成功返回 0，失败返回 -1*/
int setitimer(int which, const struct itimerval *new_value, struct itimer *old_value); /* 成功返回 0，失败返回 -1*/

struct itimerval {
  struct timeval it_interval; /* interval for periodic timer */
  struct timeval it_value;    /* time next expriation */
};
struct timeval {
  time_t      tv_sec;         /* seconds */
  suseconds_t tv_usec;        /* microseconds */
}
```

### abort() 函数
向进程发送 SIGABORT 信号，默认情况下进程会异常退出，当然可定义自己的信号处理函数。即使 SIGABORT 被进程设置为阻塞信号，调用 abort() 后， SIGABORT 仍然能被进程接收。
```
#include <stdlib.h>
void abort(void);
```

## 信号处理程序的安装
Linux 主要有两个函数实现信号的安装：signal() 和 sigaction()。其中，signal() 只有两个参数，不支持信号传递信息，主要是用于前 32 种非实时信号的安装；而 sigaction() 是较新的函数，有三个参数，支持信号传递信息，主要用来与 sigqueue() 系统调用配合使用，当然，sigaction() 同样支持非实时信号的安装。sigaction() 优于 signal() 主要体现在支持信号带有参数。

###  signal() 函数
```
#include <signal.h>
void (*signal(int signo, void (*sa_handler)(int)))(int);
```

###  sigaction() 函数
 sigaction() 函数的功能是检查或修改（或检查并修改）与指定信号相关的处理动作。此函数取代了早期的 signal()
```
#include <signal.h>

/* @param
 * signo：传递信号信息
 * act：信号处理动作
 * oldact：获取之前注册的信号处理的指针，若不需要则传递 0
 * return：成功时返回 0，失败时返回 -1
 */
int sigaction(int signo, const struct sigaction *act, struct sigaction *oldact);

struct sigaction {
  void (*sa_hanlder)(int);
  void (*sa_sigaction)(int, siginfo_t *, void *);
  sigset_t sa_mask;
  int sa_flags;
  void (*sa_restorer)(void);
};
```
sa_mask 指定在信号处理程序执行过程中，哪些信号应当被阻塞。缺省情况下当前信号本身被阻塞，防止信号的嵌套发送，除非指定 SA_NODEFER 或者 SA_NOMASK 标志位。

sa_flags 中包含了许多标志位，包括刚刚提到的 SA_NODEFER 及 SA_NOMASK 标志位。另一个比较重要的标志位是 SA_SIGINFO，当设定了该标志位时，表示信号附带的参数可以被传递到信号处理函数中。因此，应该为 sigaction() 结构中的 sa_sigaction 指定处理函数，而不应该为 sa_handler 指定信号处理函数，否则，设置该标志变得毫无意义。即使为 sa_sigaction 指定了信号处理函数，如果不设置 SA_SIGINFO，信号处理函数同样不能得到信号传递过来的数据，在信号处理函数中对这些信息的访问都将导致段错误（Segmentation fault）

## 信号集
信号集用来描述信号的集合，Linux 所支持的所有信号可以全部或部分的出现在信号集中，主要与信号阻塞相关函数配合使用。下面是信号集操作函数
```
#include <signal.h>
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigaddset(sigset_t *set, int signo);
int sigdelset(sigset_t *set, int signo);
int sigismember(cosnt sigset_t *set, int signo);
```

| 函数        | 操作                                        | 返回值                |
| :---------- | :------------------------------------------ | :-------------------- |
| sigemptyset | 初始化由 set 指向的信号集，清除其中所有信号 | 成功返回 0，失败返回 -1 |
| sigfillset  | 初始化由 set 指向的信号集，使其包括所有信号 | 成功返回 0，失败返回 -1 |
| sigaddset   | 将一个信号添加到由 set 指向的信号集         | 成功返回 0，失败返回 -1 |
| sigdelset   | 将一个信号从由 set 指向的信号集中删除       | 成功返回 0，失败返回 -1 |
| sigismember | 测试某信号是否在由 set 指向的信号集中       | 真返回 1，假返回0      |

## sigprocmask() 函数
调用 sigprocmask() 可以检测或更改，或同时进行检测和更改进程的信号屏蔽字。
```
#include <signal.h>
/* @param
 * how：指定操作
 *     SIG_BLOCK：在进程当前阻塞信号集中添加 set 指向的信号集中的信号
 *     SIG_UNBLOCK：如果进程阻塞信号集中包含 set 指向信号集的信号，则解除对该信号的阻塞
 *     SIG_SETMASK：更新进程阻塞信号集为 set 指向的信号集
 * set：若为空，不改变进程的信号阻塞集
 * oset：若非空，进程当前阻塞信号集通过 oset 返回
 * return：成功返回 0，失败返回 -1
 */
int sigprocmask(int how, const sigset_t *set, sigset_t *oset);
```
在调用 sigprocmask() 后如果有任何未决的、不再阻塞的信号，则在 sigprocmask() 返回前，至少将其中之一传递给该进程

## sigpending() 函数
sigpending() 函数返回一信号集，对于调用进程而言，其中的各信号是阻塞不能递送的，因而也一定是当前未决的。该信号集通过 set 参数返回
```
#include <signal.h>
int sigpending(sigset_t *set); /* 成功返回 0，失败返回 -1 */
```

## sigsuspend() 函数
```
#include <signal.h>
int sigsuspend(const sigset_t *sgmask); /* 返回 -1，并将 errno 设置为 EINTR */
```
进程的信号屏蔽字设置为由 sigmask 指向的值。在捕获一个信号或发生了一个会终止该进程的信号之前，该进程被挂起。如果捕捉到一个信号而且从该信号处理程序返回，则 sigsuspend() 返回，并且该进程的信号屏蔽字设置为调用 sigsuspend() 之前的值。此函数没有成功返回值，如果返回到调用者，则总是返回-1，并将 errno 设置为 EINTR。

## sigwait() 函数
```
#include <signal.h>
/* @param
 * set：信号集，等待的信号
 * sig：返回的信号
 * return：成功返回 0,失败返回一个正数
 */
int sigwait(const sigset_t *set, int *sig);
```
sigwait() 挂起调用线程，直到 set 指定的信号之一变为未决（pending signal）。sigwait() 从 set 中选择一个未决信号，以原子方式从系统中的待处理信号集中清除它，并通过 sig 返回，如果有多个信号排队等待，则第一个信号会被返回，其余信号保持排队。

## 信号生命周期
1. 信号诞生：触发信号的事件发生
2. 信号在进程中注册完毕：进程的 task_struct 结构中有关于本进程中未决信号的数据成员
3. 信号在进程中的注销：在目标进程执行过程中，会检测是否有信号等待处理（每次从系统空间返回到用户空间时都做这样的检查）。如果存在未决信号等待处理且该信号没有被进程阻塞，则在运行相应的信号处理函数前，进程会把信号在未决信号链中占有的结构卸掉
4. 信号终止：信号处理程序执行完毕