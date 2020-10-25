## redisDb
Redis 用 redisDb 数据结构表示数据库。Redis 服务器拥有多个 redisDb，在初始化服务器时，程序会根据配置参数创建目标数量的 redisDb 对象，不同的 redisDb 根据 id 区分。
```
// server.h
typedef struct redisDb {
    dict *dict;                 // 保存数据库所有 Key-Value 数据
    dict *expires;              // 保存 Key-Timeout 数据
    dict *blocking_keys;        // 保存 Key-Clients，阻塞在 Key 上的所有 Client
    dict *ready_keys;           // Key-NULL，就绪的 Key
    dict *watched_keys;         // Key-Clients
    int id;                     // 数据库 ID
    long long avg_ttl;          // 平均 TTL
    unsigned long expires_cursor;
    list *defrag_later;         
} redisDb;
```
<img src='./imgs/redis-redisDb.png'>

dict 存储 redisDb 所有的 Key-Value 数据，而 expires 存储 Key-Timeout 数据，其中 Key 只是一个指针，指向 dict 中对应的 Key，而 Timeout 是一个 64 位整数，表示 Key 失效的时间戳。blocking_keys 和 ready_keys 跟阻塞相关，blocking_keys 存储 Key-Client 数据，表示阻塞在某个 Key 上所有的 Client，这些 Client 用链表串连起来，而 ready_keys 表示就绪的 Key。同样，blocking_keys 和 ready_keys 中的 Key 也是指针，指向 dict 中对应的 Key。watched_keys 和 blocking_keys 具有相似的结构，也是 Key-Client 数据。

## dbAdd()
dbAdd() 主要调用 dictAdd() 函数向 db.dict 中添加 Key-Value 数据。如果 Key 已经存在，adAdd() 调用失败。
```
// db.c
void dbAdd(redisDb *db, robj *key, robj *val) {
    sds copy = sdsdup(key->ptr);
    int retval = dictAdd(db->dict, copy, val);

    serverAssertWithInfo(NULL,key,retval == DICT_OK);
    if (val->type == OBJ_LIST ||
        val->type == OBJ_ZSET ||
        val->type == OBJ_STREAM)
        signalKeyAsReady(db, key);
    if (server.cluster_enabled) slotToKeyAdd(key->ptr);
}
```

## dbOverwrite()
dbOverwrite() 处理 Key-Value 已经存在的情形。
```
void dbOverwrite(redisDb *db, robj *key, robj *val) {
    dictEntry *de = dictFind(db->dict,key->ptr);

    serverAssertWithInfo(NULL,key,de != NULL);
    dictEntry auxentry = *de;
    robj *old = dictGetVal(de); /* 记录旧值 */
    if (server.maxmemory_policy & MAXMEMORY_FLAG_LFU) {
        val->lru = old->lru;
    }
    dictSetVal(db->dict, de, val); /* 设置新值 */

    if (server.lazyfree_lazy_server_del) {
        freeObjAsync(old); /* 异步 free */
        dictSetVal(db->dict, &auxentry, NULL);
    }

    dictFreeVal(db->dict, &auxentry);
}
```

## setKey()
setKey() 函数直接调用 genericSetKey() 函数进行处理。genericSetKey() 函数结合了 dbAdd() 和 dbOverwrite() 函数的功能。
```
// db.c
void setKey(client *c, redisDb *db, robj *key, robj *val) {
    genericSetKey(c,db,key,val,0,1);
}

void genericSetKey(client *c, redisDb *db, robj *key, robj *val, int keepttl, int signal) {
    if (lookupKeyWrite(db,key) == NULL) {
        dbAdd(db,key,val);
    } else {
        dbOverwrite(db,key,val);
    }
    incrRefCount(val);
    if (!keepttl) removeExpire(db,key);
    if (signal) signalModifiedKey(c,db,key);
}
```

## dbDelete()
dbDelete() 将根据 server.lazyfree_lazy_server_del 配置，选择执行同步删除还是异步删除。
```
// db.c
int dbDelete(redisDb *db, robj *key) {
    return server.lazyfree_lazy_server_del ? dbAsyncDelete(db,key) :
                                             dbSyncDelete(db,key);
}
```

## expireIfNeed()
redisDb 过期 Key-Value 可以采用惰性删除（异步删除）策略，由 db.c:expireIfNeed() 函数完成，该函数主要被 lookupKey\*() 函数族调用。expireIfNeed() 函数的行为取决于 redisDb 的角色，因为 Slave 不会使 Key-Value 数据失效。Master 在解决一致性问题是发送 Slave DEL 命令时，Slave 才删除某个 Key-Value 数据。

