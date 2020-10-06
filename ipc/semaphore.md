## 概述
信号量是一种用于提供不同进程间或一个给定进程的不同线程间同步手段的原语

一个进程可以在某个信号量上执行的三种操作
- 创建一个信号量。这还要求调用者指定初始值。
- 等待一个信号量。该操作会测试这个信号量的值，如果其值小于或等于 0，那就等待（阻塞），一旦其值变为大于 0 就将它减 1
  ```
  while (semaphore_value <= 0); /* wait, block the thread or process*/
  semaphore_value--;
  /* we have the semaphore */
  ```
- 挂出一个信号量。该操作将信号量的值加 1，如果有一些进程阻塞着等待该信号量的值变为大于 0，其中一个进程现在就可能被唤醒。

## sem_open()、sem_close() 和 sem_unlink() 函数
函数 sem_open() 创建一个新的具名信号量或打开一个已存在的具名信号量。具名信号量总是即可用于线程间同步，又可用于进程间的同步
```
#include <semaphore.h>
/* @param
 * name：信号量名字
 * oflag：打开方式
 * return：成功返回指向信号量的指针，失败返回SEM_FAILED
 */
sem_t *sem_open(const char *name, int oflag, ...
          /* mode_t mode, unsigned int value*/);
```
ofalg 参数可以是 0 、 O_CREAT 或 O_CREAT | O_EXCL 。如果指定了 O_CREATE 标志，那么第三个和第四个参数是需要的：其中 mode 参数指定权限位，value 参数指定信号量的初始值，该初始值不能超过 SEM_VALUE_MAX（必须至少为32767）。

如果指定了 O_CREAT （而没有指定 O_EXCL ），那么只有当所需信号量尚未存在时才初始化它。

使用 sem_open() 打开的具名信号量，使用 sem_close() 将其关闭
```
#include <semaphore.h>
/* @param
 * sem：要关闭具名信号量的指针
 * return：成功返回0，失败返回-1
 */
int sem_close(sem_t *sem);
```
一个进程终止时，内核还对其上仍然打开着的所有具名信号**自动执行**这样的信号关闭操作。不论该进程是自愿终止的还是非自愿终止的，这种自动关闭都会发生。

关闭一个信号量并没有将它从系统中删除。这就是说，Posix 有名信号量至少是随内核持续的：即使当前没有进程打开着某个信号量，它的值仍然保持。具名信号量使用 sem_unlink() 从系统中删除
```
#include <semaphore.h>
int sem_unlink(const char *name); /* 成功返回0，失败返回-1*/
```
每个信号量有个引用计数记录当前的打开次数（就像文件一样）， sem_unlink 类似于文件IO的 unlink() 函数：当引用计数还是大于0时，name就能从文件系统中删除，然而其信号量的析构（不同于将它的名字从文件系统中删除）却要等到最后一个 sem_close 发生为止。

##  sem_wait() 和 sem_trywait() 函数
 sem_wait() 函数测试所指定信号量的值，如果该值大于 0，那就将它减 1 并立即返回。如果该值等于0，调用线程就被投入睡眠中，直到该值变为大于 0，这时再将它减 1，函数随后返回。”测试并减1“操作是原子的
```
#include <semaphore.h>
int sem_wait(sem_t *sem);
int sem_trywait(sem_t *sem);
int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout);
/* 均返回：成功返回0，失败返回-1，并设置errno */
```
 sem_wait() 和 sem_trywait() 的差别是：当所指定信号量的值已经是 0 时，后者并不将调用线程投入睡眠（阻塞），而是返回一个 EAGAIN 错误。 sem_timedwait() 指定了阻塞的时间限制

如果被某个信号中断， sem_wait() 就可能过早地返回，所返回的错误为 EINTR

##  sem_post() 和 sem_getvalue() 函数
当一个线程使用完某个信号量时，它应该调用 sem_post()，本函数把所指定信号量的值加1，然后唤醒正在等待该信号量变为正数的任意线程
```
#include <semaphore.h>
int sem_post(sem_t *sem);
int sem_getvalue(sem_t *sem, int *valp);
/* 均返回：成功返回 0，失败返回 -1，并设置 errno */
```
sem_getvalue() 在由 valp 指向的整数中返回所指定信号量的当前值。如果该信号量当前已上锁，那么返回值或为 0，或为某个负数，其绝对值就是等待该信号量解锁的线程数。

## 互斥锁、条件变量和信号量之间的差别
1. 互斥锁必须总是由给它上锁的线程解锁，信号量没有这种限制：一个线程可以等待某个给定信号量，而另一个线程可以挂出该信号量。
2. 每个信号量有一个与之关联的值，它由挂出操作加 1，由等待操作减 1，那么任何线程都可以挂出一个信号，即使当时没有线程在等待该信号量值变为正数也没有关系。然而，如果某个线程调用了 pthread_cond_signal()，不过当时没有任何线程阻塞在 pthread_cond_wait() 调用中，那么发往相应条件变量的信号将丢失
3. 在各种各样的同步技巧（互斥锁、条件变量、读写锁、信号量）中，能够从信号处理中安全调用的唯一函数是 sem_post()。

##  sem_init() 和 sem_destroy() 函数
Posix 也提供**基于内存**的信号量，他们由应用程序分配信号量的内存空间（也就是分配一个 sem_t 数据类型的内存空间），然后（调用 sem_init()函数）由系统初始化它们的值。使用完，调用 sem_destroy() 摧毁它
```
#include <semaphore.h>
/* @param
 * sem：指向一个已分配的sem_t变量
 * shared：如果为 0，同一进程的不同线程共享。否则在进程间共享（此时信号量必须放在某种类型的共享内存区中）
 * value：初始化值
 * return：成功返回 0，失败返回 -1
 */
int sem_init(sem_t *sem, int shared, unsigned int value);

int sem_destroy(sem_t *sem); /* 成功返回 0，失败返回 -1 */
```

信号量真正的持续性却取决于存放信号量的内存区的类型。只要含有某个基于内存信号量的内存区域保持有效，该信号量就一直存在。
- 如果某个基于内存的信号量是由单个进程内的各个线程共享的，那么该信号量具有随进程的持续性，当该进程终止时它也消失。
- 如果某个基于内存的信号量是在不同进程间共享的，那么该信号量必须存放在共享内存区中，因而只要该共享内存区仍然存在，该信号量也就继续存在。

## 进程间共享信号量
进程间共享基于内存信号量的规则很简单：信号量本身（其地址作为 sem_init() 第一个参素的 sem_t 数据类型变量）必须驻留在由所希望共享它的进程所共享的内存中，而且 sem_init() 的第二个参数必须为1。

具名信号量，不同进程通过名字即可共享

## 信号量限制
-  SEM_NSEMS_MAX ：一个进程可同时打开着的最大信号量数（Posix 要求至少为256）
-  SEM_VALUE_MAX ：一个信号量的最大值（Posix 要求至少为32767）