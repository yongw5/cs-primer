## request_sock
```
/// @file include/net/request_sock.h
48 /* struct request_sock - mini sock to represent a connection request
49  */
50 struct request_sock {
51     struct sock_common      __req_common;
52     struct request_sock     *dl_next;
53     u16             mss;
54     u8              num_retrans; /* number of retransmits */
55     u8              cookie_ts:1; /* syncookie: encode tcpopts in timestamp */
56     u8              num_timeout:7; /* number of timeouts */
57     /* The following two fields can be easily recomputed I think -AK */
58     u32             window_clamp; /* window clamp at creation time */
59     u32             rcv_wnd;      /* rcv_wnd offered first time */
60     u32             ts_recent;
61     unsigned long           expires;
62     const struct request_sock_ops   *rsk_ops;
63     struct sock         *sk;
64     u32             secid;
65     u32             peer_secid;
66 };
```