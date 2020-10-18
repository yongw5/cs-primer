## acceptTcpHandler()
acceptTcpHanlder() 用于处理监听套接字可读（有客户端发起连接请求）事件。首先调用 connCreateAcceptedSocket() 函数，为套接字文件 cfd 创建一个 connection 对象。connection 对象表示了一个套接字连接，记录了套接字文件描述符、套接字状态以及套接字可读或者可写时的处理函数。
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

## connCreateAcceptedSocket()
connCreateAcceptedSocket() 调用 connCreateSocket() 函数为套接字文件 cfd 创建一个 connection 对象并初始化
```
// connection.c
connection *connCreateAcceptedSocket(int fd) {
    connection *conn = connCreateSocket();
    conn->fd = fd;
    conn->state = CONN_STATE_ACCEPTING;
    return conn;
}
```
需要注意的是 connection 对象的 type 指向 CT_Socket。
```
// connection.c
connection *connCreateSocket() {
    connection *conn = zcalloc(sizeof(connection));
    conn->type = &CT_Socket;
    conn->fd = -1;

    return conn;
}
```
CT_Socket 是一个全局变量，定义如下：
```
// connection.c
ConnectionType CT_Socket = {
    .ae_handler = connSocketEventHandler,
    .close = connSocketClose,
    .write = connSocketWrite,
    .read = connSocketRead,
    .accept = connSocketAccept,
    .connect = connSocketConnect,
    .set_write_handler = connSocketSetWriteHandler,
    .set_read_handler = connSocketSetReadHandler,
    .get_last_error = connSocketGetLastError,
    .blocking_connect = connSocketBlockingConnect,
    .sync_write = connSocketSyncWrite,
    .sync_read = connSocketSyncRead,
    .sync_readline = connSocketSyncReadLine
};
```

## acceptCommonHandler()
acceptCommonHandler() 主要调用 createClient() 函数创建一个 client 对象，以及调用 connAccept() 处理连接。
```
// networking.c
#define MAX_ACCEPTS_PER_CALL 1000
static void acceptCommonHandler(connection *conn, int flags, char *ip) {
    client *c;
    UNUSED(ip);

    if (listLength(server.clients) >= server.maxclients) {
/* ... ERROR ... */
        return;
    }

    /* Create connection and client */
    if ((c = createClient(conn)) == NULL) {
/* ... ERROR ... */
        return;
    }

    /* Last chance to keep flags */
    c->flags |= flags;

    if (connAccept(conn, clientAcceptHandler) == C_ERR) {
        char conninfo[100];
        if (connGetState(conn) == CONN_STATE_ERROR)
/* ... ERROR ... */
        freeClient(connGetPrivateData(conn));
        return;
    }
}
```

## createClient()
createClient() 函数除了初始化 client 对象，还会调用 connSetReadHandler() 函数设置套接字可读时的处理函数。
```
// networking.c
client *createClient(connection *conn) {
    client *c = zmalloc(sizeof(client));

    if (conn) {
        connNonBlock(conn);
        connEnableTcpNoDelay(conn);
        if (server.tcpkeepalive)
            connKeepAlive(conn,server.tcpkeepalive);
        connSetReadHandler(conn, readQueryFromClient);
        connSetPrivateData(conn, c);
    }

    selectDb(c,0);
    uint64_t client_id = ++server.next_client_id;
/* ... */
    c->conn = conn;
    c->name = NULL;
/* ... */
    c->querybuf = sdsempty();
    c->pending_querybuf = sdsempty();
    c->querybuf_peak = 0;
    c->reqtype = 0;
    c->argc = 0;
    c->argv = NULL;
    c->cmd = c->lastcmd = NULL;
/* ... */
    return c;
}
```
connSetReadHandler() 函数直接调用 set_read_handler 指针指向的函数

```
// connection.h
static inline int connSetReadHandler(connection *conn, ConnectionCallbackFunc func) {
    return conn->type->set_read_handler(conn, func);
}
```
根据前面分析，set_read_handler 指针指向 connSocketSetReadHandler() 函数。 connSocketSetReadHandler() 将 connect 注册到 EventLoop 中，可读时，调用 ae_handler 指向的函数，即 connSocketEventHandler() 函数。
```
// connection.c
static int connSocketSetReadHandler(connection *conn, ConnectionCallbackFunc func) {
    if (func == conn->read_handler) return C_OK;

    conn->read_handler = func;
    if (!conn->read_handler)
        aeDeleteFileEvent(server.el,conn->fd,AE_READABLE);
    else
        if (aeCreateFileEvent(server.el,conn->fd,
                    AE_READABLE,conn->type->ae_handler,conn) == AE_ERR) return C_ERR;
    return C_OK;
}
```

## connAccept()
connAccept() 函数直接调用 accept 指针指向的函数。
```
// connection.h
static inline int connAccept(connection *conn, ConnectionCallbackFunc accept_handler) {
    return conn->type->accept(conn, accept_handler);
}
```
accept 指向 connSocketAccept() 函数。 connSocketAccept() 将 conn 的状态改为 CONN\_STATE\_CONNECTED，然后调用 callHanlder() 函数。
```
// connection.c
static int connSocketAccept(connection *conn, ConnectionCallbackFunc accept_handler) {
    int ret = C_OK;

    if (conn->state != CONN_STATE_ACCEPTING) return C_ERR;
    conn->state = CONN_STATE_CONNECTED;

    connIncrRefs(conn);
    if (!callHandler(conn, accept_handler)) ret = C_ERR;
    connDecrRefs(conn);

    return ret;
}
```
callHandler() 直接调用传入的函数回调，即 clientAcceptHandler() 函数。
```
// connhelper.h
static inline int callHandler(connection *conn, ConnectionCallbackFunc handler) {
    connIncrRefs(conn);
    if (handler) handler(conn);
    connDecrRefs(conn);
    if (conn->flags & CONN_FLAG_CLOSE_SCHEDULED) {
        if (!connHasRefs(conn)) connClose(conn);
        return 0;
    }
    return 1;
}
```
clientAcceptHandler() 函数主要处理自连接情况。
```
// networking.c
void clientAcceptHandler(connection *conn) {
    client *c = connGetPrivateData(conn);

    if (connGetState(conn) != CONN_STATE_CONNECTED) {
/* ... ERROR ... */
        return;
    }

    /* If the server is running in protected mode (the default) and there
     * is no password set, nor a specific interface is bound, we don't accept
     * requests from non loopback interfaces. Instead we try to explain the
     * user what to do to fix it if needed. */
    if (server.protected_mode &&
        server.bindaddr_count == 0 &&
        DefaultUser->flags & USER_FLAG_NOPASS &&
        !(c->flags & CLIENT_UNIX_SOCKET))
    {
        char cip[NET_IP_STR_LEN+1] = { 0 };
        connPeerToString(conn, cip, sizeof(cip)-1, NULL);

        if (strcmp(cip,"127.0.0.1") && strcmp(cip,"::1")) {
/* ... ERROR ... */
            return;
        }
    }

    server.stat_numconnections++;
    moduleFireServerEvent(REDISMODULE_EVENT_CLIENT_CHANGE,
                          REDISMODULE_SUBEVENT_CLIENT_CHANGE_CONNECTED,
                          c);
}
```