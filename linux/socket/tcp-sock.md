## inet_connection_sock
面向连接特性的描述块，在 inet_sock 的基础上增加了连接、确认和重传等成员
```
/// @file include/net/inet_connection_sock.h 
 68 /** inet_connection_sock - INET connection oriented sock
 87  */
 88 struct inet_connection_sock {
 89     /* inet_sock has to be the first member! */
 90     struct inet_sock           icsk_inet;
91     struct request_sock_queue   icsk_accept_queue; // 存放已建立的连接的队列
 92     struct inet_bind_bucket   *icsk_bind_hash;    // 执行绑定的本地端口信息
 93     unsigned long              icsk_timeout;      // 超时重传时间
 94     struct timer_list          icsk_retransmit_timer; // 超时重传计时器
 95     struct timer_list          icsk_delack_timer; // 延迟发送 ACK 计时器
 96     __u32                      icsk_rto;          // 超时重传时间，处置为 TCP_TIMEOUT_INIT
 97     __u32                      icsk_pmtu_cookie;  // 最后一次更新的最大传输单元（MTU）
 98     const struct tcp_congestion_ops          *icsk_ca_ops; // 指向某个拥塞控制算法
 99     const struct inet_connection_sock_af_ops *icsk_af_ops; // TCP 的一个操作接口集
100     unsigned int          (*icsk_sync_mss)(struct sock *sk, u32 pmtu); // 根据 PMTU 同步本地 MSS 函数指针
101     __u8                       icsk_ca_state;     // 拥塞控制状态
102     __u8                       icsk_retransmits;  // 超时重传次数
103     __u8                       icsk_pending;      // 标识预定的定时器事件
                                             // ICSK_TIME_RETRANS：重传计数器
                                             // ICSK_TIME_DACK：延迟ACK计时器
                                             // ICSK_TIME_PROBE0：零窗口探测计时器
                                             // ICSK_TIME_KEEPOPEN：保活计时器
104     __u8                       icsk_backoff;      // 计时器的指数退避算法的指数
105     __u8                       icsk_syn_retries;  // 最多重传 SYN 的次数，TCP_SYNCNT 设置
106     __u8                       icsk_probes_out;   // 零窗口探测或保活时发送但为确认的 TCP 分节数
107     __u16                      icsk_ext_hdr_len;  // IP 首部选项长度
108     struct { // 延迟确认控制块
109         __u8                   pending;           // ACK状态
                                             // ICSK_ACK_SCHED：
                                             // ICSK_ACK_TIMER：
                                             // ICSK_ACK_PUSHED：
                                             // ICSK_ACK_PUSHED2：
110         __u8                   quick;             // ？？
111         __u8                   pingpong;          // 快速确认（0）或延迟确认（1）
112         __u8                   blocked;  
113         __u32                  ato;               // 用来计算延迟 ACK 的估值
114         unsigned long          timeout;           // 当前的延迟 ACK 超时时间
115         __u32                  lrcvtime;          // 最后一次收到数据的时间
116         __u16                  last_seg_size;     // 最后收到的数据的长度
117         __u16                  rcv_mss;           // 由最近接收到的段计算出的 MSS，用来确定是否延迟 ACK
118     } icsk_ack; 
119     struct { // 最大传输单元（MTU）发现相关的数据
120         int                    enabled;           // 是否启用
121 
122         /* Range of MTUs to search */
123         int                    search_high;       // 上限
124         int                    search_low;        // 下限
125 
126         /* Information on the current probe. */
127         int                    probe_size;        // 当前探测大小
128     } icsk_mtup;
129     u32                        icsk_ca_priv[16];  // 拥塞控制算法是私有参数
130     u32                        icsk_user_timeout;
131 #define ICSK_CA_PRIV_SIZE   (16 * sizeof(u32))
132 };
```

