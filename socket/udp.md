## 基本 UDP 套接字编程
<img src='./imgs/udp-socket.png'>

##  recvfrom() 和 sendto() 函数
```
#include <sys/socket.h>
/* @param
 * sockfd：用于传输数据的 UDP 套接字文件描述符
 * buff：待传输数据的缓冲地址
 * nbytes：待传输数据的字节数
 * flags：可选参数，没有则传递 0
 * to：目的地址
 * addrlen：长度
 * return：成功时返回传输的字节数，失败时返回 -1
 */
ssize_t sendto(int              sockfd, 
               void            *buff, 
               size_t           nbytes, 
               int              flags, 
               struct sockaddr *to, 
               socklen_t        addrlen);

/* @param
 * sockfd：用于接收数据的 UDP 套接字文件描述符
 * buff：保持接收数据的缓冲地址
 * nbytes：可接收的最大字节数
 * flags：可选参数，没有则传递 0
 * from：保存源地址
 * addrlen：保存长度
 * return：成功时返回接受的字节数，失败时返回-1
 */
ssize_t recvfrom(int              sockfd, 
                 void            *buff, 
                 size_t           nbytes, 
                 int              flags, 
                 struct sockaddr *from, 
                 socklen_t        addrlen);
```
sendto() 函数传输数据的过程
1. 向 UDP 套接字注册远端 IP 地址和端口号；
2. 传输数据；
3. 删除 UDP 套接字中注册的目标地址信息。

## UDP 的 connect() 函数
UDP 套接字调用 connect() 函数并不是要与对方建立连接，没有三路握手，内核只是检查是否存在可知的错误，记录对端的 IP 地址和端口号（取自传递给 connect() 的套接字地址结构），然后立即返回到调用进程。

与默认的未连接 UDP 套接字相比，已连接 UDP 套接字（调用 connect() 的结果）
- 调用 sendto() 函数不能给输出操作指定目的 IP 地址和端口号（若使用，地址参数为 NULL，地址长度为 0）sendto() 只执行步骤 2 传输数据。改用 write() 或 send()
- 不必使用 recvfrom() 以获悉数据报的发送者，而改用 read()、recv() 或 recvmsg()
- 由已连接 UDP 套接字引发的异步错误会返回给它们所在的进程，而未连接 UDP 套接字不接受任何异步错误

创建已连接 UDP 套接字
```
sock = socket(PF_INET, SOCK_DGRAM, 0);
memset(&addr, 0, sizeof(addr));
addr.sin_family = AF_INET;
addr.sin_addr.s_addr = ...
addr.sin_port = ...
connect(sock, (struct sockaddr*)&addr, sizeof(addr)); // 绑定远端地址
```

## 一个 UDP 套接字多次调用 connect()
拥有一个已连接 UDP 套接字的进程可出于下列两个目的之一再次调用 connect()
- 指定新的 IP 地址和端口号
- 断开套接字