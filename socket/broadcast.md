## 多播 --- 数据传输基于 UDP 完成
多播数据传输特点
- 多播服务端针对特定多播组，只发送一次数据
- 即使只发送一次数据，但该组内的所有客户端都会接受数据
- 多播组数可以在 IP 地址范围内任意增加
- 加入特定组即可接受发往该多播组的数据

多播组是 D 类 IP 地址（224.0.0.0～239.255.255.255），向网络传递 1 个数据包时，路由器将复制该数据包并传递到多个主机

## TTL（Time to Live）
- 决定“数据包传递距离”的主要因素。
- TTL 用整数表示，每经过一个路由器就减 1
- TTL 变为 0 时，该数据包无法再被传递，只能销毁
- 通过套接字可选项 IP_MULTICAST_TTL 可以改变 TTL 的值
  ```
  int send_sock;
  int time_to_live = 64;
  ...
  send_sock = socket(PF_INET, SOCK_DGRAM, 0);
  setsockopt(send_sock, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&time_to_live, sizeof(time_to_live));
  ...
  ```
- 加入多播组也是通过设置套接字选项 IP_ADD_MEMBERSHIP 完成
  ```
  int recv_sock;
  struct ip_mreq join_addr;
  ...
  recv_sock = socket(PF_INET, SOCK_DGRAM, 0);
  ...
  join_addr.imr_multiaddr.s_addr = "inet_addr("224.0.0.14")";
  join_addr.imr_interface.s_addr = htonl(INADDR_ANY);
  setsockopt(recv_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&join_addr, sizeof(join_addr));
  ...

  struct ip_mreq {
    struct in_addr imr_multiaddr; // 加入的多播组 IP 地址
    struct in_addr imr_interface; // 加入该组的套接字所属主机的 IP 地址
  };
  ```

## 广播
- 基于 UDP 完成
- 只能向同一网络中的主机传输数据
- 类别（主要差别是 IP 地址）
  - 直接广播
    - IP 地址除了网络地址外，其余主机地址全部为1
  - 本地广播
    - IP 地址限定为 255.255.255.255 
- 通过更改套接字可选项 SO_BROADCAST 实现广播（默认生成的套接字会阻止广播）
  ```
  int send_sock;
  int bcast = 1;
  ...
  send_sock = socket(PF_INET, SOCK_DGRAM, 0);
  ...
  setsockopt(send_sock, SOL_SOCKET, SO_BORADCASE, (void*)&bcast, sizeof(bcast));
  ...
  ```

## 使用广播的应用
- ARP（地址解析协议），是 IPv4 的基本组成部分之一，使用链路层广播，“IP 地址为 a.b.c.d 的系统亮明身份，告诉我你的硬件地址”。
- DHCP（动态主机配置协议）
- NTP（网络时间协议）
- 路由守护进程