## tcp_sock
```
/// @file include/linux/tcp.h
138 struct tcp_sock {
139     /* inet_connection_sock has to be the first member of tcp_sock */
140     struct inet_connection_sock inet_conn;
141     u16 tcp_header_len; // 首部大小
142     u16 xmit_size_goal_segs; /* Goal for segmenting output packets */
148     __be32  pred_flags;
155     u32 rcv_nxt;    // 期待收到的下一个 TCP 序号
156     u32 copied_seq; /* Head of yet unread data      */
157     u32 rcv_wup;    /* rcv_nxt on last window update sent   */
158     u32 snd_nxt;    // 等待发送的下一个 TCP 序号
159 
160     u32 snd_una;    // 最早一个未确认的序号
161     u32 snd_sml;    // 最近发送的小包的最后一个字节序号，用于判断是否启用 Nagle 算法
162     u32 rcv_tstamp; // 最近一次收到 ACK 的时间，用于 TCP 保活
163     u32 lsndtime;   // 最近一次发送数据的时间，用于拥塞窗口的设置
164 
165     u32 tsoffset;   /* timestamp offset */
166 
167     struct list_head tsq_node; /* anchor in tsq_tasklet.head list */
168     unsigned long   tsq_flags;
169 
170     /* Data for direct copy to user */
171     struct { // 控制赋值数据到用户进程的控制结构
172         struct sk_buff_head prequeue;
173         struct task_struct  *task;
174         struct iovec        *iov;
175         int         memory; // prequeue 当前消耗的内存
176         int         len; // 用户缓存大小
177 #ifdef CONFIG_NET_DMA
178         /* members for async copy */
179         struct dma_chan     *dma_chan;
180         int         wakeup;
181         struct dma_pinned_list  *pinned_list;
182         dma_cookie_t        dma_cookie;
183 #endif
184     } ucopy;
185 
186     u32 snd_wl1;    // 记录更新发送窗口的那个 ACK 的序号，用于判断是否需要更新窗口大小
187     u32 snd_wnd;    // 接收方提供的接收窗口大小
188     u32 max_window; // 接收方通告过的接收窗口值最大值
189     u32 mss_cache;  // 发送方当前有效 MSS，SOL_TCP 设置
190 
191     u32 window_clamp;   // 滑动窗口最大值
192     u32 rcv_ssthresh;   // 当前接收窗口大小的阈值
193 
194     u16 advmss;     /* Advertised MSS           */
195     u8  unused;
196     u8  nonagle     : 4,/* Disable Nagle algorithm?             */
197         thin_lto    : 1,/* Use linear timeouts for thin streams */
198         thin_dupack : 1,/* Fast retransmit on first dupack      */
199         repair      : 1,
200         frto        : 1;/* F-RTO (RFC5682) activated in CA_Loss */
201     u8  repair_queue;
202     u8  do_early_retrans:1,/* Enable RFC5827 early-retransmit  */
203         syn_data:1, /* SYN includes data */
204         syn_fastopen:1, /* SYN includes Fast Open option */
205         syn_data_acked:1,/* data in SYN is acked by SYN-ACK */
206         is_cwnd_limited:1;/* forward progress limited by snd_cwnd? */
207     u32 tlp_high_seq;   /* snd_nxt at the time of TLP retransmit. */
208 
209 /* RTT measurement */
210     u32 srtt_us;    /* smoothed round trip time << 3 in usecs */
211     u32 mdev_us;    /* medium deviation         */
212     u32 mdev_max_us;    /* maximal mdev for the last rtt period */
213     u32 rttvar_us;  /* smoothed mdev_max            */
214     u32 rtt_seq;    /* sequence number to update rttvar */
215 
216     u32 packets_out;    /* Packets which are "in flight"    */
217     u32 retrans_out;    /* Retransmitted packets out        */
218     u32 max_packets_out;  /* max packets_out in last window */
219     u32 max_packets_seq;  /* right edge of max_packets_out flight */
220 
221     u16 urg_data;   /* Saved octet of OOB data and control flags */
222     u8  ecn_flags;  /* ECN status bits.         */
223     u8  reordering; /* Packet reordering metric.        */
224     u32 snd_up;     /* Urgent pointer       */
225 
226     u8  keepalive_probes; /* num of allowed keep alive probes   */
227 /*
228  *      Options received (usually on last packet, some only on SYN packets).
229  */
230     struct tcp_options_received rx_opt;
231 
232 /*
233  *  Slow start and congestion control (see also Nagle, and Karn & Partridge)
234  */
235     u32 snd_ssthresh;   /* Slow start size threshold        */
236     u32 snd_cwnd;   /* Sending congestion window        */
237     u32 snd_cwnd_cnt;   /* Linear increase counter      */
238     u32 snd_cwnd_clamp; /* Do not allow snd_cwnd to grow above this */
239     u32 snd_cwnd_used;
240     u32 snd_cwnd_stamp;
241     u32 prior_cwnd; /* Congestion window at start of Recovery. */
242     u32 prr_delivered;  /* Number of newly delivered packets to
243                  * receiver in Recovery. */
244     u32 prr_out;    /* Total number of pkts sent during Recovery. */
245 
246     u32 rcv_wnd;    /* Current receiver window      */
247     u32 write_seq;  /* Tail(+1) of data held in tcp send buffer */
248     u32 notsent_lowat;  /* TCP_NOTSENT_LOWAT */
249     u32 pushed_seq; /* Last pushed seq, required to talk to windows */
250     u32 lost_out;   /* Lost packets         */
251     u32 sacked_out; /* SACK'd packets           */
252     u32 fackets_out;    /* FACK'd packets           */
253     u32 tso_deferred;
254 
255     /* from STCP, retrans queue hinting */
256     struct sk_buff* lost_skb_hint;
257     struct sk_buff *retransmit_skb_hint;
258 
259     /* OOO segments go in this list. Note that socket lock must be held,
260      * as we do not use sk_buff_head lock.
261      */
262     struct sk_buff_head out_of_order_queue;
263 
264     /* SACKs data, these 2 need to be together (see tcp_options_write) */
265     struct tcp_sack_block duplicate_sack[1]; /* D-SACK block */
266     struct tcp_sack_block selective_acks[4]; /* The SACKS themselves*/
267 
268     struct tcp_sack_block recv_sack_cache[4];
269 
270     struct sk_buff *highest_sack;   /* skb just after the highest
271                      * skb with SACKed bit set
272                      * (validity guaranteed only if
273                      * sacked_out > 0)
274                      */
275 
276     int     lost_cnt_hint;
277     u32     retransmit_high;    /* L-bits may be on up to this seqno */
278 
279     u32 lost_retrans_low;   /* Sent seq after any rxmit (lowest) */
280 
281     u32 prior_ssthresh; /* ssthresh saved at recovery start */
282     u32 high_seq;   /* snd_nxt at onset of congestion   */
283 
284     u32 retrans_stamp;  /* Timestamp of the last retransmit,
285                  * also used in SYN-SENT to remember stamp of
286                  * the first SYN. */
287     u32 undo_marker;    /* tracking retrans started here. */
288     int undo_retrans;   /* number of undoable retransmissions. */
289     u32 total_retrans;  /* Total retransmits for entire connection */
290 
291     u32 urg_seq;    /* Seq of received urgent pointer */
292     unsigned int        keepalive_time;   /* time before keep alive takes place */
293     unsigned int        keepalive_intvl;  /* time interval between keep alive probes */
294 
295     int         linger2;
296 
297 /* Receiver side RTT estimation */
298     struct {
299         u32 rtt;
300         u32 seq;
301         u32 time;
302     } rcv_rtt_est;
303 
304 /* Receiver queue space */
305     struct {
306         int space;
307         u32 seq;
308         u32 time;
309     } rcvq_space;
310 
311 /* TCP-specific MTU probe information. */
312     struct {
313         u32       probe_seq_start;
314         u32       probe_seq_end;
315     } mtu_probe;
316     u32 mtu_info; /* We received an ICMP_FRAG_NEEDED / ICMPV6_PKT_TOOBIG
317                * while socket was owned by user.
318                */
319 
320 #ifdef CONFIG_TCP_MD5SIG
321 /* TCP AF-Specific parts; only used by MD5 Signature support so far */
322     const struct tcp_sock_af_ops    *af_specific;
323 
324 /* TCP MD5 Signature Option information */
325     struct tcp_md5sig_info  __rcu *md5sig_info;
326 #endif
327 
328 /* TCP fastopen related information */
329     struct tcp_fastopen_request *fastopen_req;
330     /* fastopen_rsk points to request_sock that resulted in this big
331      * socket. Used to retransmit SYNACKs etc.
332      */
333     struct request_sock *fastopen_rsk;
334 };
```