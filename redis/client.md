## client
Redis 服务器可以与多个客户端建立 TCP 连接，每个客户端可以向服务器发送请求，而 Redis 服务器接受并处理客户端发送的请求，并向客户端返回处理结果。

每个与 Redis 服务器连接的客户端，Redis 服务器中都有一个对应的 server.c:client 数据结构保存客户端当前的状态信息，以及执行相关功能时需要用到的数据结构。client 数据结构的成员很多，下面展示几个重要的成员
```
// server.c
struct client {
    int fd;
    connection *conn;    
    redisDb *db;
    sds querybuf;
    int argc;
    robj **argv;
    int flags;
    list *reply;
    char buf[PROTO_REPLY_CHUNK_BYTES];
    ... many other fields ...
};
```
- fd：与客户端连接的套接字文件描述符；
- conn：与客户端连接的 TCP；
- querybuf：输入缓冲区，保存来自客户端的请求，这些请求由 Redis 服务器根据 Redis 协议进行解析；
- argc, argv：客户端请求参数，Redis 服务器对应的请求处理函数可以从此读取处理参数，其中 argv[0] 保存请求的类别；
- db：客户端正在使用的 redisDb；
- reply, buf：reply 和 buf 是动态和静态缓冲区，用于保存服务器发送给客户端的答复， 一旦文件描述符可写，缓冲区的内容依次地写入套接字；

有关 querybuf 解析的函数在 networking.c 文件中定义。
在 readQueryFromClient() 函数中，套接字读出的内容被保存在 querybuf 中。然后在 processInputBuffer() 函数中被解析并且执行（processCommandAndResetClient() 调用 server.c:processCommand() 函数执行相关请求命令）。

Redis 中利用 redisComman 数据结构请求命令，其定义如下：
```
struct redisCommand {
    char *name;
    redisCommandProc *proc;
    int arity;
    char *sflags;
    uint64_t flags;
    redisGetKeysProc *getkeys_proc;
    int firstkey; /* The first argument that's a key (0 = no keys) */
    int lastkey;  /* The last argument that's a key */
    int keystep;  /* The step between first and last key */
    long long microseconds, calls;
    int id;
};
```
其中 proc 函数指针，指向请求 name 对应的处理函数。