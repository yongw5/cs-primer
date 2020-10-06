## connect(2) 接口
connect(2) 对于 TCP，建立一条与指定的外部地址的连接，如果在 connect(2) 调用前没有绑定地址和端口号，则会自动绑定一个地址和端口号到地址。对于无连接协议如 UDP 和 ICMP，connect(2) 则记录外部地址，以便发送数据报时使用。
```
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```
参数说明
- sockfd：本地套接字描述符
- addr：远端地址
- addlen：地址大小（字节数）

## 调用关系
<img src='./imgs/sys-connect.png'>

## sys_connect()
```
/// @file net/socket.c
1680 SYSCALL_DEFINE3(connect, int, fd, struct sockaddr __user *, uservaddr,
1681         int, addrlen)
1682 {
1683     struct socket *sock;
1684     struct sockaddr_storage address;
1685     int err, fput_needed;
1686 
1687     sock = sockfd_lookup_light(fd, &err, &fput_needed);
1688     if (!sock)
1689         goto out;
1690     err = move_addr_to_kernel(uservaddr, addrlen, &address); // 拷贝地址到内核空间
1691     if (err < 0)
1692         goto out_put;
1693 
1694     err =
1695         security_socket_connect(sock, (struct sockaddr *)&address, addrlen);
1696     if (err)
1697         goto out_put;
1698 
1699     err = sock->ops->connect(sock, (struct sockaddr *)&address, addrlen,
1700                  sock->file->f_flags); // inet_stream_connect()或inet_dgram_connect()
1701 out_put:
1702     fput_light(sock->file, fput_needed);
1703 out:
1704     return err;
1705 }
```

## sock->ops->connect()
### inet_stream_connect()
```
/// @file net/ipv4/af_inet.c
656 int inet_stream_connect(struct socket *sock, struct sockaddr *uaddr,
657             int addr_len, int flags)
658 {
659     int err;
660 
661     lock_sock(sock->sk);
662     err = __inet_stream_connect(sock, uaddr, addr_len, flags);
663     release_sock(sock->sk);
664     return err;
665 }
```
继续调用 \____inet_stream_connect()
```
/// @file net/ipv4/af_inet.c
569 int __inet_stream_connect(struct socket *sock, struct sockaddr *uaddr,
570               int addr_len, int flags)
571 {
572     struct sock *sk = sock->sk;
573     int err;
574     long timeo;
575 
576     if (addr_len < sizeof(uaddr->sa_family)) // 地址检查
577         return -EINVAL;
578 
579     if (uaddr->sa_family == AF_UNSPEC) {
580         err = sk->sk_prot->disconnect(sk, flags);
581         sock->state = err ? SS_DISCONNECTING : SS_UNCONNECTED;
582         goto out;
583     }
584 
585     switch (sock->state) {
586     default:
587         err = -EINVAL;
588         goto out;
589     case SS_CONNECTED:
590         err = -EISCONN;
591         goto out;
592     case SS_CONNECTING:
593         err = -EALREADY;
594         /* Fall out of switch with err, set for this state */
595         break;
596     case SS_UNCONNECTED: // 未建立连接，建立连接
597         err = -EISCONN;
598         if (sk->sk_state != TCP_CLOSE)
599             goto out;
600         // 继续调用tcp_v4_conncet()
601         err = sk->sk_prot->connect(sk, uaddr, addr_len);
602         if (err < 0)
603             goto out;
604 
605         sock->state = SS_CONNECTING; // 设置状态，正在连接
606 
607         /* Just entered SS_CONNECTING state; the only
608          * difference is that return value in non-blocking
609          * case is EINPROGRESS, rather than EALREADY.
610          */
611         err = -EINPROGRESS;
612         break;
613     }
614 
615     timeo = sock_sndtimeo(sk, flags & O_NONBLOCK);
616     // 等待三次握手完成
617     if ((1 << sk->sk_state) & (TCPF_SYN_SENT | TCPF_SYN_RECV)) {
618         int writebias = (sk->sk_protocol == IPPROTO_TCP) &&
619                 tcp_sk(sk)->fastopen_req &&
620                 tcp_sk(sk)->fastopen_req->data ? 1 : 0;
621 
622         /* Error code is set above */
623         if (!timeo || !inet_wait_for_connect(sk, timeo, writebias))
624             goto out; // 非阻塞或出错
625 
626         err = sock_intr_errno(timeo);
627         if (signal_pending(current))
628             goto out;
629     }
630 
631     /* Connection was closed by RST, timeout, ICMP error
632      * or another process disconnected us.
633      */
634     if (sk->sk_state == TCP_CLOSE) // 建立成功或失败
635         goto sock_error;
636 
637     /* sk->sk_err may be not zero now, if RECVERR was ordered by user
638      * and error was received after socket entered established state.
639      * Hence, it is handled normally after connect() return successfully.
640      */
641 
642     sock->state = SS_CONNECTED; // 三次握手成功完成，连接建立
643     err = 0;
644 out:
645     return err;
646 
647 sock_error:
648     err = sock_error(sk) ? : -ECONNABORTED;
649     sock->state = SS_UNCONNECTED;
650     if (sk->sk_prot->disconnect(sk, flags))
651         sock->state = SS_DISCONNECTING;
652     goto out;
653 }
```

### inet_dgram_connect()
```
/// @file net/ipv4/af_inet.c
524 int inet_dgram_connect(struct socket *sock, struct sockaddr *uaddr,
525                int addr_len, int flags)
526 {
527     struct sock *sk = sock->sk;
528 
529     if (addr_len < sizeof(uaddr->sa_family))
530         return -EINVAL;
531     if (uaddr->sa_family == AF_UNSPEC)
532         return sk->sk_prot->disconnect(sk, flags);
533     // 没有绑定地址，自动绑定
534     if (!inet_sk(sk)->inet_num && inet_autobind(sk))
535         return -EAGAIN;
536     return sk->sk_prot->connect(sk, uaddr, addr_len);
537 }
```