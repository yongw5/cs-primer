## 频道与模式
在发布/订阅（Publish/Subscribe）模式中，消息发送（发布）者（Redis 服务器）并不是将消息发送给某个或者某些消息接受（订阅）者（Redis 客户端），而是将发布的消息定性为频道，并且不需要知道该频道可能有什么（如果有）订阅者。同样，订阅者可以对一个或者多个频道感兴趣（订阅），并且只接受感兴趣的消息，而不需要该频道的发布者是谁。

redisServer 定义了频道（channle）相关的变量，Redis 客户端可以订阅相关频道。Redis 服务器将所有频道的订阅关系都保存在 pubsub_channels 字典中，该字典的 Key 是某个被订阅的频道，而 Value 是一个链表，记录了所有订阅该频道的客户端。
```
// server.c
struct redisServer {
/* ... *//* Pubsub */
    dict *pubsub_channels;  /* Map channels to list of subscribed clients */
    list *pubsub_patterns;  /* A list of pubsub_patterns */
    dict *pubsub_patterns_dict;  /* A dict of pubsub_patterns */
    int notify_keyspace_events; /* Events to propagate via Pub/Sub. This is an
                                   xor of NOTIFY_... flags. */
/* ... */
};

typedef struct client {
/* ... */
    dict *pubsub_channels;  /* channels a client is interested in (SUBSCRIBE) */
    list *pubsub_patterns;  /* patterns a client is interested in (SUBSCRIBE) */
/* ... */
} client;
```
除此之外，Redis 还支持客户端通过模式，订阅某些相关的频道。比如 Redis 客户端订阅 news.* 模式，则该客户端会受到如下频道的消息（如果服务器存在这些频道）：
```
news.art.figurative
news.music.jazz
news.xxx
```
Redis 服务器将所有模式的订阅关系保存在 pubsub_patterns 列表中，该列表的成员是 pubsubPattern 对象，记录的 Redis 客户端以及其订阅的模式。
```
// server.c
typedef struct pubsubPattern {
    client *client;
    robj *pattern;
} pubsubPattern;
```
而且 pubsub_patterns_dict 字典还保存 Redis 服务器所有被订阅的模式，方便查找。

## SUBSCRIBE/UNSUBSCRIBE
Redis 客户端可以通过 SUBSCRIBE 命令订阅某个或者某些频道，比如
```
SUBSCRIBE music
SUBSCRIBE music sports
```
Redis 服务器调用 subscribeCommand() 函数处理 SUBSCRIBE 命令
```
// pubsub.c
void subscribeCommand(client *c) {
    int j;

    for (j = 1; j < c->argc; j++)
        pubsubSubscribeChannel(c,c->argv[j]);
    c->flags |= CLIENT_PUBSUB;
}
```
pubsubSubscribeChannel() 函数负责将订阅信息保存到 pubsub_channels，包括将订阅的频道加入 server.pubsub_channels 以及 client.pubsub_channels 中。
```
// pubsub.c
int pubsubSubscribeChannel(client *c, robj *channel) {
    dictEntry *de;
    list *clients = NULL;
    int retval = 0;

    /* Add the channel to the client -> channels hash table */
    if (dictAdd(c->pubsub_channels,channel,NULL) == DICT_OK) {
        retval = 1;
        incrRefCount(channel);
        /* Add the client to the channel -> list of clients hash table */
        de = dictFind(server.pubsub_channels,channel);
        if (de == NULL) {
            clients = listCreate();
            dictAdd(server.pubsub_channels,channel,clients);
            incrRefCount(channel);
        } else {
            clients = dictGetVal(de);
        }
        listAddNodeTail(clients,c);
    }
    /* Notify the client */
    addReplyPubsubSubscribed(c,channel);
    return retval;
}
```
UNSUBSCRIBE 负责取消订阅频道，Redis 服务器调用 unsubscribeCommand() 函数处理该命令
```
// pubsub.c
void unsubscribeCommand(client *c) {
    if (c->argc == 1) {
        pubsubUnsubscribeAllChannels(c,1);
    } else {
        int j;

        for (j = 1; j < c->argc; j++)
            pubsubUnsubscribeChannel(c,c->argv[j],1);
    }
    if (clientSubscriptionsCount(c) == 0) c->flags &= ~CLIENT_PUBSUB;
}
```

