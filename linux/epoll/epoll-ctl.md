## epoll_ctl(2)
```
/// @file fs/eventpoll.c
1833 SYSCALL_DEFINE4(epoll_ctl, int, epfd, int, op, int, fd,
1834         struct epoll_event __user *, event)
1835 {
1836     int error;
1837     int full_check = 0;
1838     struct fd f, tf;
1839     struct eventpoll *ep;
1840     struct epitem *epi;
1841     struct epoll_event epds;
1842     struct eventpoll *tep = NULL;
1843 
1844     error = -EFAULT;
1845     if (ep_op_has_event(op) && // 如果不是删除操作，就需要将事件拷贝到内核空间
1846         copy_from_user(&epds, event, sizeof(struct epoll_event)))
1847         goto error_return;
1848 
1849     error = -EBADF;
1850     f = fdget(epfd);
1851     if (!f.file)  // epfd 绑定的 file 对象
1852         goto error_return;
1853 
1854     /* Get the "struct file *" for the target file */
1855     tf = fdget(fd);
1856     if (!tf.file) // 需要监视文件绑定的 file 对象
1857         goto error_fput;
1858 
1859     /* The target file descriptor must support poll */
1860     error = -EPERM;
1861     if (!tf.file->f_op->poll) // 监视文件不支持 poll 机制，无法处理
1862         goto error_tgt_fput;
1863 
1864     /* Check if EPOLLWAKEUP is allowed */
1865     if (ep_op_has_event(op))
1866         ep_take_care_of_epollwakeup(&epds);
1867 
1873     error = -EINVAL;
1874     if (f.file == tf.file || !is_file_epoll(f.file)) // 不能监视自己
1875         goto error_tgt_fput;
1876 
1881     ep = f.file->private_data; // 从 file 对象中找到 eventpoll 对象
1882 
1898     mutex_lock_nested(&ep->mtx, 0);
1899     if (op == EPOLL_CTL_ADD) { // 新增操作，下面操作不明真相
1900         if (!list_empty(&f.file->f_ep_links) || // 
1901                         is_file_epoll(tf.file)) { // 注册文件是 eventpoll 文件
1902             full_check = 1;
1903             mutex_unlock(&ep->mtx);
1904             mutex_lock(&epmutex);
1905             if (is_file_epoll(tf.file)) {
1906                 error = -ELOOP;
1907                 if (ep_loop_check(ep, tf.file) != 0) {
1908                     clear_tfile_check_list();
1909                     goto error_tgt_fput;
1910                 }
1911             } else
1912                 list_add(&tf.file->f_tfile_llink,
1913                             &tfile_check_list);
1914             mutex_lock_nested(&ep->mtx, 0);
1915             if (is_file_epoll(tf.file)) {
1916                 tep = tf.file->private_data;
1917                 mutex_lock_nested(&tep->mtx, 1);
1918             }
1919         }
1920     }
1921 
1927     epi = ep_find(ep, tf.file, fd); // 在红黑树查找 fd
1928 
1929     error = -EINVAL;
1930     switch (op) {
1931     case EPOLL_CTL_ADD:
1932         if (!epi) { // 未注册则添加
1933             epds.events |= POLLERR | POLLHUP;
1934             error = ep_insert(ep, &epds, tf.file, fd, full_check);
1935         } else // 已经注册，不能重复注册，出错，
1936             error = -EEXIST;
1937         if (full_check)
1938             clear_tfile_check_list();
1939         break;
1940     case EPOLL_CTL_DEL: // 删除
1941         if (epi) // 存在则删除
1942             error = ep_remove(ep, epi);
1943         else // 删除未注册文件
1944             error = -ENOENT;
1945         break;
1946     case EPOLL_CTL_MOD: // 修改
1947         if (epi) { // 已经注册则修改
1948             epds.events |= POLLERR | POLLHUP;
1949             error = ep_modify(ep, epi, &epds);
1950         } else // 修改不存在文件，出错
1951             error = -ENOENT;
1952         break;
1953     }
1954     if (tep != NULL)
1955         mutex_unlock(&tep->mtx);
1956     mutex_unlock(&ep->mtx);
1958 error_tgt_fput:
1959     if (full_check)
1960         mutex_unlock(&epmutex);
1961 
1962     fdput(tf);
1963 error_fput:
1964     fdput(f);
1965 error_return:
1966 
1967     return error;
1968 }
```
在添加监视事件的时候，首先要保证没有注册过，如果存在，返回 -EEXIST 错误。不过，检测是否注册不仅仅依靠文件描述符，还会查看其绑定的 file 对象的地址。默认设置对目标文件的 POLLERR 和 POLLHUP 监听事件，然后调用 ep_insert() 函数，其函数核心的两个工作是：（1）将回调函数加入到要监视的文件文件描述符的等待队列上；（2）将要监听事件插入到的红黑树里面。
```
/// @file fs/eventpoll.c
1277 static int ep_insert(struct eventpoll *ep, struct epoll_event *event,
1278              struct file *tfile, int fd, int full_check)
1279 {
1280     int error, revents, pwake = 0;
1281     unsigned long flags;
1282     long user_watches;
1283     struct epitem *epi;
1284     struct ep_pqueue epq; 
1285     // 最多内核空间的 4% 给 eventpoll 使用，每个事件占用
         // (sizeof(struct epitem) + sizeof(struct eppoll_entry))
         // 在 64 位系统中占用 128 + 72 = 200 字节
1286     user_watches = atomic_long_read(&ep->user->epoll_watches);
1287     if (unlikely(user_watches >= max_user_watches))
1288         return -ENOSPC;
1289     if (!(epi = kmem_cache_alloc(epi_cache, GFP_KERNEL))) // 分配 epitem
1290         return -ENOMEM;
1291 
1292     /* Item initialization follow here ... */
1293     INIT_LIST_HEAD(&epi->rdllink);
1294     INIT_LIST_HEAD(&epi->fllink);
1295     INIT_LIST_HEAD(&epi->pwqlist);
1296     epi->ep = ep; // 所属 eventpoll
1297     ep_set_ffd(&epi->ffd, tfile, fd); // 设置监视文件
1298     epi->event = *event;              // 设置监视事件
1299     epi->nwait = 0;
1300     epi->next = EP_UNACTIVE_PTR;
1301     if (epi->event.events & EPOLLWAKEUP) {
1302         error = ep_create_wakeup_source(epi);
1303         if (error)
1304             goto error_create_wakeup_source;
1305     } else {
1306         RCU_INIT_POINTER(epi->ws, NULL);
1307     }
1308 
1309     /* Initialize the poll table using the queue callback */
1310     epq.epi = epi;
1311     init_poll_funcptr(&epq.pt, ep_ptable_queue_proc);
1312     // 上面函数的操作
         // epq.pt._qproc = ep_ptable_queue_proc; epq.pt._key = ~0UL;

1320     revents = ep_item_poll(epi, &epq.pt);
1321 
1322     /*
1323      * We have to check if something went wrong during the poll wait queue
1324      * install process. Namely an allocation for a wait queue failed due
1325      * high memory pressure.
1326      */
1327     error = -ENOMEM;
1328     if (epi->nwait < 0)
1329         goto error_unregister;
1330 
1331     /* Add the current item to the list of active epoll hook for this file */
1332     spin_lock(&tfile->f_lock);
1333     list_add_tail_rcu(&epi->fllink, &tfile->f_ep_links);
1334     spin_unlock(&tfile->f_lock);
1335 
1340     ep_rbtree_insert(ep, epi); // 插入到红黑树
1341 
1342     /* now check if we've created too many backpaths */
1343     error = -EINVAL;
1344     if (full_check && reverse_path_check())
1345         goto error_remove_epi;
1346 
1347     /* We have to drop the new item inside our item list to keep track of it */
1348     spin_lock_irqsave(&ep->lock, flags);
1349 
1350      // 如果事件就绪且没有添加到就绪链表
1351     if ((revents & event->events) && !ep_is_linked(&epi->rdllink)) {
1352         list_add_tail(&epi->rdllink, &ep->rdllist); // 添加到就绪链表
1353         ep_pm_stay_awake(epi);
1354 
1355         /* Notify waiting tasks that events are available */
1356         if (waitqueue_active(&ep->wq))
1357             wake_up_locked(&ep->wq);
1358         if (waitqueue_active(&ep->poll_wait))
1359             pwake++;
1360     }
1361 
1362     spin_unlock_irqrestore(&ep->lock, flags);
1363 
1364     atomic_long_inc(&ep->user->epoll_watches);
1365 
1366     /* We have to call this outside the lock */
1367     if (pwake)
1368         ep_poll_safewake(&ep->poll_wait);
1369 
1370     return 0;
/// ...
1398     return error;
1399 }
```
ep_item_poll(epi, &epq.pt) 做了什么
```
/// @file fs/eventpoll.c
801 static inline unsigned int ep_item_poll(struct epitem *epi, poll_table *pt)
802 {
803     pt->_key = epi->event.events;
804 
805     return epi->ffd.file->f_op->poll(epi->ffd.file, pt) & epi->event.events;
806 }
```
可以看到，调用目标文件的 poll 函数，函数指针 pt->_qproc 指向的是 ep_ptable_queue_proc()。下面看该函数的操作
```
/// @file fs/eventpoll.c
1101 static void ep_ptable_queue_proc(struct file *file, wait_queue_head_t *whead,
1102                  poll_table *pt)
1103 {
1104     struct epitem *epi = ep_item_from_epqueue(pt);
1105     struct eppoll_entry *pwq;
1106 
1107     if (epi->nwait >= 0 && (pwq = kmem_cache_alloc(pwq_cache, GFP_KERNEL))) {
1108         init_waitqueue_func_entry(&pwq->wait, ep_poll_callback);
1109         pwq->whead = whead;
1110         pwq->base = epi;
1111         add_wait_queue(whead, &pwq->wait);
1112         list_add_tail(&pwq->llink, &epi->pwqlist);
1113         epi->nwait++;
1114     } else { // 出错
1115         /* We have to signal that an error occurred */
1116         epi->nwait = -1;
1117     }
1118 }
```
可以看到，ep_ptable_queue_proc() 将自己 epitem 对象通过 eppoll_entry 对象挂到目标文件的等待（阻塞队列）上
<img src='./imgs/ep-ptable-queue-proc.png'>

