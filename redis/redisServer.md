## redisServer
redisServer 数据结构记录了 Redis 服务器所有的信息，表征了一个 Redis 服务器实例。包括如下属性
- 通用属性，包括配置文件、执行文件及其参数、命令表、常用命令、事件循环和 Redis 数据结构。
- 网络配置，包括端口和地址信息、连接的客户端等
- 持久化相关
- 配置相关
- 主从复制相关
- 集群相关
- 脚本相关

```
// server.c
struct redisServer {
/* 通用属性 */
    pid_t pid;                  /* 主进程 ID */
    char *configfile;           /* 配置文件（绝对路径） */
    char *executable;           /* 执行文件（绝对路径） */
    char **exec_argv;           /* 执行文件参数 */
/* ... */
    int hz;                     /* serverCron() 调用频率 */
    redisDb *db;                /* Redis 数据库数组 */
    dict *commands;             /* 命令表 */
    dict *orig_commands;        /* 旧命令表 */
    aeEventLoop *el;            /* 事件循环 */
    _Atomic unsigned int lruclock;
/* ... */
    char *pidfile;              /* PID 文件 */
/* ... */
    int sentinel_mode;          /* Sentinel 模式 */
/* ... */ /* 网络配置 */
    int port;                   /* TCP 端口号 */
    int tcp_backlog;            /* TCP 监听 backlog 大小 */
    char *bindaddr[CONFIG_BINDADDR_MAX]; /* 服务器绑定地址  */
    int bindaddr_count;         /* bindaddr 数组大小 */
/* ... */
    list *clients;              /* List of active clients */
    list *clients_to_close;     /* Clients to close asynchronously */
    list *clients_pending_write; /* There is to write or install handler. */
    list *clients_pending_read;  /* Client has pending read socket buffers. */
    list *slaves, *monitors;    /* List of slaves and MONITORs */
    client *current_client;     /* Current client executing the command. */
    rax *clients_timeout_table; /* Radix tree for blocked clients timeouts. */
    long fixed_time_expire;     /* If > 0, expire keys against server.mstime. */
    rax *clients_index;         /* Active clients dictionary by client ID. */
/* ... */ /* 常用命令 */
    struct redisCommand *delCommand, *multiCommand, *lpushCommand,
                        *lpopCommand, *rpopCommand, *zpopminCommand,
                        *zpopmaxCommand, *sremCommand, *execCommand,
                        *expireCommand, *pexpireCommand, *xclaimCommand,
                        *xgroupCommand, *rpoplpushCommand;
/* ... */ /* 配置参数 */
    _Atomic size_t client_max_querybuf_len; /* querybuf 缓冲区最大长度 */
    int dbnum;                      /* Redis 数据库数组长度 */
    int supervised;                 /* 1 if supervised, 0 otherwise. */
    int supervised_mode;            /* See SUPERVISED_* */
    int daemonize;                  /* True if running as a daemon */
/* ... AOF / RDB 持久化配置参数 ... */
/* ... 复制参数 ... */
/* ziplist、intset 最大长度或最多元素 */
    /* Zip structure config, see redis.conf for more information  */
    size_t hash_max_ziplist_entries;
    size_t hash_max_ziplist_value;
    size_t set_max_intset_entries;
    size_t zset_max_ziplist_entries;
    size_t zset_max_ziplist_value;
    size_t hll_sparse_max_bytes;
    size_t stream_node_max_bytes;
    long long stream_node_max_entries;
    /* List parameters */
    int list_max_ziplist_size;
    int list_compress_depth;
/* ... 集群相关 ... */
/* ... 脚本相关 ... */
/* ... */
};
```
Redis 在 server.c 文件中定义了一个全局变量 server
```
struct redisServer server;
```
因此，一个服务器只有一个 redisServer 实例。

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
/* ... */
};
```
- fd：与客户端连接的套接字文件描述符；
- conn：与客户端连接的 TCP；
- querybuf：输入缓冲区，保存来自客户端的请求，这些请求由 Redis 服务器根据 Redis 协议进行解析；
- argc, argv：客户端请求参数，Redis 服务器对应的请求处理函数可以从此读取处理参数，其中 argv[0] 保存请求的类别；
- db：客户端正在使用的 redisDb；
- reply, buf：reply 和 buf 是动态和静态缓冲区，用于保存服务器发送给客户端的答复， 一旦文件描述符可写，缓冲区的内容依次地写入套接字；

## connection
connection 表示一个套接字连接，并且记录了三个回调函数，conn_handler、write_handler 和 read_handler，分别处理连接、可写、可读事件。
```
// connection.h
struct connection {
    ConnectionType *type;
    ConnectionState state;
    short int flags;
    short int refs;
    int last_errno;
    void *private_data;
    ConnectionCallbackFunc conn_handler;
    ConnectionCallbackFunc write_handler;
    ConnectionCallbackFunc read_handler;
    int fd;
};
```