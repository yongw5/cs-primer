## 文件系统注册
这里的文件系统是指可能会被挂载到目录树中的各个实际文件系统，所谓实际文件系统，即是指 VFS 中的实际操作最终要通过它们来完成而已，并不意味着它们一定要存在于某种特定的存储设备上。分别通过 register_filesystem() 和 unregister_filesystem() 完成文件系统的注册和移除。注册过程实际上将表示各实际文件系统的 file_system_type 数据结构的实例化，然后形成一个链表，内核中用一个名为 file_systems 的全局变量来指向该链表的表头。
```
#include <linux/fs.h>
extern int register_filesystem(struct file_system_type *);
extern int unregister_filesystem(struct file_system_type *);
```

## file_system_type
Linxu 用 file_system_type 来表征一个文件系统的类型
```
/// @file include/linux/fs.h
1768 struct file_system_type {
1769     const char               *name;          // 名字，比如 “ext2”
1770     int                       fs_flags;      // 类型标志，在下面定义
1771 #define FS_REQUIRES_DEV       1 
1772 #define FS_BINARY_MOUNTDATA   2
1773 #define FS_HAS_SUBTYPE        4
1774 #define FS_USERNS_MOUNT       8 
1775 #define FS_USERNS_DEV_MOUNT   16
1776 #define FS_USERNS_VISIBLE     32
1777 #define FS_NOEXEC             64
1778 #define FS_RENAME_DOES_D_MOVE 32768
1779     struct dentry *(*mount) (struct file_system_type *, int,
1780                const char *, void *);        // 挂载操作
1781     void (*kill_sb) (struct super_block *);  // 在unmount时清理所属的超级块
1782     struct module            *owner;
1783     struct file_system_type  *next;          // 下一个 file_system_type
1784     struct hlist_head         fs_supers;
1785 
1786     struct lock_class_key     s_lock_key;
1787     struct lock_class_key     s_umount_key;
1788     struct lock_class_key     s_vfs_rename_key;
1789     struct lock_class_key     s_writers_key[SB_FREEZE_LEVELS];
1790 
1791     struct lock_class_key     i_lock_key;
1792     struct lock_class_key     i_mutex_key;
1793     struct lock_class_key     i_mutex_dir_key;
1794 };
```

## register_filesystem()
功能是将某个 file_system_type 对象添加到全局的 file_systems 对象链表中。如果 file_systems 对象链表中有同名字的对象，则返回错误的值，否则返回 0。
```
/// @file fs/filesystems.c
46 static struct file_system_type **find_filesystem(const char *name, unsigned len)
47 {   
48     struct file_system_type **p;
49     for (p=&file_systems; *p; p=&(*p)->next) // 牛逼NB
50         if (strlen((*p)->name) == len &&
51             strncmp((*p)->name, name, len) == 0)
52             break;
53     return p;
54 }

69 int register_filesystem(struct file_system_type * fs)
70 {
71     int res = 0;
72     struct file_system_type ** p;
73 
74     BUG_ON(strchr(fs->name, '.'));
75     if (fs->next)
76         return -EBUSY;
77     write_lock(&file_systems_lock);
78     p = find_filesystem(fs->name, strlen(fs->name));
79     if (*p)
80         res = -EBUSY;
81     else
82         *p = fs;
83     write_unlock(&file_systems_lock);
84     return res;
85 }
```

## unregister_filesystem()
```
101 int unregister_filesystem(struct file_system_type * fs)
102 {
103     struct file_system_type ** tmp;
104 
105     write_lock(&file_systems_lock);
106     tmp = &file_systems;
107     while (*tmp) {
108         if (fs == *tmp) {
109             *tmp = fs->next;
110             fs->next = NULL;
111             write_unlock(&file_systems_lock);
112             synchronize_rcu();
113             return 0;
114         }
115         tmp = &(*tmp)->next;
116     }
117     write_unlock(&file_systems_lock);
118 
119     return -EINVAL;
120 }
```