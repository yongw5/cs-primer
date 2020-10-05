##  inode 
索引结点 inode 包含了内核在操作文件或目录时需要的全部信息。对于 UNIX 风格的文件系统，这些信息可以根据需要从磁盘索引结点直接读入或者写会磁盘。磁盘上的一个索引结点代表一个文件，内核中一个 inode 代表打开的一个文件。
- 文件类型
- 文件大小
- 访问权限
- 访问或修改时间
- 文件位置（指向磁盘数据块）

```
/// @file include/linux/fs.h
506 struct inode {
507     umode_t                          i_mode;       // 访问权限
508     unsigned short                   i_opflags;
509     kuid_t                           i_uid;        // 使用者 ID
510     kgid_t                           i_gid;        // 使用组 ID
511     unsigned int                     i_flags;      // 文件系统标志
518     const struct inode_operations   *i_op;         // 索引结点操作
519     struct super_block              *i_sb;         // 本 inode 所属超级块
520     struct address_space            *i_mapping;    // 地址映射相关
527     unsigned long                    i_ino;        // 索引结点号
535     union {
536         const unsigned int           i_nlink;      // 连接到本 inode 的 dentry 的数目
537         unsigned int               __i_nlink;
538     };
539     dev_t                            i_rdev;       // 挂载目录的设备标识符
540     loff_t                           i_size;       // 文件大小（字节）
541     struct timespec                  i_atime;      // 最后访问时间
542     struct timespec                  i_mtime;      // 最后修改时间
543     struct timespec                  i_ctime;      // 最后改变时间
544     spinlock_t                       i_lock;       // 自旋锁
545     unsigned short                   i_bytes;      // 使用的字节数
546     unsigned int                     i_blkbits;    // blocksize 的位数
547     blkcnt_t                         i_blocks;     // 占用块数
/// ...
554     unsigned long                    i_state;      // 状文件态
/// ...
559     struct hlist_node                i_hash;
560     struct list_head                 i_wb_list;
561     struct list_head                 i_lru;        //
562     struct list_head                 i_sb_list;    // 超级块链表
563     union {
564         struct hlist_head            i_dentry;
565         struct rcu_head              i_rcu;
566     };
567     u64                              i_version;
568     atomic_t                         i_count;      // 引用计数
569     atomic_t                         i_dio_count;
570     atomic_t                         i_writecount; // 写者引用计数
571 #ifdef CONFIG_IMA
572     atomic_t                         i_readcount;  // 读者引用计数
573 #endif
581     union {
582         struct pipe_inode_info      *i_pipe;       // 具名管道文件
583         struct block_device         *i_bdev;       // 块设备文件
584         struct cdev                 *i_cdev;       // 字符设备文件
585     };
/// ...
595 };
```
inode 相关的操作
```
/// @file include/linux/fs.h
1507 struct inode_operations {
1508     struct dentry * (*lookup) (struct inode *,struct dentry *, unsigned int);
1509     void * (*follow_link) (struct dentry *, struct nameidata *);
1510     int (*permission) (struct inode *, int);
1511     struct posix_acl * (*get_acl)(struct inode *, int);
1512 
1513     int (*readlink) (struct dentry *, char __user *,int);
1514     void (*put_link) (struct dentry *, struct nameidata *, void *);
1515 
1516     int (*create) (struct inode *,struct dentry *, umode_t, bool);
1517     int (*link) (struct dentry *,struct inode *,struct dentry *);
1518     int (*unlink) (struct inode *,struct dentry *);
1519     int (*symlink) (struct inode *,struct dentry *,const char *);
1520     int (*mkdir) (struct inode *,struct dentry *,umode_t);
1521     int (*rmdir) (struct inode *,struct dentry *);
1522     int (*mknod) (struct inode *,struct dentry *,umode_t,dev_t);
1523     int (*rename) (struct inode *, struct dentry *,
1524             struct inode *, struct dentry *);
1525     int (*rename2) (struct inode *, struct dentry *,
1526             struct inode *, struct dentry *, unsigned int);
1527     int (*setattr) (struct dentry *, struct iattr *);
1528     int (*getattr) (struct vfsmount *mnt, struct dentry *, struct kstat *);
1529     int (*setxattr) (struct dentry *, const char *,const void *,size_t,int);
1530     ssize_t (*getxattr) (struct dentry *, const char *, void *, size_t);
1531     ssize_t (*listxattr) (struct dentry *, char *, size_t);
1532     int (*removexattr) (struct dentry *, const char *);
1533     int (*fiemap)(struct inode *, struct fiemap_extent_info *, u64 start,
1534               u64 len);
1535     int (*update_time)(struct inode *, struct timespec *, int);
1536     int (*atomic_open)(struct inode *, struct dentry *,
1537                struct file *, unsigned open_flag,
1538                umode_t create_mode, int *opened);
1539     int (*tmpfile) (struct inode *, struct dentry *, umode_t);
1540     int (*set_acl)(struct inode *, struct posix_acl *, int);
1541 } ____cacheline_aligned;
```
比较重要的操作
- lookup()：用于在指定目录中根据名字查找一个 inode ，该索引结点要对应于指定的文件名
- permission：访问权限
- create()：为传入 dentry 对象创建一个新的索引结点
- link()、unlink()：建立连接和删除连接
- mkdir()、rmdir()：建立目录和删除目录
- mknod()：创建一个设备文件，FIFO 或者套接字文件
- rename()：重命名