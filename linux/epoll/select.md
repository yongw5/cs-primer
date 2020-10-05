## select(2) 
select(2) 提供一种 fd_set 的数据结构，实际上是一个 long 类型的数组，并提供操作 fd_set 的函数 FD_ZERO()、FD_SET()、FD_CLR() 和 FD_ISSET()。fd_set 中的每一位都能与已打开的文件描述符 fd 建立联系。当调用 select(2) 时，由内核遍历 fd_set 的内容，根据 IO 状态修改 fd_set 的内容，通过将某位设置为 1 标志描述符已经就绪。

## 函数接口
```
#include <sys/select.h>
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *execeptfds,
           struct timeval *timeout);
void FD_ZERO(fd_set *set);
void FD_SET(int fd, fd_set *set);
void FD_CLR(int fd, fd_set *set);
int FD_ISSET(int fd, fd_set *set);
```

## 系统实现（Linux）
首先是相关数据结构。
```
/// @file include/uapi/linux/posix_types.h
21 #undef __FD_SETSIZE
22 #define __FD_SETSIZE    1024
23 
24 typedef struct {
25     unsigned long fds_bits[__FD_SETSIZE / (8 * sizeof(long))];
26 } __kernel_fd_set;

/// @file include/linux/types.h
14 typedef __kernel_fd_set		fd_set;
```

### sys_select()
sys_select() 会将超时时间（如果提供）从用户空间拷贝到内核空间，并且调用 poll_select_set_timeout() 将时间转换成纳秒粒度级别。然后调用 core_sys_select() 进行进一步处理，最后会调用 poll_select_copy_remaning() 将剩余时间拷贝到用户空间。
```
/// @file fs/select.c
622 SYSCALL_DEFINE5(select, int, n, fd_set __user *, inp, fd_set __user *, outp     ,
623         fd_set __user *, exp, struct timeval __user *, tvp)
624 {
625     struct timespec end_time, *to = NULL;
626     struct timeval tv;
627     int ret;
628 
629     if (tvp) {
630         if (copy_from_user(&tv, tvp, sizeof(tv)))
631             return -EFAULT;
632 
633         to = &end_time; // 纳秒粒度
634         if (poll_select_set_timeout(to,
635                 tv.tv_sec + (tv.tv_usec / USEC_PER_SEC),
636                 (tv.tv_usec % USEC_PER_SEC) * NSEC_PER_USEC))
637             return -EINVAL;
638     }
639 
640     ret = core_sys_select(n, inp, outp, exp, to); // 主要工作
641     ret = poll_select_copy_remaining(&end_time, tvp, 1, ret); // 拷贝剩余时间
642 
643     return ret;
644 }
```

### core_sys_select()
在分析前先了解一个数据结构 fd_set_bits。
```
/// @file include/linux/poll.h
 18 #define FRONTEND_STACK_ALLOC    256
 19 #define SELECT_STACK_ALLOC  FRONTEND_STACK_ALLOC

111 typedef struct {
112     unsigned long *in, *out, *ex;
113     unsigned long *res_in, *res_out, *res_ex;
114 } fd_set_bits;
```
fd_set_bits 里面封装保存监听事件和返回就绪事件的变量，都是指向 unsigned long 的指针，在轮询文件描述符的时候，以 unsigned long 为单位进行处理。接下来看 core_sys_select() 的代码，core_sys_select() 会预先在栈空间上分配 SELECT_STACK_ALLOC 个字节内存存放 fd_set_bits，如果需要处理的最大描述符过大（大于320），栈空间无法存放 fd_set_bits 数据结构，就需要从堆空间中重新分配，栈空间不使用。
```
/// @file fs/select.c
547 int core_sys_select(int n, fd_set __user *inp, fd_set __user *outp,
548                fd_set __user *exp, struct timespec *end_time)
549 {
550     fd_set_bits fds;
551     void *bits;
552     int ret, max_fds;
553     unsigned int size;
554     struct fdtable *fdt;
555     /* Allocate small arguments on the stack to save memory and be faster */
556     long stack_fds[SELECT_STACK_ALLOC/sizeof(long)]; // 256字节大小
557 
558     ret = -EINVAL;
559     if (n < 0)
560         goto out_nofds;
561 
562     /* max_fds can increase, so grab it once to avoid race */
563     rcu_read_lock();
564     fdt = files_fdtable(current->files); // fd 数组管理结构
565     max_fds = fdt->max_fds; // 当前进程打开的最大文件描述符
566     rcu_read_unlock();
567     if (n > max_fds) // 超过最大，截取，为什么不是错误
568         n = max_fds;
569 
570     /*
571      * We need 6 bitmaps (in/out/ex for both incoming and outgoing),
572      * since we used fdset we need to allocate memory in units of
573      * long-words. 
574      */
575     size = FDS_BYTES(n); // 返回用 unsigned long 对象保存 n 位需要的字节数，一定是 8 的倍数
576     bits = stack_fds;
577     if (size > sizeof(stack_fds) / 6) { // 栈空间无法存放
578         /* Not enough space in on-stack array; must use kmalloc */
579         ret = -ENOMEM;
580         bits = kmalloc(6 * size, GFP_KERNEL); // 在堆上申请更大的空间
581         if (!bits)
582             goto out_nofds;
583     } // 下面将 bits 管理的内存分配给 fd_set_bits 数据结构
584     fds.in      = bits;
585     fds.out     = bits +   size;
586     fds.ex      = bits + 2*size;
587     fds.res_in  = bits + 3*size;
588     fds.res_out = bits + 4*size;
589     fds.res_ex  = bits + 5*size;
590     // 然后将用户空间的三个 fd_set 数据拷贝到内核空间 fd_set_bits 中
591     if ((ret = get_fd_set(n, inp, fds.in)) ||
592         (ret = get_fd_set(n, outp, fds.out)) ||
593         (ret = get_fd_set(n, exp, fds.ex)))
594         goto out;
595     zero_fd_set(n, fds.res_in);  // 清空存储返回数据的内存
596     zero_fd_set(n, fds.res_out);
597     zero_fd_set(n, fds.res_ex);
598 
599     ret = do_select(n, &fds, end_time); // 轮询的主要工作
600 
601     if (ret < 0)
602         goto out;
603     if (!ret) {
604         ret = -ERESTARTNOHAND;
605         if (signal_pending(current))
606             goto out;
607         ret = 0;
608     }
609     // 将输入参数的空间当作输出空间，把结果从内核空间拷贝到用户空间
610     if (set_fd_set(n, inp, fds.res_in) ||
611         set_fd_set(n, outp, fds.res_out) ||
612         set_fd_set(n, exp, fds.res_ex))
613         ret = -EFAULT;
614 
615 out:
616     if (bits != stack_fds)
617         kfree(bits);
618 out_nofds:
619     return ret;
620 }
```

