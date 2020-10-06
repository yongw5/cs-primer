## sock
比较通用的网络层描述块，构成传输控制块的基础，与具体的协议无关。添加了接收队列和发送队列，接受队列和发送队列的大小。以及状态变化、可读、可写等的回调
```
/// @file include/net/sock.h
219 /**
220   * struct sock - network layer representation of sockets
295  */
296 struct sock { 
297     /*
298      * Now struct inet_timewait_sock also uses sock_common, so please just
299      * don't add nothing before this first member (__sk_common) --acme
300      */
301     struct sock_common      __sk_common;
302 #define sk_node             __sk_common.skc_node // 重命名
303 #define sk_nulls_node       __sk_common.skc_nulls_node
304 #define sk_refcnt           __sk_common.skc_refcnt
305 #define sk_tx_queue_mapping __sk_common.skc_tx_queue_mapping
306 
307 #define sk_dontcopy_begin   __sk_common.skc_dontcopy_begin
308 #define sk_dontcopy_end     __sk_common.skc_dontcopy_end
309 #define sk_hash             __sk_common.skc_hash
310 #define sk_portpair         __sk_common.skc_portpair
311 #define sk_num              __sk_common.skc_num
312 #define sk_dport            __sk_common.skc_dport
313 #define sk_addrpair         __sk_common.skc_addrpair
314 #define sk_daddr            __sk_common.skc_daddr
315 #define sk_rcv_saddr        __sk_common.skc_rcv_saddr
316 #define sk_family           __sk_common.skc_family
317 #define sk_state            __sk_common.skc_state
318 #define sk_reuse            __sk_common.skc_reuse
319 #define sk_reuseport        __sk_common.skc_reuseport
320 #define sk_bound_dev_if     __sk_common.skc_bound_dev_if
321 #define sk_bind_node        __sk_common.skc_bind_node
322 #define sk_prot             __sk_common.skc_prot
323 #define sk_net              __sk_common.skc_net
324 #define sk_v6_daddr         __sk_common.skc_v6_daddr
325 #define sk_v6_rcv_saddr     __sk_common.skc_v6_rcv_saddr
326 
327     socket_lock_t             sk_lock;            // 用于同步
328     struct sk_buff_head       sk_receive_queue;   // 接收队列
329     /*
330      * The backlog queue is special, it is always used with
331      * the per-socket spinlock held and requires low latency
332      * access. Therefore we special case it's implementation.
333      * Note : rmem_alloc is in this structure to fill a hole
334      * on 64bit arches, not because its logically part of
335      * backlog.
336      */
337     struct {
338         atomic_t              rmem_alloc;
339         int                   len;
340         struct sk_buff       *head;
341         struct sk_buff       *tail;
342     } sk_backlog; // 后备队列
343 #define sk_rmem_alloc sk_backlog.rmem_alloc
344     int                       sk_forward_alloc;
345 #ifdef CONFIG_RPS
346     __u32                     sk_rxhash;
347 #endif
348 #ifdef CONFIG_NET_RX_BUSY_POLL
349     unsigned int              sk_napi_id;
350     unsigned int              sk_ll_usec;
351 #endif
352     atomic_t                  sk_drops;
353     int                       sk_rcvbuf;          // 接收缓冲区大小
354 
355     struct sk_filter __rcu   *sk_filter;
356     struct socket_wq __rcu   *sk_wq;
357 
358 #ifdef CONFIG_NET_DMA
359     struct sk_buff_head       sk_async_wait_queue; // DMA
360 #endif
361 
362 #ifdef CONFIG_XFRM
363     struct xfrm_policy       *sk_policy[2];
364 #endif
365     unsigned long             sk_flags;           // SO_LINGER, SO_BROADCAST, SO_KEEPALIVE, 
                                                      // SO_OOBINLINE, SO_TIMESTAMPING
366     struct dst_entry         *sk_rx_dst;
367     struct dst_entry __rcu   *sk_dst_cache;       // 目的路由缓存
368     spinlock_t                sk_dst_lock;        // 目的路由缓存操作锁
369     atomic_t                  sk_wmem_alloc;      // 为发送而分配SKB的总大小
370     atomic_t                  sk_omem_alloc;      // 分配辅助缓冲区的总大小
371     int                       sk_sndbuf;          // 发送缓冲区大小
372     struct sk_buff_head       sk_write_queue;     // 发送队列
373     kmemcheck_bitfield_begin(flags);
374     unsigned int              sk_shutdown  : 2,   // RCV_SHUTDOWN
                                                      // SEND_SHUTDOWN
                                                      // SHUTDOWN_MASK
375                               sk_no_check_tx : 1, // SO_NO_CHECK设置
376                               sk_no_check_rx : 1,
377                               sk_userlocks : 4,   // SOCK_SNDBUF_LOCK：设置了发送缓冲区大小
                                                      // SOCK_RCVBUF_LOCK：设置了接受缓冲区大小
                                                      // SOCK_BINDADDR_LOCK：绑定了本地端口
                                                      // SOCK_BINDPORT_LOCK：绑定了本地地址
378                               sk_protocol  : 8,   // 所属协议
379                               sk_type      : 16;  // 套接字类型
380 #define SK_PROTOCOL_MAX U8_MAX
381     kmemcheck_bitfield_end(flags);
382     int                       sk_wmem_queued;     // 发送队列中数据总长度
383     gfp_t                     sk_allocation;
384     u32                       sk_pacing_rate;
385     u32                       sk_max_pacing_rate;
386     netdev_features_t         sk_route_caps;
387     netdev_features_t         sk_route_nocaps;
388     int                       sk_gso_type;
389     unsigned int              sk_gso_max_size;
390     u16                       sk_gso_max_segs;
391     int                       sk_rcvlowat;        // 接收低水位 SO_RCVLOWAT 设置
392     unsigned long             sk_lingertime;      // SO_LINGER 设置
393     struct sk_buff_head       sk_error_queue;     // 错误队列
394     struct proto             *sk_prot_creator;
395     rwlock_t                  sk_callback_lock;
396     int                       sk_err,
397                               sk_err_soft;
398     unsigned short            sk_ack_backlog;     // 当前已建立的连接数
399     unsigned short            sk_max_ack_backlog; // 已建立连接数的上限
400     __u32                     sk_priority;        // SO_PRIORITY 设置
401 #if IS_ENABLED(CONFIG_CGROUP_NET_PRIO)
402     __u32                     sk_cgrp_prioidx;
403 #endif
404     struct pid               *sk_peer_pid;
405     const struct cred        *sk_peer_cred;
406     long                      sk_rcvtimeo;        // 接收超时，SO_RCVTIMEO 设置
407     long                      sk_sndtimeo;        // 发送超时，SO_SNDTIMEO 设置
408     void                     *sk_protinfo;        // 存放私有数据
409     struct timer_list         sk_timer;           // 计时器链表
410     ktime_t                   sk_stamp;
411     struct socket            *sk_socket;
412     void                     *sk_user_data;
413     struct page_frag          sk_frag;
414     struct sk_buff           *sk_send_head;      // 发送队列
415     __s32                     sk_peek_off;       // 当前 peek_offset 值
416     int                       sk_write_pending;  // 标识有数据即将写入套接字
417 #ifdef CONFIG_SECURITY
418     void                     *sk_security;
419 #endif
420     __u32                    sk_mark;
421     u32                      sk_classid;
422     struct cg_proto          *sk_cgrp;
423     void            (*sk_state_change)(struct sock *sk); // 状态变化回调（唤醒等待进程）
424     void            (*sk_data_ready)(struct sock *sk);   // 可读回调
425     void            (*sk_write_space)(struct sock *sk);  // 可写回调
426     void            (*sk_error_report)(struct sock *sk); // 出错回调
427     int             (*sk_backlog_rcv)(struct sock *sk,
428                           struct sk_buff *skb);          // 后备队列处理回调
429     void            (*sk_destruct)(struct sock *sk);     // 析构函数回调，释放 sock 时调用
430 };
```

