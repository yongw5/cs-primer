## 简介
自旋锁与互斥锁优点类似，只是自旋锁不会引起调用者睡眠。自旋锁是使用忙等待来确保互斥锁的一种特殊的方法。如果自旋锁可用，则获得锁，执行临界区代码，然后释放锁。如果自旋锁已经被别的执行单元保持，不可使用，调用者就一直循环（忙等待）在那里看是否该自旋锁的保持者已经释放了锁，直到锁可用为止，"自旋"一词就是因此而得名。忙等待虽然看起来效率低下，但是自旋锁使用者一般保持锁时间非常短，因此选择自旋而不是睡眠是非常必要的。实际上忙等待比将线程休眠然后当锁可用时将其唤醒要快得多。

## 接口
创建和初始化自旋锁
```
spinlock_t my_spinlock = __SPIN_LOCK_UNLOCKED;
/// or
DEFINE_SPINLOCK(my_spinlock);
/// or
spinlock_t my_spinlock;
spin_lock_init(&my_spinlock);
```
自旋锁的获取 spin_lock() 和释放 spin_unlock()，不会执行中断禁用。spin_trylock() 尽力获得自旋锁，如果能立即获得锁，它获得锁并返回真，否则不能立即获得锁，立即返回假。
```
spin_lock(&my_spinlock);
/// critical section
spin_unlock(&my_spinlock);
```
spin_lock_irqsave() 获取自旋锁，并且在本地处理器上禁止中断。spin_unlock_irqrestore() 释放自旋锁，并且恢复（通过 flags 参数）中断
```
spin_lock_irqsave(&my_spinlock, flags);
/// critical section
spin_unlock_irqrestore(&my_spinlock, flags);
```
如果内核线程通过下半部（bottom-half）方式共享数据，那么可以使用自旋锁的另一个变体。下半部方法可以将设备驱动程序中的工作延迟到中断处理后执行。此时自旋锁禁止了本地 CPU 上的软中断。这可以阻止 softirq、tasklet 和 bottom half 在本地 CPU 上运行
```
spin_lock_bh(&my_spinlock);
/// critical section
spin_unlock_bh(&my_spinlock);
```

## 实现
### spinlock_t
spinlock_t 的定义如下
```
/// @file include/linux/spinlock_types.h
64 typedef struct spinlock {
65     union {
66         struct raw_spinlock rlock;
67 
68 #ifdef CONFIG_DEBUG_LOCK_ALLOC
69 # define LOCK_PADSIZE (offsetof(struct raw_spinlock, dep_map))
70         struct {
71             u8 __padding[LOCK_PADSIZE];
72             struct lockdep_map dep_map;
73         };
74 #endif
75     };
76 } spinlock_t;
```
如果忽略 CONFIG_DEBUG_LOCK_ALLOC，spinlock_t 只包含一个 raw_spinlock 类型的变量 rlock
```
/// @file include/linux/spinlock_types.h
20 typedef struct raw_spinlock {
21     arch_spinlock_t raw_lock;
22 #ifdef CONFIG_GENERIC_LOCKBREAK
23     unsigned int break_lock;
24 #endif
25 #ifdef CONFIG_DEBUG_SPINLOCK
26     unsigned int magic, owner_cpu;
27     void *owner;
28 #endif
29 #ifdef CONFIG_DEBUG_LOCK_ALLOC
30     struct lockdep_map dep_map;
31 #endif
32 } raw_spinlock_t;
```
同样的 raw_spinlock 也主要包含一个 arch_spinlock_t 类型的成员 raw_lock。arch_spinlock_t 的实现跟体系结构有关系。在 x86 体系结构下，其定义如下：
```
/// @file arch/x86/include/asm/spinlock_types.h
14 #if (CONFIG_NR_CPUS < (256 / __TICKET_LOCK_INC))
15 typedef u8  __ticket_t;
16 typedef u16 __ticketpair_t;
17 #else
18 typedef u16 __ticket_t;
19 typedef u32 __ticketpair_t;
20 #endif

26 typedef struct arch_spinlock {
27     union {
28         __ticketpair_t head_tail;
29         struct __raw_tickets {
30             __ticket_t head, tail;
31         } tickets;
32     };
33 } arch_spinlock_t;
```

