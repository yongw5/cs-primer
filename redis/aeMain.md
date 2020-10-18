## aeEventLoop
Redis 服务器是一个事件驱动的服务程序，aeEventLoop 代表事件循环，借助 select、poll、epoll 或者 kqueue 系统调用处理文件事件和时间事件。
```
// ae.h
typedef struct aeEventLoop {
    int maxfd;    /* 当前注册的最大的文件描述符 */
    int setsize;  /* 可注册文件描述符上限 */
    long long timeEventNextId;      /* 时间事件 ID */
    time_t lastTime;
    aeFileEvent *events;            /* 注册的文件事件 */
    aeFiredEvent *fired;            /* 就绪的事件 */
    aeTimeEvent *timeEventHead;     /* 时间事件链表 */
    int stop;
    void *apidata;
    aeBeforeSleepProc *beforesleep; /* 阻塞前调用 */
    aeBeforeSleepProc *aftersleep;  /* 阻塞返回调用 */
    int flags;
} aeEventLoop;
```

## aeCreateEventLoop()
aeCreateEventLoop() 函数用于创建一个 aeEventLoop 对象。
```
// ae.c
aeEventLoop *aeCreateEventLoop(int setsize) {
    aeEventLoop *eventLoop;
    int i;

    if ((eventLoop = zmalloc(sizeof(*eventLoop))) == NULL) goto err;
    eventLoop->events = zmalloc(sizeof(aeFileEvent)*setsize);
    eventLoop->fired = zmalloc(sizeof(aeFiredEvent)*setsize);
    if (eventLoop->events == NULL || eventLoop->fired == NULL) goto err;
    eventLoop->setsize = setsize;
    eventLoop->lastTime = time(NULL);
    eventLoop->timeEventHead = NULL;
    eventLoop->timeEventNextId = 0;
    eventLoop->stop = 0;
    eventLoop->maxfd = -1;
    eventLoop->beforesleep = NULL;
    eventLoop->aftersleep = NULL;
    eventLoop->flags = 0;
    if (aeApiCreate(eventLoop) == -1) goto err;
    /* Events with mask == AE_NONE are not set. So let's initialize the
     * vector with it. */
    for (i = 0; i < setsize; i++)
        eventLoop->events[i].mask = AE_NONE;
    return eventLoop;

err:
    if (eventLoop) {
        zfree(eventLoop->events);
        zfree(eventLoop->fired);
        zfree(eventLoop);
    }
    return NULL;
}
```


## aeMain()
aeMain() 启动事件循环，每次循环调用 aeProcssEvents() 函数处理就绪的时间事件和文件事件
```
// ae.c
void aeMain(aeEventLoop *eventLoop) {
    eventLoop->stop = 0;
    while (!eventLoop->stop) {
        if (eventLoop->beforesleep != NULL)
            eventLoop->beforesleep(eventLoop);
        aeProcessEvents(eventLoop, AE_ALL_EVENTS|AE_CALL_AFTER_SLEEP);
    }
}
```

## aeProcessEvents()
aeProcessEvents() 函数用于处理就绪的时间事件（超时）和文件事件（可读或者可写）
```
// ae.c:
int aeProcessEvents(aeEventLoop *eventLoop, int flags)
{
    int processed = 0, numevents;

/* 如果没有就绪事件，立即返回 */
    if (!(flags & AE_TIME_EVENTS) && !(flags & AE_FILE_EVENTS)) return 0;

/* 即使没有注册的文件事件，因为需要主动阻塞直到下一个时间事件就绪 */
    if (eventLoop->maxfd != -1 ||
        ((flags & AE_TIME_EVENTS) && !(flags & AE_DONT_WAIT))) {
        int j;
        aeTimeEvent *shortest = NULL;
        struct timeval tv, *tvp;
/* 计算阻塞时间 */
        if (flags & AE_TIME_EVENTS && !(flags & AE_DONT_WAIT))
            shortest = aeSearchNearestTimer(eventLoop);  /* 最先超时的时间事件 */
        if (shortest) {
            long now_sec, now_ms;

            aeGetTime(&now_sec, &now_ms);  /* 当前时间 */
            tvp = &tv;

            long long ms =
                (shortest->when_sec - now_sec)*1000 +
                shortest->when_ms - now_ms; /* 计算阻塞时间 */

            if (ms > 0) {
                tvp->tv_sec = ms/1000;
                tvp->tv_usec = (ms % 1000)*1000;
            } else {
                tvp->tv_sec = 0;
                tvp->tv_usec = 0;
            }
        } else {
            if (flags & AE_DONT_WAIT) { /* 尽快返回 */
                tv.tv_sec = tv.tv_usec = 0;
                tvp = &tv;
            } else { /* 一直阻塞 */
                /* Otherwise we can block */
                tvp = NULL; /* wait forever */
            }
        }

        if (eventLoop->flags & AE_DONT_WAIT) {
            tv.tv_sec = tv.tv_usec = 0;
            tvp = &tv;
        }
/* IO 复用，可以是 select、poll 或者 epoll */
        numevents = aeApiPoll(eventLoop, tvp);

        if (eventLoop->aftersleep != NULL && flags & AE_CALL_AFTER_SLEEP)
            eventLoop->aftersleep(eventLoop);
/* 处理就绪的文件事件 */
        for (j = 0; j < numevents; j++) {
            aeFileEvent *fe = &eventLoop->events[eventLoop->fired[j].fd];
            int mask = eventLoop->fired[j].mask;
            int fd = eventLoop->fired[j].fd;
            int fired = 0; /* 统计当前 fd 就绪的事件 */

            int invert = fe->mask & AE_BARRIER; /* 是否忽略可写 */
            if (!invert && fe->mask & mask & AE_READABLE) { /* 处理可读 */
                fe->rfileProc(eventLoop,fd,fe->clientData,mask);
                fired++;
                fe = &eventLoop->events[fd]; /* Refresh in case of resize. */
            }

            if (fe->mask & mask & AE_WRITABLE) { /* 处理可写 */
                if (!fired || fe->wfileProc != fe->rfileProc) {
                    fe->wfileProc(eventLoop,fd,fe->clientData,mask);
                    fired++;
                }
            }

            if (invert) {
                fe = &eventLoop->events[fd]; /* Refresh in case of resize. */
                if ((fe->mask & mask & AE_READABLE) &&
                    (!fired || fe->wfileProc != fe->rfileProc))
                {
                    fe->rfileProc(eventLoop,fd,fe->clientData,mask);
                    fired++;
                }
            }

            processed++;
        }
    }

/* 就绪的时间事件 */
    if (flags & AE_TIME_EVENTS)
        processed += processTimeEvents(eventLoop);

    return processed;
}
```