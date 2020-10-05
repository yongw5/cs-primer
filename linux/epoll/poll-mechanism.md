## 套接字文件的 poll 机制
在分析之间，先介绍 Linux 文件系统的 poll 机制。需要知道，file 结构体是从进程角度对打开文件的抽象，可以绑定不同文件类型以及文件操作。就套接字文件而言，在调用 socket(2) 后，除了创建一个套接字文件，还会创建一个 file 对象，然后将两者关联起来，file 对象的文件操作是调用套接字文件的操作。所以 *f_op->poll 函数指针其实是指向 sock_poll() 函数。接下来看这个函数的实现：
```
/// @file net/socket.c
1141 static unsigned int sock_poll(struct file *file, poll_table *wait)
1142 {
1143     unsigned int busy_flag = 0;
1144     struct socket *sock;
1145 
1146     /*
1147      *      We can't return errors to poll, so it's either yes or no.
1148      */
1149     sock = file->private_data; // file 和 socket 关联的地方
1150 
1151     if (sk_can_busy_loop(sock->sk)) { // 直接返回false，表示不能忙等
1152         /* this socket can poll_ll so tell the system call */
1153         busy_flag = POLL_BUSY_LOOP;
1154 
1155         /* once, only if requested by syscall */
1156         if (wait && (wait->_key & POLL_BUSY_LOOP))
1157             sk_busy_loop(sock->sk, 1);
1158     }
1159 
1160     return busy_flag | sock->ops->poll(file, sock, wait); // 调用
1161 }
```
同样的道理，函数指针 sock->ops->poll 也是指向其底层结构的文件操作函数。对 TCP 而言指向的是 tcp_poll() 函数，对 UDP 而言指向的是 udp_poll()。我们分析 tcp_poll() 函数：
```
/// @file net/ipv4/tcp.c
436 unsigned int tcp_poll(struct file *file, struct socket *sock, poll_table *wait)
437 {
438     unsigned int mask;
439     struct sock *sk = sock->sk;
440     const struct tcp_sock *tp = tcp_sk(sk);
441 
442     sock_rps_record_flow(sk);
443 
444     sock_poll_wait(file, sk_sleep(sk), wait); // 继续进入
445     if (sk->sk_state == TCP_LISTEN) // 如果是监听套接字，完成队列不为空就返回 POLLIN
446         return inet_csk_listen_poll(sk);
452     // 后面就是查看套接字目前位置所有发生的事件
453     mask = 0;
482     if (sk->sk_shutdown == SHUTDOWN_MASK || sk->sk_state == TCP_CLOSE)
483         mask |= POLLHUP; // 套接字关闭
484     if (sk->sk_shutdown & RCV_SHUTDOWN) // （对方发送FIN，被动）关闭接受方向
485         mask |= POLLIN | POLLRDNORM | POLLRDHUP;
486 
487     /* Connected or passive Fast Open socket? */
488     if (sk->sk_state != TCP_SYN_SENT &&
489         (sk->sk_state != TCP_SYN_RECV || tp->fastopen_rsk != NULL)) {
490         int target = sock_rcvlowat(sk, 0, INT_MAX);
491 
492         if (tp->urg_seq == tp->copied_seq &&
493             !sock_flag(sk, SOCK_URGINLINE) &&
494             tp->urg_data)
495             target++;
496 
497         /* Potential race condition. If read of tp below will
498          * escape above sk->sk_state, we can be illegally awaken
499          * in SYN_* states. */
500         if (tp->rcv_nxt - tp->copied_seq >= target)
501             mask |= POLLIN | POLLRDNORM;
502 
503         if (!(sk->sk_shutdown & SEND_SHUTDOWN)) {
504             if (sk_stream_is_writeable(sk)) {
505                 mask |= POLLOUT | POLLWRNORM;
506             } else {  /* send SIGIO later */
507                 set_bit(SOCK_ASYNC_NOSPACE,
508                     &sk->sk_socket->flags);
509                 set_bit(SOCK_NOSPACE, &sk->sk_socket->flags);
510 
511                 /* Race breaker. If space is freed after
512                  * wspace test but before the flags are set,
513                  * IO signal will be lost.
514                  */
515                 if (sk_stream_is_writeable(sk))
516                     mask |= POLLOUT | POLLWRNORM;
517             }
518         } else
519             mask |= POLLOUT | POLLWRNORM;
520 
521         if (tp->urg_data & TCP_URG_VALID)
522             mask |= POLLPRI;
523     }
525     smp_rmb();
526     if (sk->sk_err)
527         mask |= POLLERR;
528 
529     return mask; // 返回就绪的事件
530 }
```
sock_poll_wait() 调用 poll_wait() 函数
```
/// @file include/net/sock.h
1996 static inline void sock_poll_wait(struct file *filp,
1997         wait_queue_head_t *wait_address, poll_table *p)
1998 {
1999     if (!poll_does_not_wait(p) && wait_address) {
2000         poll_wait(filp, wait_address, p);
2001         /* We need to be sure we are in sync with the
2002          * socket flags modification.
2003          *
2004          * This memory barrier is paired in the wq_has_sleeper.
2005          */
2006         smp_mb();
2007     }
2008 }
```
poll_wait() 函数就调用 p->_qproc 指向的函数。需要注意的是 wait_address 就是某个文件的等待队列，可以猜想，poll_wait() 是将特定的进程挂到某个文件的等待队列上，随后会阻塞等待这个文件唤醒。此时调用进程并没有投入睡眠，要等到调用进程调用将自己的状态设置为 TASK_INTERRUPTIBLE 后调用进程才放弃 CPU 进入睡眠状态。
```
/// @file include/linux/poll.h
37 typedef struct poll_table_struct {
38     poll_queue_proc _qproc;
39     unsigned long _key;
40 } poll_table;
41 
42 static inline void poll_wait(struct file * filp, wait_queue_head_t * wait_address, poll_table *p)
43 {
44     if (p && p->_qproc && wait_address)
45         p->_qproc(filp, wait_address, p); // 调用函数
46 }
```