### spin_lock_init()
```
/// @file include/linux/spinlock.h
290 static inline raw_spinlock_t *spinlock_check(spinlock_t *lock)
291 {   
292     return &lock->rlock;
293 }
294
295 #define spin_lock_init(_lock)               \
296 do {                                        \
297     spinlock_check(_lock);                  \
298     raw_spin_lock_init(&(_lock)->rlock);    \
299 } while (0)
```
初始化工作在 raw_spin_lock_init() 中完成，其定义如下
```
/// @file include/linux/spinlock.h
103 # define raw_spin_lock_init(lock)               \
104     do { *(lock) = __RAW_SPIN_LOCK_UNLOCKED(lock); } while (0)
```
宏定义 __RAW_SPIN_LOCK_UNLOCKED() 主要的工作是清空 raw_lock 成员
```
/// @file include/linux/spinlock_types.h
53 #define __RAW_SPIN_LOCK_INITIALIZER(lockname)   \
54     {                   \
55     .raw_lock = __ARCH_SPIN_LOCK_UNLOCKED,  \
56     SPIN_DEBUG_INIT(lockname)       \
57     SPIN_DEP_MAP_INIT(lockname) }
58 
59 #define __RAW_SPIN_LOCK_UNLOCKED(lockname)  \
60     (raw_spinlock_t) __RAW_SPIN_LOCK_INITIALIZER(lockname)
```
__ARCH_SPIN_LOCK_UNLOCKED 内容为空
```
/// @file include/linux/spinlock_types_up.h
27 #define __ARCH_SPIN_LOCK_UNLOCKED { }
```
初始化那刻，head 和 tail 的值都为 0，表示锁空闲。

### spin_lock()
```
/// @file include/linux/spinlock.h
301 static inline void spin_lock(spinlock_t *lock)
302 {
303     raw_spin_lock(&lock->rlock);
304 }
```
宏定义 raw_spin_lock() 定义如下
```
/// @file include/linux/spinlock.h
188 #define raw_spin_lock(lock) _raw_spin_lock(lock)
```
根据 CPU 体系结构，spinlock 分为 SMP 版本和 UP 版本。UP 版本没有意义，这里分析 SMP 版本。宏定义 _raw_spin_lock() 定义如下
```
/// @file include/linux/spinlock_api_smp.h
47 #define _raw_spin_lock(lock) __raw_spin_lock(lock)
```
__raw_spin_lock() 的实现如下：
```
/// @file include/linux/spinlock_api_smp.h
139 static inline void __raw_spin_lock(raw_spinlock_t *lock)
140 {
141     preempt_disable(); // 禁止抢占
142     spin_acquire(&lock->dep_map, 0, 0, _RET_IP_); // DEBUG 使用，正常没有做任何事
143     LOCK_CONTENDED(lock, do_raw_spin_trylock, do_raw_spin_lock); // 加锁
144 }
```
宏定义 LOCK_CONTENDED 会根据 CONFIG_LOCK_STAT 执行不用的逻辑，但是加锁都是调用传入的 do_raw_spin_lock 函数，在没有设置 CONFIG_DEBUG_SPINLOCK 的系统中，其定义如下
```
/// @file include/linux/spinlock.h
155 static inline void do_raw_spin_lock(raw_spinlock_t *lock) __acquires(lock)
156 {
157     __acquire(lock);
158     arch_spin_lock(&lock->raw_lock);
159 }
```
在 x86 体系结构下，arch_spin_lock() 的定义如下：
```
/// @file arch/x86/include/asm/spinlock.h
 82 static __always_inline void arch_spin_lock(arch_spinlock_t *lock)
 83 {   // inc.tail 的值为 1
 84     register struct __raw_tickets inc = { .tail = TICKET_LOCK_INC };
 85     // 原子地 inc.tail += lock->tichkets.tail，xadd 返回运算前的结果
 86     inc = xadd(&lock->tickets, inc); 
 87     if (likely(inc.head == inc.tail))
 88         goto out;
 89 
 90     inc.tail &= ~TICKET_SLOWPATH_FLAG;
 91     for (;;) { // 循环直到 head 和 tail 相等
 92         unsigned count = SPIN_THRESHOLD; // 值为 1 << 15
 93 
 94         do {
 95             if (ACCESS_ONCE(lock->tickets.head) == inc.tail)
 96                 goto out; // 获取锁，跳出循环
 97             cpu_relax();
 98         } while (--count);
 99         __ticket_lock_spinning(lock, inc.tail);
100     }
101 out:    barrier();  /* make sure nothing creeps before the lock is taken */
102 }
```
__raw_tickets 有两个成员 head 和 tail，head 指示队列的当前头部，而 tail 指示队列的当前尾部。初始化那刻，head 和 tail 的值都为 0，表示锁空闲。arch_spin_lock() 获取锁的过程是：
1. 申请一个 __raw_tickets 类型的变量 inc，其 tail 的值为 1，head 的值为 0;
2. 原子地完成 xadd()，xadd() 的工作有三部分，（1）保存 lock->tickets 的值；（2）lock->tickets.tail += inc.tail；（3）返回保存的 lock->tickets 旧值。xadd() 可以确保三个部分的整体是原子操作
3. 然后忙等直到 lock->tichets.head == inc.tail 表示成功地获取到自旋锁 

