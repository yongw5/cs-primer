## dentry
VFS 把目录当作文件对待，但是没有一个具体的磁盘结构与之对应。
```
/// @file include/linux/dcache.h
108 struct dentry {
110     unsigned int                    d_flags;                   // 目录项表示
111     seqcount_t                      d_seq;
112     struct hlist_bl_node            d_hash;
113     struct dentry                  *d_parent;                  // 父目录
114     struct qstr                     d_name;                    // 目录项名字
115     struct inode                   *d_inode;                   // 关联的inode
117     unsigned char                   d_iname[DNAME_INLINE_LEN]; // 短文件名
120     struct lockref                  d_lockref;                 // 引用计数（带锁）
121     const struct dentry_operations *d_op;                      // 目录项操作
122     struct super_block             *d_sb;                      // 所属超级块
123     unsigned long                   d_time;                    // 重置时间
124     void                           *d_fsdata;                  // 文件系统特有数据
126     struct list_head                d_lru;                     // LRU链表
127     struct list_head                d_child;                   // 父目录孩子
128     struct list_head                d_subdirs;                 // 子目录
132     union {
133         struct hlist_node           d_alias;
134         struct rcu_head             d_rcu;
135     } d_u;
136 };
```
dentry 相关操作
```
/// @file include/linux/dcache.h
150 struct dentry_operations {
151     int (*d_revalidate)(struct dentry *, unsigned int);
152     int (*d_weak_revalidate)(struct dentry *, unsigned int);
153     int (*d_hash)(const struct dentry *, struct qstr *);
154     int (*d_compare)(const struct dentry *, const struct dentry *,
155             unsigned int, const char *, const struct qstr *);
156     int (*d_delete)(const struct dentry *);
157     void (*d_release)(struct dentry *);
158     void (*d_prune)(struct dentry *);
159     void (*d_iput)(struct dentry *, struct inode *);
160     char *(*d_dname)(struct dentry *, char *, int);
161     struct vfsmount *(*d_automount)(struct path *);
162     int (*d_manage)(struct dentry *, bool);
163 } ____cacheline_aligned;
```
重要的操作
- d_compare()：比较两个文件名
- d_delete()：删除目录项
- d_release()：释放目录项
- d_prune()：
- d_iput()：当一个目录项对象丢失了其相关联的 inode 结点时，调用此函数
- d_dname()：延迟目录项生成，在真正需要时生成