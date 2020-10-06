## inet_timewait_sock
```
/// @file include/net/inet_timewait_sock.h
 97 /*
 98  * This is a TIME_WAIT sock. It works around the memory consumption
 99  * problems of sockets in such a state on heavily loaded servers, but
100  * without violating the protocol specification.
101  */
102 struct inet_timewait_sock {
103     /*
104      * Now struct sock also uses sock_common, so please just
105      * don't add nothing before this first member (__tw_common) --acme
106      */
107     struct sock_common  __tw_common;
108 #define tw_family       __tw_common.skc_family
109 #define tw_state        __tw_common.skc_state
110 #define tw_reuse        __tw_common.skc_reuse
111 #define tw_reuseport        __tw_common.skc_reuseport
112 #define tw_bound_dev_if     __tw_common.skc_bound_dev_if
113 #define tw_node         __tw_common.skc_nulls_node
114 #define tw_bind_node        __tw_common.skc_bind_node
115 #define tw_refcnt       __tw_common.skc_refcnt
116 #define tw_hash         __tw_common.skc_hash
117 #define tw_prot         __tw_common.skc_prot
118 #define tw_net          __tw_common.skc_net
119 #define tw_daddr            __tw_common.skc_daddr
120 #define tw_v6_daddr     __tw_common.skc_v6_daddr
121 #define tw_rcv_saddr        __tw_common.skc_rcv_saddr
122 #define tw_v6_rcv_saddr     __tw_common.skc_v6_rcv_saddr
123 #define tw_dport        __tw_common.skc_dport
124 #define tw_num          __tw_common.skc_num
125 
126     int         tw_timeout;
127     volatile unsigned char  tw_substate;
128     unsigned char       tw_rcv_wscale;
129 
130     /* Socket demultiplex comparisons on incoming packets. */
131     /* these three are in inet_sock */
132     __be16          tw_sport;
133     kmemcheck_bitfield_begin(flags);
134     /* And these are ours. */
135     unsigned int        tw_ipv6only     : 1,
136                 tw_transparent  : 1,
137                 tw_flowlabel    : 20,
138                 tw_pad      : 2,    /* 2 bits hole */
139                 tw_tos      : 8;
140     kmemcheck_bitfield_end(flags);
141     u32         tw_ttd;
142     struct inet_bind_bucket *tw_tb;
143     struct hlist_node   tw_death_node;
144 };
```

## tcp_timewait_sock
```
/// @file include/linux/tcp.h
352 struct tcp_timewait_sock {
353     struct inet_timewait_sock tw_sk;
354     u32           tw_rcv_nxt;
355     u32           tw_snd_nxt;
356     u32           tw_rcv_wnd;
357     u32           tw_ts_offset;
358     u32           tw_ts_recent;
359     long              tw_ts_recent_stamp;
360 #ifdef CONFIG_TCP_MD5SIG
361     struct tcp_md5sig_key     *tw_md5_key;
362 #endif
363 };
```