expireIfNeed() 函数首先调用 keyIsExpired() 函数判断某一 Key 数据是否过期，基本逻辑是将 Key 保存的过期时间（如果存在）和当前时间比较。
```
// db.c
int keyIsExpired(redisDb *db, robj *key) {
    mstime_t when = getExpire(db,key);
    mstime_t now;

    if (when < 0) return 0; /* No expire for this key */

    if (server.loading) return 0; /* 加载阶段 */

    if (server.lua_caller) {
        now = server.lua_time_start;
    }
    else if (server.fixed_time_expire > 0) {
        now = server.mstime;
    }
    else {
        now = mstime();
    }
    return now > when;
}

```
如果一个 Key 过期，expireIfNeeded() 函数首先将删除命令传播到所有 Slave 服务器，然后根据 server.lazyfree_lazy_expire 的配置，执行异步删除（非阻塞）还是同步删除（阻塞）。
```
// db.c
int expireIfNeeded(redisDb *db, robj *key) {
    if (!keyIsExpired(db,key)) return 0;

/* Slave 不删除数据，返回 */
    if (server.masterhost != NULL) return 1;

    server.stat_expiredkeys++;
    propagateExpire(db,key,server.lazyfree_lazy_expire);
    notifyKeyspaceEvent(NOTIFY_EXPIRED,
        "expired",key,db->id);
    int retval = server.lazyfree_lazy_expire ? dbAsyncDelete(db,key) : /* 异步删除 */
                                               dbSyncDelete(db,key); /* 同步删除 */
    if (retval) signalModifiedKey(NULL,db,key);
    return retval;
}
```

## dbAsyncDelete()
dbAsyncDelete() 根据 Value 所占内存的大小（阈值为 LAZYFREE_THRESHOLD）决定是否启用后台线程进行异步删除处理。
```
// lazefree.c
#define LAZYFREE_THRESHOLD 64
int dbAsyncDelete(redisDb *db, robj *key) {
    if (dictSize(db->expires) > 0) dictDelete(db->expires,key->ptr);

    dictEntry *de = dictUnlink(db->dict,key->ptr);
    if (de) {
        robj *val = dictGetVal(de);
        size_t free_effort = lazyfreeGetFreeEffort(val);

        if (free_effort > LAZYFREE_THRESHOLD && val->refcount == 1) {
            atomicIncr(lazyfree_objects,1);
            bioCreateBackgroundJob(BIO_LAZY_FREE,val,NULL,NULL); /* 异步删除 */
            dictSetVal(db->dict,de,NULL);
        }
    }

    if (de) {
        dictFreeUnlinkedEntry(db->dict,de);
        if (server.cluster_enabled) slotToKeyDel(key->ptr);
        return 1;
    } else {
        return 0;
    }
}
```

## lazyfreeGetFreeEffort()
lazyfreeGetFreeEffort() 函数返回 obj 所占用内存。
```
// lazefree.c
size_t lazyfreeGetFreeEffort(robj *obj) {
    if (obj->type == OBJ_LIST) {
        quicklist *ql = obj->ptr;
        return ql->len;
    } else if (obj->type == OBJ_SET && obj->encoding == OBJ_ENCODING_HT) {
        dict *ht = obj->ptr;
        return dictSize(ht);
    } else if (obj->type == OBJ_ZSET && obj->encoding == OBJ_ENCODING_SKIPLIST){
        zset *zs = obj->ptr;
        return zs->zsl->length;
    } else if (obj->type == OBJ_HASH && obj->encoding == OBJ_ENCODING_HT) {
        dict *ht = obj->ptr;
        return dictSize(ht);
    } else {
        return 1; /* Everything else is a single allocation. */
    }
}
```

## lookupKey()
```
// db.c
robj *lookupKey(redisDb *db, robj *key, int flags) {
    dictEntry *de = dictFind(db->dict,key->ptr);
    if (de) {
        robj *val = dictGetVal(de);
        
        if (!hasActiveChildProcess() && !(flags & LOOKUP_NOTOUCH)){
            if (server.maxmemory_policy & MAXMEMORY_FLAG_LFU) {
                updateLFU(val);
            } else {
                val->lru = LRU_CLOCK();
            }
        }
        return val;
    } else {
        return NULL;
    }
}
```