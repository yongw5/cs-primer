## sock_common
所有传输控制块的共有成员。
```
/// @file include/net/sock.h
  1 /*
  2  * INET     An implementation of the TCP/IP protocol suite for the LINUX
  3  *      operating system.  INET is implemented using the  BSD Socket
  4  *      interface as the means of communication with the user level.
  5  *
  6  *      Definitions for the AF_INET socket handler.
132 /**
154  *  This is the minimal network layer representation of sockets, the header
155  *  for struct sock and struct inet_timewait_sock.
156  */
157 struct sock_common {
158     /* skc_daddr and skc_rcv_saddr must be grouped on a 8 bytes aligned
159      * address on 64bit arches : cf INET_MATCH()
160      */
161     union { // IP地址
162         __addrpair             skc_addrpair;       // IP 地址对
163         struct {
164             __be32             skc_daddr;          // 远端 IP 地址
165             __be32             skc_rcv_saddr;      // 本地 IP 地址
166         };
167     };
168     union  { // 查找哈希值
169         unsigned int           skc_hash;           // 与各种协议查找表一起使用的哈希值
170         __u16                  skc_u16hashes[2];   // DUP 查找表的两个哈希值
171     };
172     /* skc_dport && skc_num must be grouped as well */
173     union {
174         __portpair             skc_portpair;       // 端口对
175         struct {
176             __be16             skc_dport;          // 原端端口号，inet_dport/tw_dport使用
177             __u16              skc_num;            // 本地端口号，inet_num/tw_num使用
178         };
179     };
180 
181     unsigned short              skc_family;        // 协议族
182     volatile unsigned char      skc_state;         // TCP 状态
183     unsigned char               skc_reuse:4;       // 保存 SO_REUSEADDR 设置
184     unsigned char               skc_reuseport:4;   // 保存 SO_REUSEPORT 设置
185     int                         skc_bound_dev_if;  // 如果不为 0，代表绑定的 device 序列号
186     union {
187         struct hlist_node       skc_bind_node;     // 用于绑定协议查找表
188         struct hlist_nulls_node skc_portaddr_node; // 用于绑定 UDP 的查找表
189     };
190     struct proto               *skc_prot;          // 绑定各自的接口（函数）
191 #ifdef CONFIG_NET_NS
192     struct net                 *skc_net;           // 所属网络命名空间
193 #endif
194 
195 #if IS_ENABLED(CONFIG_IPV6)
196     struct in6_addr             skc_v6_daddr;      // IPv6 远端地址
197     struct in6_addr             skc_v6_rcv_saddr;  // IPv6 本地地址
198 #endif
199 
200     /*
201      * fields between dontcopy_begin/dontcopy_end
202      * are not copied in sock_copy()
203      */
204     /* private: */
205     int                         skc_dontcopy_begin[0];
206     /* public: */
207     union {
208         struct hlist_node       skc_node;
209         struct hlist_nulls_node skc_nulls_node;
210     };
211     int                         skc_tx_queue_mapping; // tx queue 值？？
212     atomic_t                    skc_refcnt;           // 引用计数
213     /* private: */
214     int                         skc_dontcopy_end[0];
215     /* public: */
216 };
```