## inet_sock
比较通用的 IPv4 协议簇描述块，包含 IPv4 协议簇基础传输层，即 UDP、TCP 以及 RAW 控制块共有的信息。添加了 IPv4 协议专用的属性，比如 TTL
```
/// @file include/net/inet_sock.h
132 /** struct inet_sock - representation of INET sockets
151  */
152 struct inet_sock {
153     /* sk and pinet6 has to be the first two members of inet_sock */
154     struct sock                  sk;
155 #if IS_ENABLED(CONFIG_IPV6)
156     struct ipv6_pinfo           *pinet6;
157 #endif
158     /* Socket demultiplex comparisons on incoming packets. */
159 #define inet_daddr               sk.__sk_common.skc_daddr     // 绑定的目的 IP 地址
160 #define inet_rcv_saddr           sk.__sk_common.skc_rcv_saddr // 绑定的本地 IP 地址
161 #define inet_dport               sk.__sk_common.skc_dport     // 目的端口号
162 #define inet_num                 sk.__sk_common.skc_num       // 本地端口号
163 
164     __be32                       inet_saddr;           // 发送的实际本地 IP 地址
165     __s16                        uc_ttl;               // 单播 TTL，默认-1        
166     __u16                        cmsg_flags;
167     __be16                       inet_sport;           // 发送的实际本地端口号
168     __u16                        inet_id;              // IP 首部标识 ID
169 
170     struct ip_options_rcu __rcu *inet_opt;             // IP 首部的选项指针
171     int                          rx_dst_ifindex;
172     __u8                         tos;                  // IP 首部 TOS（区分服务）
173     __u8                         min_ttl;
174     __u8                         mc_ttl;               // 多播 TTL
175     __u8                         pmtudisc;
176     __u8                         recverr:1,            // IP_RECVERR 设置
177                                  is_icsk:1,            // 是否是 inet_connection_sock
178                                  freebind:1,           // IP_FREEBIND设 置
179                                  hdrincl:1,
180                                  mc_loop:1,            // 组播是否发向回路
181                                  transparent:1,
182                                  mc_all:1,
183                                  nodefrag:1;
184     __u8                         rcv_tos;
185     int                          uc_index;             // 单播的网络设备索引号，为0表示任意
186     int                          mc_index;             // 多播的网络设备索引号，为0表示任意
187     __be32                       mc_addr;              // 多播源地址
188     struct ip_mc_socklist __rcu *mc_list;              // 加入的组播组
189     struct inet_cork_full        cork;
190 };
191 
```