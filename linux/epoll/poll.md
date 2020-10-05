## poll(2) 实现及其优缺点
poll(2) 和 select(2) 类似，没有本质差别，管理多个描述符也是进行轮询，根据描述符的状态进行处理。但是 poll(2) 用链表管理监视事件，没有最大描述符数量的限制，并且传入的 fds 在 poll(2) 函数返回后不会清空，活动事件记录在 revents 成员中。

## 函数接口
```
#include <poll.h>
int poll(struct pollfd *fds, nfds_t nfds, int timeout);
```

## 系统实现（Linux）
```
/// @file include/uapi/asm-generic/poll.h
35 struct pollfd {
36     int fd;
37     short events;  // 监视事件
38     short revents; // 就绪事件
39 };

/// @file fs/select.c
18 #define FRONTEND_STACK_ALLOC    256
20 #define POLL_STACK_ALLOC    FRONTEND_STACK_ALLOC

735 struct poll_list {
736     struct poll_list *next;
737     int len;
738     struct pollfd entries[0];
739 };
```

### sys_poll()
```
/// @file fs/select.c
957 SYSCALL_DEFINE3(poll, struct pollfd __user *, ufds, unsigned int, nfds,
958         int, timeout_msecs)
959 {
960     struct timespec end_time, *to = NULL;
961     int ret;
962 
963     if (timeout_msecs >= 0) { // 拷贝时间，微秒粒度
964         to = &end_time;
965         poll_select_set_timeout(to, timeout_msecs / MSEC_PER_SEC,
966             NSEC_PER_MSEC * (timeout_msecs % MSEC_PER_SEC));
967     }
968 
969     ret = do_sys_poll(ufds, nfds, to); // 主要
970 
971     if (ret == -EINTR) {
972         struct restart_block *restart_block;
973 
974         restart_block = &current_thread_info()->restart_block;
975         restart_block->fn = do_restart_poll;
976         restart_block->poll.ufds = ufds;
977         restart_block->poll.nfds = nfds;
978 
979         if (timeout_msecs >= 0) {
980             restart_block->poll.tv_sec = end_time.tv_sec;
981             restart_block->poll.tv_nsec = end_time.tv_nsec;
982             restart_block->poll.has_timeout = 1;
983         } else
984             restart_block->poll.has_timeout = 0;
985 
986         ret = -ERESTART_RESTARTBLOCK;
987     }
988     return ret;
989 }
```

### do_sys_poll()
和 select(2) 一样，poll(2) 也会预先在栈空间申请大小为 POLL_STACK_ALLOC 的内存，栈空间可以处理 30 个文件描述符。不过和 select(2) 不同的是，即使栈空间太小，要从堆上申请内存，预先分配的栈空间也是被使用的。
```
/// @file fs/select.c
870 int do_sys_poll(struct pollfd __user *ufds, unsigned int nfds,
871         struct timespec *end_time)
872 {
873     struct poll_wqueues table;
874     int err = -EFAULT, fdcount, len, size;
875     /* Allocate small arguments on the stack to save memory and be
876        faster - use long to make sure the buffer is aligned properly
877        on 64 bit archs to avoid unaligned access */
878     long stack_pps[POLL_STACK_ALLOC/sizeof(long)]; // 原先分配 256 字节栈空间
879     struct poll_list *const head = (struct poll_list *)stack_pps;
880     struct poll_list *walk = head;
881     unsigned long todo = nfds;
882 
883     if (nfds > rlimit(RLIMIT_NOFILE))
884         return -EINVAL;
885 
886     len = min_t(unsigned int, nfds, N_STACK_PPS);
887     for (;;) {
888         walk->next = NULL;
889         walk->len = len;
890         if (!len)
891             break;
892         // 从用于空间拷贝到内核空间
893         if (copy_from_user(walk->entries, ufds + nfds-todo,
894                     sizeof(struct pollfd) * walk->len))
895             goto out_fds;
896         // 栈空间大约可以处理 30 个文件描述符
897         todo -= walk->len;
898         if (!todo)
899             break;
900 
901         len = min(todo, POLLFD_PER_PAGE); // 一次最大申请一页，可以 510 个文件描述符 
902         size = sizeof(struct poll_list) + sizeof(struct pollfd) * len;
903         walk = walk->next = kmalloc(size, GFP_KERNEL); // 空间不够，申请堆空间，栈空间继续用
904         if (!walk) {
905             err = -ENOMEM;
906             goto out_fds;
907         }
908     }
909 
910     poll_initwait(&table); // 和 select 处理一样
911     fdcount = do_poll(nfds, head, &table, end_time); // 主要工作
912     poll_freewait(&table);
913     // 将结果从内核空间拷贝到用户空间
914     for (walk = head; walk; walk = walk->next) {
915         struct pollfd *fds = walk->entries;
916         int j;
917 
918         for (j = 0; j < walk->len; j++, ufds++)
919             if (__put_user(fds[j].revents, &ufds->revents))
920                 goto out_fds;
921     }
922 
923     err = fdcount;
924 out_fds:
925     walk = head->next;
926     while (walk) {
927         struct poll_list *pos = walk;
928         walk = walk->next;
929         kfree(pos);
930     }
931 
932     return err;
933 }
```