### spin_unlock()
```
/// @file include/linux/spinlock.h
341 static inline void spin_unlock(spinlock_t *lock)
342 {
343     raw_spin_unlock(&lock->rlock);
344 }
```
和 spin_lock() 一样的逻辑，x86 系统 SMP 模式下，最终调用的函数是 arch_spin_unlock()。其定义如下：
```
/// @file arch/x86/include/asm/spinlock.h
146 static __always_inline void arch_spin_unlock(arch_spinlock_t *lock)
147 {
148     if (TICKET_SLOWPATH_FLAG &&
149         static_key_false(&paravirt_ticketlocks_enabled)) { // 假
150         arch_spinlock_t prev;
151 
152         prev = *lock;
153         add_smp(&lock->tickets.head, TICKET_LOCK_INC);
154 
155         /* add_smp() is a full mb() */
156 
157         if (unlikely(lock->tickets.tail & TICKET_SLOWPATH_FLAG))
158             __ticket_unlock_slowpath(lock, prev);
159     } else
160         __add(&lock->tickets.head, TICKET_LOCK_INC, UNLOCK_LOCK_PREFIX);
161 }
```
arch_spin_unlock() 的工作很简单，原子地执行 lock->tickets.head += 1

### spin_trylock()
x86 系统 SMP 下 spin_trylock() 最终调用 arch_spin_trylock() 函数
```
/// @file arch/x86/include/asm/spinlock.h
104 static __always_inline int arch_spin_trylock(arch_spinlock_t *lock)
105 {   
106     arch_spinlock_t old, new;
107     
108     old.tickets = ACCESS_ONCE(lock->tickets); 
109     if (old.tickets.head != (old.tickets.tail & ~TICKET_SLOWPATH_FLAG))
110         return 0;
111     // tail 递增 1，获取锁
112     new.head_tail = old.head_tail + (TICKET_LOCK_INC << TICKET_SHIFT);
113     
114     /* cmpxchg is a full barrier, so nothing can move before it */ 
115     return cmpxchg(&lock->head_tail, old.head_tail, new.head_tail) == old.head_tail;
116 }
```
arch_spin_trylock() 的逻辑如下
- 如果当前 lock->tickets 的 head != tail，表示锁被占用（空闲时 head == tail）不能获取锁，返回 0
- 否则，尝试原子性地更新 lock->head_tail（tail 递增 1，head_tail 高 8 位或 16 保存的是 tail）
  - 如果更新前的旧值等于原来的值（old.head_tail）就表示这个期间没有其他对象操作本自旋锁，表示加锁成功。
  - 否则，这个期间有其他操作本自旋锁，不能成功加锁

## 总结
自旋锁用类似“队列”的结构完成互斥控制。head 指示队列的当前头部，而 tail 指示队列的当前尾部。初始化那刻，head 和 tail 的值都为 0，head == tail 表示锁空闲。加锁的过程就是换取“队列”现在的尾部 tial（记为 tail_），指示自己在“队列”的位置，然后队列将 tail 加 1。然后忙等待条件 head == tail_ 成立，表示自己可以获得锁。释放锁就是将 head 递增 1。