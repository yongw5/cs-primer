#### upd_sock
```
/// @file include/linux/udp.h
42 struct udp_sock {
43     /* inet_sock has to be the first member */
44     struct inet_sock inet;
45 #define udp_port_hash       inet.sk.__sk_common.skc_u16hashes[0]
46 #define udp_portaddr_hash   inet.sk.__sk_common.skc_u16hashes[1]
47 #define udp_portaddr_node   inet.sk.__sk_common.skc_portaddr_node
48     int      pending;   /* Any pending frames ? */
49     unsigned int     corkflag;  /* Cork is required */
50     __u8         encap_type;    /* Is this an Encapsulation socket? */
51     unsigned char    no_check6_tx:1,/* Send zero UDP6 checksums on TX? */
52              no_check6_rx:1;/* Allow zero UDP6 checksums on RX? */
53     /*
54      * Following member retains the information to create a UDP header
55      * when the socket is uncorked.
56      */
57     __u16        len;       /* total length of pending frames */
58     /*
59      * Fields specific to UDP-Lite.
60      */
61     __u16        pcslen;
62     __u16        pcrlen;
63 /* indicator bits used by pcflag: */
64 #define UDPLITE_BIT      0x1        /* set by udplite proto init function */
65 #define UDPLITE_SEND_CC  0x2        /* set via udplite setsockopt         */
66 #define UDPLITE_RECV_CC  0x4        /* set via udplite setsocktopt        */
67     __u8         pcflag;        /* marks socket as UDP-Lite if > 0    */
68     __u8         unused[3];
69     /*
70      * For encapsulation sockets.
71      */
72     int (*encap_rcv)(struct sock *sk, struct sk_buff *skb);
73     void (*encap_destroy)(struct sock *sk);
74 };
```