### do_poll()
和 do_select() 的原理一样
```
/// @file fs/select.c
781 static int do_poll(unsigned int nfds,  struct poll_list *list,
782            struct poll_wqueues *wait, struct timespec *end_time)
783 {
784     poll_table* pt = &wait->pt;
785     ktime_t expire, *to = NULL;
786     int timed_out = 0, count = 0;
787     unsigned long slack = 0;
788     unsigned int busy_flag = net_busy_loop_on() ? POLL_BUSY_LOOP : 0;
789     unsigned long busy_end = 0;
790 
791     /* Optimise the no-wait case */
792     if (end_time && !end_time->tv_sec && !end_time->tv_nsec) {
793         pt->_qproc = NULL;
794         timed_out = 1;
795     }
796 
797     if (end_time && !timed_out)
798         slack = select_estimate_accuracy(end_time);
799 
800     for (;;) { // 主循环
801         struct poll_list *walk;
802         bool can_busy_loop = false;
803 
804         for (walk = list; walk != NULL; walk = walk->next) {
805             struct pollfd * pfd, * pfd_end;
806 
807             pfd = walk->entries;
808             pfd_end = pfd + walk->len;
809             for (; pfd != pfd_end; pfd++) {
817                 if (do_pollfd(pfd, pt, &can_busy_loop,
818                           busy_flag)) {
819                     count++;
820                     pt->_qproc = NULL;
821                     /* found something, stop busy polling */
822                     busy_flag = 0;
823                     can_busy_loop = false;
824                 }
825             }
826         }
831         pt->_qproc = NULL;
832         if (!count) {
833             count = wait->error;
834             if (signal_pending(current))
835                 count = -EINTR;
836         }
837         if (count || timed_out)
838             break;
839 
840         /* only if found POLL_BUSY_LOOP sockets && not out of time */
841         if (can_busy_loop && !need_resched()) {
842             if (!busy_end) {
843                 busy_end = busy_loop_end_time();
844                 continue;
845             }
846             if (!busy_loop_timeout(busy_end))
847                 continue;
848         }
849         busy_flag = 0;
850 
856         if (end_time && !to) {
857             expire = timespec_to_ktime(*end_time);
858             to = &expire;
859         }
860 
861         if (!poll_schedule_timeout(wait, TASK_INTERRUPTIBLE, to, slack)) // 睡眠
862             timed_out = 1;
863     }
864     return count;
865 }
```

## 优缺点
- 优点
  - select(2) 会修改传入的 fd_set 参数，把它当作返回的空间存储返回的数据，而 poll(2) 不会，返回数据和传入的数据不互相干扰；
  - poll(2) 的描述符类型使用链表实现，没有描述符数量的限制；
- 缺点
  - 每次调用 poll(2)，都需要把 pollfd 链表从用户空间拷贝到内核空间，在返回时，将返会数据从内核空间拷贝到用户空间
  - poll(2) 返回的是含有整个 pollfd 链表，应用程序需要遍历整个链表才能发现哪些句柄发生了事件