我们再来看回调函数 ep_poll_callback() 做了什么
```
/// @file fs/eventpoll.c
1007 static int ep_poll_callback(wait_queue_t *wait, unsigned mode, int sync, void *key)
1008 {
1009     int pwake = 0;
1010     unsigned long flags;
1011     struct epitem *epi = ep_item_from_wait(wait); // wait==>eppoll_entry==>epitem
1012     struct eventpoll *ep = epi->ep; // 所属 eventpoll
1013 
1014     spin_lock_irqsave(&ep->lock, flags);
1014
1022     if (!(epi->event.events & ~EP_PRIVATE_BITS)) // 没有 EPOLL 可以监视的时间
1023         goto out_unlock; // 出去
1024 
1025     // key标识可本监视文件上的所有状态
1031     if (key && !((unsigned long) key & epi->event.events)) // 没有监视的事件
1032         goto out_unlock; // 出去
1033     // 如果正在将就绪事件拷贝到用户空间，暂时将 epitm 放到备选队列
1040     if (unlikely(ep->ovflist != EP_UNACTIVE_PTR)) {
1041         if (epi->next == EP_UNACTIVE_PTR) {
1042             epi->next = ep->ovflist;
1043             ep->ovflist = epi;
1044             if (epi->ws) {
1049                 __pm_stay_awake(ep->ws);
1050             }
1051 
1052         }
1053         goto out_unlock;
1054     }
1055 
1057     if (!ep_is_linked(&epi->rdllink)) { // 还没有在就绪队列，加入
1058         list_add_tail(&epi->rdllink, &ep->rdllist);
1059         ep_pm_stay_awake_rcu(epi);
1060     }
1061 
1066     if (waitqueue_active(&ep->wq)) // 唤醒阻塞在 epollevent 的 epoll_wait 进程
1067         wake_up_locked(&ep->wq);
1068     if (waitqueue_active(&ep->poll_wait))
1069         pwake++;
1070 
1071 out_unlock:
1072     spin_unlock_irqrestore(&ep->lock, flags);
1073 
1074     /* We have to call this outside the lock */
1075     if (pwake)
1076         ep_poll_safewake(&ep->poll_wait);
1077 
1078     if ((unsigned long)key & POLLFREE) { // ??
1084         list_del_init(&wait->task_list);
1085         /*
1086          * ->whead != NULL protects us from the race with ep_free()
1087          * or ep_remove(), ep_remove_wait_queue() takes whead->lock
1088          * held by the caller. Once we nullify it, nothing protects
1089          * ep/epi or even wait.
1090          */
1091         smp_store_release(&ep_pwq_from_wait(wait)->whead, NULL);
1092     }
1093 
1094     return 1;
1095 }
```
可以看到，回调函数 ep_poll_callback() 在事件就绪后，将对应的 epitem 对象添加到就绪链表中。