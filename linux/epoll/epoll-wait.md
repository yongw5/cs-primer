## epoll_wait(2)
```
/// @file fs/eventpoll.c
1974 SYSCALL_DEFINE4(epoll_wait, int, epfd, struct epoll_event __user *, events,
1975         int, maxevents, int, timeout)
1976 {
1977     int error;
1978     struct fd f;
1979     struct eventpoll *ep;
1980 
1981     /* The maximum number of event must be greater than zero */
1982     if (maxevents <= 0 || maxevents > EP_MAX_EVENTS)
1983         return -EINVAL;
1984 
1985     /* Verify that the area passed by the user is writeable */
1986     if (!access_ok(VERIFY_WRITE, events, maxevents * sizeof(struct epoll_event)))
1987         return -EFAULT;
1988 
1989     /* Get the "struct file *" for the eventpoll file */
1990     f = fdget(epfd);
1991     if (!f.file)
1992         return -EBADF;
1993 
1994     /*
1995      * We have to check that the file structure underneath the fd
1996      * the user passed to us _is_ an eventpoll file.
1997      */
1998     error = -EINVAL;
1999     if (!is_file_epoll(f.file)) // 不是 epoll 文件
2000         goto error_fput;
2001 
2002     /*
2003      * At this point it is safe to assume that the "private_data" contains
2004      * our own data structure.
2005      */
2006     ep = f.file->private_data;
2007 
2008     /* Time to fish for events ... */
2009     error = ep_poll(ep, events, maxevents, timeout); // 主要操作
2010 
2011 error_fput:
2012     fdput(f);
2013     return error;
2014 }
```
接下来分析 ep_poll()
```
/// @file fs/eventpoll.c
1598 static int ep_poll(struct eventpoll *ep, struct epoll_event __user *events,
1599            int maxevents, long timeout)
1600 {
1601     int res = 0, eavail, timed_out = 0;
1602     unsigned long flags;
1603     long slack = 0;
1604     wait_queue_t wait;
1605     ktime_t expires, *to = NULL;
1606 
1607     if (timeout > 0) { // 设置超时时间
1608         struct timespec end_time = ep_set_mstimeout(timeout);
1609 
1610         slack = select_estimate_accuracy(&end_time);
1611         to = &expires;
1612         *to = timespec_to_ktime(end_time);
1613     } else if (timeout == 0) {
1618         timed_out = 1;
1619         spin_lock_irqsave(&ep->lock, flags);
1620         goto check_events;
1621     }
1622 
1623 fetch_events:
1624     spin_lock_irqsave(&ep->lock, flags);
1625 
1626     if (!ep_events_available(ep)) { // 就绪或备选队列为空，没有就绪队列
         // 将当前进程加入到 ep->wq 等待队列里面，然后在一个无限for循环里面，
         // 首先调用set_current_state(TASK_INTERRUPTIBLE)，将当前进程
         // 设置为可中断的睡眠状态，然后调度让当前进程就让出cpu，进入睡眠，直到有其
         //  他进程调用wake_up或者有中断信号进来唤醒本进程
1632         init_waitqueue_entry(&wait, current); // 设置睡眠时的唤醒函数
1633         __add_wait_queue_exclusive(&ep->wq, &wait);
1634 
1635         for (;;) {
1641             set_current_state(TASK_INTERRUPTIBLE); 准备睡眠
1642             if (ep_events_available(ep) || timed_out) // 有就绪或超时，退出
1643                 break;
1644             if (signal_pending(current)) { // 是信号唤起，退出，处理信号
1645                 res = -EINTR;
1646                 break;
1647             }
1648 
1649             spin_unlock_irqrestore(&ep->lock, flags);
                 // 调度，让出 CPU，投入睡眠
1650             if (!schedule_hrtimeout_range(to, slack, HRTIMER_MODE_ABS))
1651                 timed_out = 1;
1652 
1653             spin_lock_irqsave(&ep->lock, flags);
1654         }
1655         __remove_wait_queue(&ep->wq, &wait);
1656 
1657         set_current_state(TASK_RUNNING); // 被唤醒，开始处理
1658     }
1659 check_events:
1660     /* Is it worth to try to dig for events ? */
1661     eavail = ep_events_available(ep); // 是否有就绪事件
1662 
1663     spin_unlock_irqrestore(&ep->lock, flags);
1664 
1665     /*
1666      * Try to transfer events to user space. In case we get 0 events and
1667      * there's still timeout left over, we go trying again in search of
1668      * more luck.
1669      */
1670     if (!res && eavail &&
1671         !(res = ep_send_events(ep, events, maxevents)) && !timed_out)
1672         goto fetch_events;
1673 
1674     return res;
1675 }
```
如果有就绪事件发生，则调用 ep_send_events() 函数做进一步处理。
```
/// @file fs/eventpoll.c
1559 static int ep_send_events(struct eventpoll *ep,
1560               struct epoll_event __user *events, int maxevents)
1561 {
1562     struct ep_send_events_data esed;
1563 
1564     esed.maxevents = maxevents; // 用户空间大小
1565     esed.events = events;       // 用户空间地址
1566 
1567     return ep_scan_ready_list(ep, ep_send_events_proc, &esed, 0, false);
1568 }
```
继续调用 ep_scan_ready_list() 函数
```
/// @file fs/eventpoll.c
597 static int ep_scan_ready_list(struct eventpoll *ep,
598                   int (*sproc)(struct eventpoll *,
599                        struct list_head *, void *),
600                   void *priv, int depth, bool ep_locked)
601 {
602     int error, pwake = 0;
603     unsigned long flags;
604     struct epitem *epi, *nepi;
605     LIST_HEAD(txlist); // 新建一个队列
611 
612     if (!ep_locked)
613         mutex_lock_nested(&ep->mtx, depth); // 占用互斥锁
614 
623     spin_lock_irqsave(&ep->lock, flags); // 加锁
624     list_splice_init(&ep->rdllist, &txlist); // 将 fdlist 的内容转移到 txlist
625     ep->ovflist = NULL; // 初始化，标志正在拷贝数据到用户空间
626     spin_unlock_irqrestore(&ep->lock, flags); // 释放锁
627 
631     error = (*sproc)(ep, &txlist, priv); // 调用 ep_send_events_proc()
632 
633     spin_lock_irqsave(&ep->lock, flags); // 占用锁
634     // 处理备选队列，将它们拷贝到就绪队列
639     for (nepi = ep->ovflist; (epi = nepi) != NULL;
640          nepi = epi->next, epi->next = EP_UNACTIVE_PTR) {
647         if (!ep_is_linked(&epi->rdllink)) {
648             list_add_tail(&epi->rdllink, &ep->rdllist);
649             ep_pm_stay_awake(epi);
650         }
651     }
652     // 标志 ovflist 不能使用，就绪事件挂到 rdllist 上
657     ep->ovflist = EP_UNACTIVE_PTR;
658 
662     list_splice(&txlist, &ep->rdllist); // 没有拷贝的放回就绪队列
663     __pm_relax(ep->ws);
664 
665     if (!list_empty(&ep->rdllist)) {
670         if (waitqueue_active(&ep->wq)) // 有就绪事件
671             wake_up_locked(&ep->wq);
672         if (waitqueue_active(&ep->poll_wait))
673             pwake++;
674     }
675     spin_unlock_irqrestore(&ep->lock, flags);
676 
677     if (!ep_locked)
678         mutex_unlock(&ep->mtx); // 释放互斥锁
679 
680     /* We have to call this outside the lock */
681     if (pwake)
682         ep_poll_safewake(&ep->poll_wait);
683 
684     return error;
685 }
```
需要看一下 ep_send_events_proc() 的处理操作
```
/// @file fs/eventpoll.c
1479 static int ep_send_events_proc(struct eventpoll *ep, struct list_head *head,
1480                    void *priv)
1481 {   // 调用这占用了互斥锁
1482     struct ep_send_events_data *esed = priv;
1483     int eventcnt;
1484     unsigned int revents;
1485     struct epitem *epi;
1486     struct epoll_event __user *uevent;
1487     struct wakeup_source *ws;
1488     poll_table pt;
1489 
1490     init_poll_funcptr(&pt, NULL);
1491      
1497     for (eventcnt = 0, uevent = esed->events;
1498          !list_empty(head) && eventcnt < esed->maxevents;) {
1499         epi = list_first_entry(head, struct epitem, rdllink); // 从链表中获取一个 epitem
1500 
1510         ws = ep_wakeup_source(epi);
1511         if (ws) {
1512             if (ws->active)
1513                 __pm_stay_awake(ep->ws);
1514             __pm_relax(ws);
1515         }
1516 
1517         list_del_init(&epi->rdllink); // 从就绪链表中取下来
1518 
1519         revents = ep_item_poll(epi, &pt); // 再次查看发生的事件
1520 
1527         if (revents) { // 有事件发生，将当前的事件和用户传入的数据都拷贝到用户空间
1528             if (__put_user(revents, &uevent->events) ||
1529                 __put_user(epi->event.data, &uevent->data)) { // 拷贝失败
1530                 list_add(&epi->rdllink, head); // txlist的内容放回就绪链表
1531                 ep_pm_stay_awake(epi);
1532                 return eventcnt ? eventcnt : -EFAULT; // 返回出错
1533             }
1534             eventcnt++;
1535             uevent++;
1536             if (epi->event.events & EPOLLONESHOT)
1537                 epi->event.events &= EP_PRIVATE_BITS;
1538             else if (!(epi->event.events & EPOLLET)) { // 不是边缘触发，相当于默认条件触发
1539                 // 是水平触发，需要将 epitem 再次插入到就绪链表，下一次
                     // 调用 epoll_wait 会再次检查事件是否就绪。
1550                 list_add_tail(&epi->rdllink, &ep->rdllist);
1551                 ep_pm_stay_awake(epi);
1552             }
1553         }
1554     }
1555 
1556     return eventcnt;
1557 }
```