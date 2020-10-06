## inet_request_sock
```
/// @file include/net/inet_sock.h
71 struct inet_request_sock {
72     struct request_sock req;
73 #define ir_loc_addr     req.__req_common.skc_rcv_saddr
74 #define ir_rmt_addr     req.__req_common.skc_daddr
75 #define ir_num          req.__req_common.skc_num
76 #define ir_rmt_port     req.__req_common.skc_dport
77 #define ir_v6_rmt_addr      req.__req_common.skc_v6_daddr
78 #define ir_v6_loc_addr      req.__req_common.skc_v6_rcv_saddr
79 #define ir_iif          req.__req_common.skc_bound_dev_if
80 
81     kmemcheck_bitfield_begin(flags);
82     u16         snd_wscale : 4,
83                 rcv_wscale : 4,
84                 tstamp_ok  : 1,
85                 sack_ok    : 1,
86                 wscale_ok  : 1,
87                 ecn_ok     : 1,
88                 acked      : 1,
89                 no_srccheck: 1;
90     kmemcheck_bitfield_end(flags);
91     struct ip_options_rcu   *opt;
92     struct sk_buff      *pktopts;
93     u32                     ir_mark;
94 };
```

## tcp_resuqst_sock
```
/// @file include/linux/tcp.h
117 struct tcp_request_sock {
118     struct inet_request_sock    req;
119 #ifdef CONFIG_TCP_MD5SIG
120     /* Only used by TCP MD5 Signature so far. */
121     const struct tcp_request_sock_ops *af_specific;
122 #endif
123     struct sock         *listener; /* needed for TFO */
124     u32             rcv_isn;
125     u32             snt_isn;
126     u32             snt_synack; /* synack sent time */
127     u32             rcv_nxt; /* the ack # by SYNACK. For
128                           * FastOpen it's the seq#
129                           * after data-in-SYN.
130                           */
131 };
```