### do_select()
do_select() 用轮询的方式检测监听描述符的状态是否满足条件，若达到符合的相关条件则在返回 fd_set_bits 对应的数据域中标记该描述符。虽然该轮训的机制是死循环，但是不是一直轮训，当内核轮询一遍文件描述符没有发现任何事件就绪时，会调用 poll_schedule_timeout() 函数将自己挂起，等待相应的文件或定时器来唤醒自己，然后再继续循环体看看哪些文件已经就绪，以此减少对 CPU 的占用。我们先看轮询的过程，然后在分析如何让睡眠等待唤醒
```
/// @file fs/select.c
399 int do_select(int n, fd_set_bits *fds, struct timespec *end_time)
400 {
401     ktime_t expire, *to = NULL;
402     struct poll_wqueues table; // 用于睡眠唤醒自己的结构，后面分析
403     poll_table *wait;
404     int retval, i, timed_out = 0;
405     unsigned long slack = 0;
406     unsigned int busy_flag = net_busy_loop_on() ? POLL_BUSY_LOOP : 0;
407     unsigned long busy_end = 0;
408 
409     rcu_read_lock();
410     retval = max_select_fd(n, fds); // 获取监听的最大描述符（仍然是打开状态）
411     rcu_read_unlock();
412 
413     if (retval < 0)
414         return retval;
415     n = retval;  // 传入的文件描述符可能被意外关闭？？
416 
417     poll_initwait(&table);
418     wait = &table.pt;
419     if (end_time && !end_time->tv_sec && !end_time->tv_nsec) {
420         wait->_qproc = NULL; // 定时器唤醒自己，不需要文件来唤醒
421         timed_out = 1; // 需要超时返回处理
422     }
423 
424     if (end_time && !timed_out)
425         slack = select_estimate_accuracy(end_time); // 现在到超时时间的纳秒数
426 
427     retval = 0;
428     for (;;) { // 主循环，开始轮询
429         unsigned long *rinp, *routp, *rexp, *inp, *outp, *exp;
430         bool can_busy_loop = false;
431         
432         inp = fds->in; outp = fds->out; exp = fds->ex;
433         rinp = fds->res_in; routp = fds->res_out; rexp = fds->res_ex;
434 
435         for (i = 0; i < n; ++rinp, ++routp, ++rexp) { // 每次处理8字节（unsigned long）
436             unsigned long in, out, ex, all_bits, bit = 1, mask, j;
437             unsigned long res_in = 0, res_out = 0, res_ex = 0;
438 
439             in = *inp++; out = *outp++; ex = *exp++; // 本次处理的 8 个字节（64个描述符）
440             all_bits = in | out | ex;
441             if (all_bits == 0) { // 没有任何监听事件，处理下一个 8 字节
442                 i += BITS_PER_LONG; // i + 64
443                 continue;
444             }
445             // 否则开始每一位进行检测
446             for (j = 0; j < BITS_PER_LONG; ++j, ++i, bit <<= 1) {
447                 struct fd f;
448                 if (i >= n)
449                     break;
450                 if (!(bit & all_bits)) // 本位没有事件，下一位
451                     continue;
452                 f = fdget(i);
453                 if (f.file) { // struct file* 指针
454                     const struct file_operations *f_op;
455                     f_op = f.file->f_op; // 文件操作函数
456                     mask = DEFAULT_POLLMASK;
457                     if (f_op->poll) { // 支持 poll 
458                         wait_key_set(wait, in, out, // 设置 wait->_key
459                                  bit, busy_flag);
460                         mask = (*f_op->poll)(f.file, wait); // poll 机制，后面分析
461                     } // mask记录了该文件的就绪的事件，如果有处于监听的，就记录下来
462                     fdput(f);
463                     if ((mask & POLLIN_SET) && (in & bit)) {
464                         res_in |= bit;
465                         retval++;
466                         wait->_qproc = NULL; // 循环结束就返回
467                     }
468                     if ((mask & POLLOUT_SET) && (out & bit)) {
469                         res_out |= bit;
470                         retval++;
471                         wait->_qproc = NULL;
472                     }
473                     if ((mask & POLLEX_SET) && (ex & bit)) {
474                         res_ex |= bit;
475                         retval++;
476                         wait->_qproc = NULL;
477                     }
478                     /* got something, stop busy polling */
479                     if (retval) { // 有就绪事件，轮询结束就返回
480                         can_busy_loop = false;
481                         busy_flag = 0;
482 
483                     /*
484                      * only remember a returned
485                      * POLL_BUSY_LOOP if we asked for it
486                      */
487                     } else if (busy_flag & mask)
488                         can_busy_loop = true;
489 
490                 }
491             } // 8字节（64位）循环，下面记录结果
492             if (res_in)
493                 *rinp = res_in;
494             if (res_out)
495                 *routp = res_out;
496             if (res_ex)
497                 *rexp = res_ex;
498             cond_resched(); // 暂时放弃CPU，处理紧急事情。自己重新以抢占的形式调度回来，继续执行
499         } // 一遍轮询结束
500         wait->_qproc = NULL;
501         if (retval || timed_out || signal_pending(current))
502             break; // 有就绪时间、超时、信号事件，跳出主循环返回
503         if (table.error) { // 出错
504             retval = table.error;
505             break;
506         }
507 
508         /* only if found POLL_BUSY_LOOP sockets && not out of time */
509         if (can_busy_loop && !need_resched()) { // 可以忙等
510             if (!busy_end) {
511                 busy_end = busy_loop_end_time(); 
512                 continue;
513             }
514             if (!busy_loop_timeout(busy_end))
515                 continue;
516         }
517         busy_flag = 0;
518 
519         /*
520          * If this is the first loop and we have a timeout
521          * given, then we convert to ktime_t and set the to
522          * pointer to the expiry value.
523          */
524         if (end_time && !to) {
525             expire = timespec_to_ktime(*end_time);
526             to = &expire;
527         }
528         // 设置当前进程的状态为 TASK_INTERRUPTIBLE，可以被信号和 wake_up()
            // 以及超时唤醒的，然后投入睡眠
529         if (!poll_schedule_timeout(&table, TASK_INTERRUPTIBLE,
530                        to, slack))
531             timed_out = 1;
532     }
533 
534     poll_freewait(&table);
535 
536     return retval;
537 }
```