## PSUBSCRIBE/PUNSUBSCRIBE
Redis 客户端可以通过 PSUBSCRIBE 命令订阅某个或者某些模式，比如
```
PSUBSCRIBE news.* sports.[ie]t
```
Redis 服务器调用 psubscribeCommand() 函数处理该命令
```
// pubsub.c
void psubscribeCommand(client *c) {
    int j;

    for (j = 1; j < c->argc; j++)
        pubsubSubscribePattern(c,c->argv[j]);
    c->flags |= CLIENT_PUBSUB;
}
```
pubsubSubscribePattern() 函数负责将模式订阅关系添加到 {client, server}.pubsub_patterns 中
```
// pubsub.c
int pubsubSubscribePattern(client *c, robj *pattern) {
    dictEntry *de;
    list *clients;
    int retval = 0;

    if (listSearchKey(c->pubsub_patterns,pattern) == NULL) {
        retval = 1;
        pubsubPattern *pat;
        listAddNodeTail(c->pubsub_patterns,pattern);
        incrRefCount(pattern);
        pat = zmalloc(sizeof(*pat));
        pat->pattern = getDecodedObject(pattern);
        pat->client = c;
        listAddNodeTail(server.pubsub_patterns,pat);
        /* Add the client to the pattern -> list of clients hash table */
        de = dictFind(server.pubsub_patterns_dict,pattern);
        if (de == NULL) {
            clients = listCreate();
            dictAdd(server.pubsub_patterns_dict,pattern,clients);
            incrRefCount(pattern);
        } else {
            clients = dictGetVal(de);
        }
        listAddNodeTail(clients,c);
    }
    /* Notify the client */
    addReplyPubsubPatSubscribed(c,pattern);
    return retval;
}
```
UNSUBSCRIBE 负责取消订阅模式，Redis 服务器调用 punsubscribeCommand() 函数处理该命令
```
void punsubscribeCommand(client *c) {
    if (c->argc == 1) {
        pubsubUnsubscribeAllPatterns(c,1);
    } else {
        int j;

        for (j = 1; j < c->argc; j++)
            pubsubUnsubscribePattern(c,c->argv[j],1);
    }
    if (clientSubscriptionsCount(c) == 0) c->flags &= ~CLIENT_PUBSUB;
}
```

## PUHLISH
某个 Redis 客户端可以通过 PUHLISH 命令发布消息，比如
```
PUHLISH music JayZhou
```
Redis 服务器调用 publishCommand() 处理该命令
```
// pubsub.c
void publishCommand(client *c) {
    int receivers = pubsubPublishMessage(c->argv[1],c->argv[2]);
    if (server.cluster_enabled)
        clusterPropagatePublish(c->argv[1],c->argv[2]);
    else
        forceCommandPropagation(c,PROPAGATE_REPL);
    addReplyLongLong(c,receivers);
}
```
首先，调用 pubsubPublishMessage() 函数将消息发送给所有订阅者和模式匹配者，然后如果是 Redis 集群，将调用 clusterPropagatePublish() 函数处理，否则调用 forceCommandPropagation() 函数，将命令持久化。pubsubPublishMessage() 函数定义如下：
```
// pubsub.c
int pubsubPublishMessage(robj *channel, robj *message) {
    int receivers = 0;
    dictEntry *de;
    dictIterator *di;
    listNode *ln;
    listIter li;

    /* Send to clients listening for that channel */
    de = dictFind(server.pubsub_channels,channel);
    if (de) {
        list *list = dictGetVal(de);
        listNode *ln;
        listIter li;

        listRewind(list,&li);
        while ((ln = listNext(&li)) != NULL) {
            client *c = ln->value;
            addReplyPubsubMessage(c,channel,message);
            receivers++;
        }
    }
    /* Send to clients listening to matching channels */
    di = dictGetIterator(server.pubsub_patterns_dict);
    if (di) {
        channel = getDecodedObject(channel);
        while((de = dictNext(di)) != NULL) {
            robj *pattern = dictGetKey(de);
            list *clients = dictGetVal(de);
            if (!stringmatchlen((char*)pattern->ptr,
                                sdslen(pattern->ptr),
                                (char*)channel->ptr,
                                sdslen(channel->ptr),0)) continue;

            listRewind(clients,&li);
            while ((ln = listNext(&li)) != NULL) {
                client *c = listNodeValue(ln);
                addReplyPubsubPatMessage(c,pattern,channel,message);
                receivers++;
            }
        }
        decrRefCount(channel);
        dictReleaseIterator(di);
    }
    return receivers;
}
```

