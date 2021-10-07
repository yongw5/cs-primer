# 初识 Livevent
## Libevent 简介
Libevent 是一个提供异步事件通知的软件库，其核心机制为：在文件描述符 fd 上发生特定事件或者超时发生时，或者某个信号触发时，执行设置的回调函数。Libevent 旨在替换事件驱动网络服务器中的事件循环（EventLoop）。应用程序只需调用 event_dispatch()，然后动态添加或删除事件，而无需更改事件循环。

## Libevent 安装
从 GitHub 上下载最新 [Libevent](https://github.com/libevent/libevent) 代码，进入 libevent 目录，依次执行：
- ./autogen.sh
- ./configure
- make
- sudo make install

默认情况下，在 /user/local/lib 目录下可以找到 libevent 相关的动态库。

## 管中窥豹：HelloWord 示例
这个示例实现的是一个简单的服务器，监听 9995 端口的连接请求，当一个连接请求到来，回显一个 “Hello, World!”；当收到中断信号（例如前台执行时的ctrl + c），则退出。

### 定义连接相关回调函数
首先，定义一个监听端口新连接到来的回调 listener_cb：创建一个新的连接，设置连接可写、连接被关闭的回调函数。
```
listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
    struct sockaddr *sa, int socklen, void *user_data)
{
    struct event_base *base = user_data;
    struct bufferevent *bev;

    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        fprintf(stderr, "Error constructing bufferevent!");
        event_base_loopbreak(base);
        return;
    }
    bufferevent_setcb(bev, NULL, conn_writecb, conn_eventcb, NULL);
    bufferevent_enable(bev, EV_WRITE);
    bufferevent_disable(bev, EV_READ);

    bufferevent_write(bev, MESSAGE, strlen(MESSAGE));
}
```
conn_writecb() 在连接可写时，输出“flushed answer”
```
static void
conn_writecb(struct bufferevent *bev, void *user_data)
{
    struct evbuffer *output = bufferevent_get_output(bev);
    if (evbuffer_get_length(output) == 0) {
        printf("flushed answer\n");
        bufferevent_free(bev);
    }
}
```
### 定义信号回调函数
然后，设置收到中断信号的回调函数
```
static void
signal_cb(evutil_socket_t sig, short events, void *user_data)
{
    struct event_base *base = user_data;
    struct timeval delay = { 2, 0 };

    printf("Caught an interrupt signal; exiting cleanly in two seconds.\n");

    event_base_loopexit(base, &delay);
}
```
### 设置事件循环
```
int
main(int argc, char **argv)
{
    // 新建一个 event_base
    base = event_base_new();

    sin.sin_family = AF_INET;
        sin.sin_port = htons(PORT);
    // 连接事件
    listener = evconnlistener_new_bind(base, listener_cb, (void *)base,
        LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1,
        (struct sockaddr*)&sin,
        sizeof(sin));
    // 信号事件
    signal_event = evsignal_new(base, SIGINT, signal_cb, (void *)base);
    // 开始事件循环
    event_base_dispatch(base);
    // 清理
    evconnlistener_free(listener);
    event_free(signal_event);
    event_base_free(base);

    printf("done\n");
    return 0;
}
```

从 HelloWorld 示例可以看出，Libevent 核心组建是 event 和 event_base，用户只需要定义事件处理回调函数，然后将事件注册到 evnet_base 中，然后开始事件循环，event_base 就在事件就绪时，调用设置的回调函数。