### *f_op->poll() 做了什么
从前面的分析中，*f_op->poll 会调用 wait->_qproc 指向的方法。我们看看 poll_initwait(&table) 做了什么
```
/// @file fs/select.c
119 void poll_initwait(struct poll_wqueues *pwq)
120 {
121     init_poll_funcptr(&pwq->pt, __pollwait);
122     pwq->polling_task = current;
123     pwq->triggered = 0;
124     pwq->error = 0;
125     pwq->table = NULL;
126     pwq->inline_index = 0;
127 }
```
init_poll_funcptr() 就是将 \__pollwait() 函数绑定到函数指针 pwq->pt 上
```
/// @file fs/select.c
219 static void __pollwait(struct file *filp, wait_queue_head_t *wait_address,
220                 poll_table *p)
221 {
222     struct poll_wqueues *pwq = container_of(p, struct poll_wqueues, pt);
223     struct poll_table_entry *entry = poll_get_entry(pwq);
224     if (!entry)
225         return;
226     entry->filp = get_file(filp);
227     entry->wait_address = wait_address;
228     entry->key = p->_key;
229     init_waitqueue_func_entry(&entry->wait, pollwake);
230     entry->wait.private = pwq;
231     add_wait_queue(wait_address, &entry->wait);
232 }
```
可以看到，\__pollwait() 是将调用进程挂到某个等待队列上，然后绑定唤醒函数为 pollwake()。


## 优缺点
- 优点
  - 跨平台
- 缺点
  - 单个进程能够监视的文件描述符的数量存在最大限制，通常是 1024。当然可以更改数量，但由于 select(2) 采用轮询的方式扫描文件描述符，文件描述符数量越多，性能越差
  - 每次调用 select(2)，都需要把 fd_set 对象从用户空间拷贝到内核空间，在返回时，从内核空间拷贝到用户空间
  - select(2) 返回的是含有整个监视的文件描述符，应用程序需要遍历整个数组才能发现哪些句柄发生了事件
  - 会（清空）修改传入的 fd_set 对象（地址传递），返回的使用当作返回空间。所以应用程序所以每次都需要重新拷贝，传入副本，以免自己维持的 fd_set 被污染。