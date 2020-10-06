## SLAB 分配器
用 `cat /proc/slabinfo` 可以查看系统创建的 SLAB 分配器。和套接字有关的有如下几个（不考虑 IPv6）
- sock_inode_cache：分配 socket_alloc 对象，由全局指针变量 sock_inode_cachep 指向
- TCP：分配 tcp_sock 对象，由全局变量 tcp_prot.slab 指向
- request_sock_TCP：分配 tcp_request_sock 对象，由全局变量 tcp_prot.rsk_prot->slab 指向
- tw_sock_TCP：分配 tcp_timewait_sock 对象，由全局变量 tcp_prot.twsk_prot->slab 指向
- UDP：分配 struct udp_sock 对象，由全局变量 udp_prot.slab 指向（在 /sys/kernel/slab 可以找到）
- RAW：分配 struct raw_sock 对象，由全局变量 raw_prot.slab 指向

用于分配 tcp_sock、tcp_request_sock 和 tcp_timewait_sock 对象的 SLAB 分配器在 inet_init::proto_register(&tcp_prot, 1) 中完成（net/ipv4/af_inet.c:1703）。proto_register() 主要完成上面分析的 SLAB 分配器的创建
1. 创建分配 tcp_sock 的 SLAB，名字为 TCP ，用 tcp_prot.slab 指向。
```
/// @file net/core/sock.c
/// prot 指向 tcp_prot 全局变量
2797 int proto_register(struct proto *prot, int alloc_slab)
2798 {
2799     if (alloc_slab) {
2800         prot->slab = kmem_cache_create(prot->name, prot->obj_size, 0,
2801                     SLAB_HWCACHE_ALIGN | prot->slab_flags,
2802                     NULL);
```
1. 创建分配 tcp_request_sock 的 SLAB，名字为 request_sock_TCP ，用 tcp_prot.rsk_prot->slab 指向
```
/// @file net/core/sock.c
2810         if (prot->rsk_prot != NULL) {
2811             prot->rsk_prot->slab_name = kasprintf(GFP_KERNEL, "request_sock_%s", prot->name);
2812             if (prot->rsk_prot->slab_name == NULL)
2813                 goto out_free_sock_slab;
2814 
2815             prot->rsk_prot->slab = kmem_cache_create(prot->rsk_prot->slab_name,
2816                                  prot->rsk_prot->obj_size, 0,
2817                                  SLAB_HWCACHE_ALIGN, NULL);
```

1. 创建分配 tcp_timewait_sock 的 SLAB，名字为 tw_sock_TCP，用 tcp_prot.twsk_prot->slab 指向
```
/// @file net/core/sock.c
2826         if (prot->twsk_prot != NULL) {
2827             prot->twsk_prot->twsk_slab_name = kasprintf(GFP_KERNEL, "tw_sock_%s", prot->name);
2828 
2829             if (prot->twsk_prot->twsk_slab_name == NULL)
2830                 goto out_free_request_sock_slab;
2831 
2832             prot->twsk_prot->twsk_slab =
2833                 kmem_cache_create(prot->twsk_prot->twsk_slab_name,
2834                           prot->twsk_prot->twsk_obj_size,
2835                           0,
2836                           SLAB_HWCACHE_ALIGN |
2837                             prot->slab_flags,
2838                           NULL);
```

## net_families 数组
最初的赋值操作在函数 inet_init::sock_register(&inet_family_ops) 中完成（net/ipv4/af_inet.c:1703）。inet_family_ops 是个全局变量，定义如下
```
/// net/ipv4/af_inet.c 991
991 static const struct net_proto_family inet_family_ops = {
992     .family = PF_INET,
993     .create = inet_create,
994     .owner  = THIS_MODULE,
995 };
```
sock_register() 主要的工作就是将 PF_INET 协议族注册进 net_families 数组
```
/// @file net/socket.c
2606 int sock_register(const struct net_proto_family *ops)
2607 {
2608     int err;
2609 
2610     if (ops->family >= NPROTO) {
2611         pr_crit("protocol %d >= NPROTO(%d)\n", ops->family, NPROTO);
2612         return -ENOBUFS;
2613     }
2614 
2615     spin_lock(&net_family_lock);
2616     if (rcu_dereference_protected(net_families[ops->family],
2617                       lockdep_is_held(&net_family_lock)))
2618         err = -EEXIST;
2619     else {
2620         rcu_assign_pointer(net_families[ops->family], ops); // 赋值
2621         err = 0;
2622     }
2623     spin_unlock(&net_family_lock);
2624 
2625     pr_info("NET: Registered protocol family %d\n", ops->family);
2626     return err;
2627 }
```
结果就是 net_families[PF_INET]->create 函数指针指向的是 inet_create() 函数

## inet_protos 数组
函数 inet_init::inet_add_protocol() 将协议 tcp_protocol、udp_protocol、icmp_protocol 和 igmp_protocol 写进全局变量 inet_protos。结果就是
```
inet_protos[
  IPPROTO_ICMP, &icmp_protocol,
  IPPROTO_UDP, &udp_protocol,
  IPPROTO_TCP, &tcp_protocol,
  IPPROTO_IGMP, &igmp_protocol
]
```

##  inetsw 链表
inetsw 存放的是套接字接口 inet_protosw 对象的指针，初始化时将数组 inetsw_array 的内容赋值给 inetsw 。全局变量 inetsw_array 的定义如下：
```
/// @file net/ipv4/af_inet.c
1000 static struct inet_protosw inetsw_array[] =
1001 {
1002     {
1003         .type =       SOCK_STREAM,
1004         .protocol =   IPPROTO_TCP,
1005         .prot =       &tcp_prot,
1006         .ops =        &inet_stream_ops,
1007         .flags =      INET_PROTOSW_PERMANENT |
1008                   INET_PROTOSW_ICSK,
1009     },
1010 
1011     {
1012         .type =       SOCK_DGRAM,
1013         .protocol =   IPPROTO_UDP,
1014         .prot =       &udp_prot,
1015         .ops =        &inet_dgram_ops,
1016         .flags =      INET_PROTOSW_PERMANENT,
1017        },
1018 
1019        {
1020         .type =       SOCK_DGRAM,
1021         .protocol =   IPPROTO_ICMP,
1022         .prot =       &ping_prot,
1023         .ops =        &inet_sockraw_ops,
1024         .flags =      INET_PROTOSW_REUSE,
1025        },
1026 
1027        {
1028            .type =       SOCK_RAW,
1029            .protocol =   IPPROTO_IP,    /* wild card */
1030            .prot =       &raw_prot,
1031            .ops =        &inet_sockraw_ops,
1032            .flags =      INET_PROTOSW_REUSE,
1033        }
1034 };
```