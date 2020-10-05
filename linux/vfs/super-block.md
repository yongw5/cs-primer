## super_block 
super_block 代表一个具体某个已经挂载的文件系统（每个物理的磁盘、硬盘都有一个文件控制块 FCB，super_block 相当于 FCB 的内存映像）。标识一个文件系统的信息，比如：
- 依附的物理硬件
- 索引结点 inode 和数据块 block 的位置
- block 的大小（字节）
- 文件系统类型
- 最长文件名
- 最大文件大小
- 根目录的 inode 位置
- 支持的操作

```
/// @file include/linux/fs.h
1182 struct super_block {
1183     struct list_head                 s_list;     
1184     dev_t                            s_dev;              // 设备标识符
1185     unsigned char                    s_blocksize_bits;   // blocksize 的位数
1186     unsigned long                    s_blocksize;        // 块大小（字节）
1187     loff_t                           s_maxbytes;         // 最大文件大小（字节）
1188     struct file_system_type         *s_type;             // 文件系统类型
1189     const struct super_operations   *s_op;               // 超级块方法（函数操作）
/// ...
1193     unsigned long                    s_flags;            // 挂载标志
1194     unsigned long                    s_iflags;           // 
1195     unsigned long                    s_magic;            // 文件系统的魔术字
1196     struct dentry                   *s_root;             // 挂载点
1197     struct rw_semaphore              s_umount;           // 超级块信号量
1198     int                              s_count;            // 超级块引用计数
1199     atomic_t                         s_active;           // 活动引用计数
/// ...
1205     struct list_head                 s_inodes;           // inode链表
/// ...
1215 
1216     char                             s_id[32];           // 名字
1217     u8                               s_uuid[16];         // UUID
1219     void                            *s_fs_info;          // 文件系统特殊信息
1220     unsigned int                     s_max_links;
1221     fmode_t                          s_mode;             // 安装权限
/// ...
```

struct super_operations 中定义了超级块支持的操作，是一组函数指针
```
/// @file include/linux/fs.h
1555 struct super_operations {
1556     struct inode *(*alloc_inode)(struct super_block *sb);
1557     void (*destroy_inode)(struct inode *);
1558 
1559     void (*dirty_inode) (struct inode *, int flags);
1560     int (*write_inode) (struct inode *, struct writeback_control *wbc);
1561     int (*drop_inode) (struct inode *);
1562     void (*evict_inode) (struct inode *);
1563     void (*put_super) (struct super_block *);
1564     int (*sync_fs)(struct super_block *sb, int wait);
1565     int (*freeze_fs) (struct super_block *);
1566     int (*unfreeze_fs) (struct super_block *);
1567     int (*statfs) (struct dentry *, struct kstatfs *);
1568     int (*remount_fs) (struct super_block *, int *, char *);
1569     void (*umount_begin) (struct super_block *);
1570 
1571     int (*show_options)(struct seq_file *, struct dentry *);
1572     int (*show_devname)(struct seq_file *, struct dentry *);
1573     int (*show_path)(struct seq_file *, struct dentry *);
1574     int (*show_stats)(struct seq_file *, struct dentry *);
1575 #ifdef CONFIG_QUOTA
1576     ssize_t (*quota_read)(struct super_block *, int, char *, size_t, loff_t);
1577     ssize_t (*quota_write)(struct super_block *, int, const char *, size_t, loff_t);
1578 #endif
1579     int (*bdev_try_to_free_page)(struct super_block*, struct page*, gfp_t);
1580     long (*nr_cached_objects)(struct super_block *, int);
1581     long (*free_cached_objects)(struct super_block *, long, int);
1582 };
```
比较重要的操作
- write_inode(), alloc_inode(), destroy_inode()：写、分配和释放 inode 
- put_super()：卸载文件系统时由 VFS 系统调用，用于释放 super_block 
- sync_fs()：文件内容同步
- remount_fs()：当指定新的标识重新挂载文件系统时，VFS 调用此函数
- statfs()：获取文件系统状态，返回文件信息
