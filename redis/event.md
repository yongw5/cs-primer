## 事件 —— Reactor 模式实现
Redis 服务器是一个事件驱动程序，服务器需要处理以下两类事件：
- 文件事件：套接字可读或者可写
- 时间事件：超时

```
#define AE_FILE_EVENTS 1
#define AE_TIME_EVENTS 2
#define AE_ALL_EVENTS (AE_FILE_EVENTS|AE_TIME_EVENTS)
#define AE_DONT_WAIT 4
#define AE_CALL_AFTER_SLEEP 8
```
文件事件主要是可读和可写，另外，还提供 AE_BARRIER，对于 WRITABLE，如果在同一事件循环迭代中已触发 READABLE 事件，则永远不要触发该事件。 当您希望在发送回复之前将内容保留到磁盘上并且希望以组的方式进行操作时很有用。
```
#define AE_NONE 0
#define AE_READABLE 1
#define AE_WRITABLE 2
#define AE_BARRIER 4
```
Redis 定义了特定的函数处理文件事件和时间事件
```
typedef void aeFileProc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);
typedef int aeTimeProc(struct aeEventLoop *eventLoop, long long id, void *clientData);
typedef void aeEventFinalizerProc(struct aeEventLoop *eventLoop, void *clientData);
typedef void aeBeforeSleepProc(struct aeEventLoop *eventLoop);
```

## 文件事件
Redis 使用 I/O 多路复用同时监听多个套接字可读或者可写事件。当被监听的套接字准备好执行 accept、read、write 和 close 等操作时，套接字可读或者可写文件事件就会产生，此时可以调用相应的事件处理函数进行处理。
```
typedef struct aeFileEvent {
    int mask; /* one of AE_(READABLE|WRITABLE|BARRIER) */
    aeFileProc *rfileProc;
    aeFileProc *wfileProc;
    void *clientData;
} aeFileEvent;
```
Redis 服务端有两类套接字，一类监听套接字，一类是已连接套接字。监听套接字有新的可应答（acceptable）套接字出现（服务端执行 connect 操作）或者已连接套接字可读（客户端执行 write 或者 close 操作）时，服务端套接字产生 AE_READBLE 事件。当已连接套接字输出缓冲区有空闲内存，服务端已连接套接字产生 AE_WRITABLE 事件。

I/O 多路复用机制允许服务器同时监听多个套接字的 AE_READABLE 事件和 AE_WRITABLE 事件，如果一个套接字同时产生了这两种事件，AE_READABLE 事件被优先处理，等到 AE_READABLE 事件处理完之后，才处理 AE_WRITABLE 事件。

监听套接字 AE_READABLE 处理函数定义如下：
```
// networking.c
void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask) {
    int cport, cfd, max = MAX_ACCEPTS_PER_CALL;
    char cip[NET_IP_STR_LEN];
    UNUSED(el);
    UNUSED(mask);
    UNUSED(privdata);

    while(max--) {
        cfd = anetTcpAccept(server.neterr, fd, cip, sizeof(cip), &cport);
        if (cfd == ANET_ERR) {
            if (errno != EWOULDBLOCK)
                serverLog(LL_WARNING,
                    "Accepting client connection: %s", server.neterr);
            return;
        }
        serverLog(LL_VERBOSE,"Accepted %s:%d", cip, cport);
        acceptCommonHandler(connCreateAcceptedSocket(cfd),0,cip);
    }
}
```
而已连接套接字 AE_READABLE 和 AE_WRITABLE 事件通过函数 networking.c:connSocketEventHandler() 处理。

## 时间事件
Redis 服务器将所有时间事件都放在一个无序链表（不是按照超时时间从小到大排序）中，EventLoop 的每次循环中，就会遍历整个链表，查找所有已到达（超时）的时间事件，并调用相应的事件处理函数。
```
typedef struct aeTimeEvent {
    long long id; /* time event identifier. */
    long when_sec; /* seconds */
    long when_ms; /* milliseconds */
    aeTimeProc *timeProc;
    aeEventFinalizerProc *finalizerProc;
    void *clientData;
    struct aeTimeEvent *prev;
    struct aeTimeEvent *next;
} aeTimeEvent;
```