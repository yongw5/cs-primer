## file
从进程的角度，标识打开的文件。主要维持如下信息
- 文件读写的标记的位置
- 打开文件的权限
- 指向 inode 的指针

```
/// @file include/linux/fs.h
751 struct file {
752     union {
753         struct llist_node            fu_llist;
754         struct rcu_head              fu_rcuhead;
755     } f_u;
756     struct path                      f_path;        // 文件路径
757 #define f_dentry                     f_path.dentry
758     struct inode                    *f_inode;       // inode缓存
759     const struct file_operations    *f_op;          // 文件操作

765     spinlock_t                       f_lock;        // 自旋锁
766     atomic_long_t                    f_count;       // 引用计数
767     unsigned int                     f_flags;       // 打开文件指定的标志
768     fmode_t                          f_mode;        // 访问权限
769     struct mutex                     f_pos_lock;    // 文件指针锁
770     loff_t                           f_pos;         // 文件指针偏移量
771     struct fown_struct               f_owner;       // 
787     struct address_space            *f_mapping;     // 内存映射相关该
795 };
```
文件相关的操作
```
/// @file include/linux/fs.h
1474 struct file_operations {
1475     struct module *owner;
1476     loff_t (*llseek) (struct file *, loff_t, int);
1477     ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
1478     ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
1479     ssize_t (*aio_read) (struct kiocb *, const struct iovec *, unsigned long, loff_t);
1480     ssize_t (*aio_write) (struct kiocb *, const struct iovec *, unsigned long, loff_t);
1481     ssize_t (*read_iter) (struct kiocb *, struct iov_iter *);
1482     ssize_t (*write_iter) (struct kiocb *, struct iov_iter *);
1483     int (*iterate) (struct file *, struct dir_context *);
1484     unsigned int (*poll) (struct file *, struct poll_table_struct *);
1485     long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
1486     long (*compat_ioctl) (struct file *, unsigned int, unsigned long);
1487     int (*mmap) (struct file *, struct vm_area_struct *);
1488     int (*open) (struct inode *, struct file *);
1489     int (*flush) (struct file *, fl_owner_t id);
1490     int (*release) (struct inode *, struct file *);
1491     int (*fsync) (struct file *, loff_t, loff_t, int datasync);
1492     int (*aio_fsync) (struct kiocb *, int datasync);
1493     int (*fasync) (int, struct file *, int);
1494     int (*lock) (struct file *, int, struct file_lock *);
1495     ssize_t (*sendpage) (struct file *, struct page *, int, size_t, loff_t *, int);
1496     unsigned long (*get_unmapped_area)(struct file *, unsigned long, unsigned long, unsigned long, unsigned long);
1497     int (*check_flags)(int);
1498     int (*flock) (struct file *, int, struct file_lock *);
1499     ssize_t (*splice_write)(struct pipe_inode_info *, struct file *, loff_t *, size_t, unsigned int);
1500     ssize_t (*splice_read)(struct file *, loff_t *, struct pipe_inode_info *, size_t, unsigned int);
1501     int (*setlease)(struct file *, long, struct file_lock **);
1502     long (*fallocate)(struct file *file, int mode, loff_t offset,
1503               loff_t len);
1504     int (*show_fdinfo)(struct seq_file *m, struct file *f);
1505 };
```
文件常用操作
- llseek()：更新偏移位置
- read()、write()、aio_read()、aio_write()：文件读写
- poll()：进程检查此文件上是否有活动并且（可选）进入睡眠状态直到有活动时唤醒
- mmap()：内存映射
- open()：文件打开
- flush()：文件刷新
- release()：当最后一个引用的文件关闭，调用