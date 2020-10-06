## listen_sock
```
/// @file include/net/request_sock.h
 95 struct listen_sock {
 96     u8          max_qlen_log;
 97     u8          synflood_warned;
 98     /* 2 bytes hole, try to use */
 99     int         qlen;
100     int         qlen_young;
101     int         clock_hand;
102     u32         hash_rnd;
103     u32         nr_table_entries;
104     struct request_sock *syn_table[0];
105 };
```