## 消息格式
需要明确的是，Redis 客户端不只会收到 PUBLISH 推送的消息，定义和取消订阅都会返回消息给客户端。消息统一的格式是
```
<subscribe/unsubscribe/message><channle><msg>
```
- subscribe 表示订阅成功，第二个字段 <channle> 就表示成功订阅的频道。第三个字段 <msg> 表示当前订阅的频道数量；
- unsubscribe 表示取消订阅成功。第二个字段 <channle> 就表示成功取消订阅的频道。第三个字段 <msg> 表示当前订阅的频道数量，如果为 0，表示没有订阅任何频道；
- message 表示收到来自 PUBLISH 的消息。第二个字段表示消息来源 channle。第三个字段表示消息的内容。

## PUBSUB
PUBSUB 是一个查看订阅与发布系统状态的命令，比如某个频道有多少订阅者，或者某个模式有多少订阅者，具有一些列子命令。
- PUBSUB CHANNLES [pattern]：返回当前的活跃频道；
- PUBSUB NUMSUB [channle-1, channle-N]：返回给定频道订阅者的数量（不包括订阅模式）；
- PUBSUB NUMPAT：返回订阅模式的数量；

```
// pubsub.c
void pubsubCommand(client *c) {
    if (c->argc == 2 && !strcasecmp(c->argv[1]->ptr,"help")) {
        const char *help[] = {
"CHANNELS [<pattern>] -- Return the currently active channels matching a pattern (default: all).",
"NUMPAT -- Return number of subscriptions to patterns.",
"NUMSUB [channel-1 .. channel-N] -- Returns the number of subscribers for the specified channels (excluding patterns, default: none).",
NULL
        };
        addReplyHelp(c, help);
    } else if (!strcasecmp(c->argv[1]->ptr,"channels") &&
        (c->argc == 2 || c->argc == 3))
    {
        /* PUBSUB CHANNELS [<pattern>] */
        sds pat = (c->argc == 2) ? NULL : c->argv[2]->ptr;
        dictIterator *di = dictGetIterator(server.pubsub_channels);
        dictEntry *de;
        long mblen = 0;
        void *replylen;

        replylen = addReplyDeferredLen(c);
        while((de = dictNext(di)) != NULL) {
            robj *cobj = dictGetKey(de);
            sds channel = cobj->ptr;

            if (!pat || stringmatchlen(pat, sdslen(pat),
                                       channel, sdslen(channel),0))
            {
                addReplyBulk(c,cobj);
                mblen++;
            }
        }
        dictReleaseIterator(di);
        setDeferredArrayLen(c,replylen,mblen);
    } else if (!strcasecmp(c->argv[1]->ptr,"numsub") && c->argc >= 2) {
        /* PUBSUB NUMSUB [Channel_1 ... Channel_N] */
        int j;

        addReplyArrayLen(c,(c->argc-2)*2);
        for (j = 2; j < c->argc; j++) {
            list *l = dictFetchValue(server.pubsub_channels,c->argv[j]);

            addReplyBulk(c,c->argv[j]);
            addReplyLongLong(c,l ? listLength(l) : 0);
        }
    } else if (!strcasecmp(c->argv[1]->ptr,"numpat") && c->argc == 2) {
        /* PUBSUB NUMPAT */
        addReplyLongLong(c,listLength(server.pubsub_patterns));
    } else {
        addReplySubcommandSyntaxError(c);
    }
}
```