## dict
dict 是 Redis 哈希表的实现，采用链地址发解决冲突。dict 定义如下：
```
// dict.h
typedef struct dictEntry {
    void *key;
    union {
        void *val;
        uint64_t u64;
        int64_t s64;
        double d;
    } v;
    struct dictEntry *next; // 链地址法
} dictEntry;

typedef struct dictType { // 一些函数
    uint64_t (*hashFunction)(const void *key);
    void *(*keyDup)(void *privdata, const void *key);
    void *(*valDup)(void *privdata, const void *obj);
    int (*keyCompare)(void *privdata, const void *key1, const void *key2);
    void (*keyDestructor)(void *privdata, void *key);
    void (*valDestructor)(void *privdata, void *obj);
} dictType;

typedef struct dictht { // 哈希表
    dictEntry **table;
    unsigned long size;
    unsigned long sizemask;
    unsigned long used;
} dictht;

typedef struct dict {
    dictType *type;
    void *privdata;
    dictht ht[2];
    long rehashidx; /* 再哈希标志 */
    unsigned long iterators;
} dict;

typedef struct dictIterator {
    dict *d;
    long index;
    int table, safe;
    dictEntry *entry, *nextEntry;
    long long fingerprint;
} dictIterator;
```
dict 整体布局如下所示：
<img src='./imgs/redis-dict.png'>

在大多数时候，只有 ht[0].table 中有数据，ht[1].table 处于空闲。其中 dictht 中 size 表示哈希表的大小，used 表示处于使用中的条目，而 sizemask 用于将哈希函数（Redis 使用 MurmurHash2 哈希算法）的值转换成数组索引，因此，size 值总是 2^N，而 sizemask 的值为 2^N - 1，这样，通过将哈希值与 sizemask 简单地进行按位与操作便可转化成数组索引。
```
// dict.c
static long _dictKeyIndex(dict *d, const void *key, uint64_t hash, dictEntry **existing)
{
    unsigned long idx, table;
    dictEntry *he;
    if (existing) *existing = NULL;

    if (_dictExpandIfNeeded(d) == DICT_ERR) /* 扩容但是出错 */
        return -1;
    for (table = 0; table <= 1; table++) {
        idx = hash & d->ht[table].sizemask; /* 数组索引 */
        he = d->ht[table].table[idx];
        while(he) {
            if (key==he->key || dictCompareKeys(d, key, he->key)) {
                if (existing) *existing = he;
                return -1; /* 已经存在 */
            }
            he = he->next;
        }
        if (!dictIsRehashing(d)) break; /* 没有再哈希 */
    }
    return idx;
}
```
当哈希表 ht[0] 中的数据超过一定量或者空间有很多空闲，需要扩大空间或者缩小空间，这是需要借用 ht[1] 将 ht[0] 的数据重新进行散列，保存在 ht[1]。完成后，释放 ht[0] 的内存空间，将 ht[1] 的数据转移到 ht[0]，ht[1] 再次处于空闲状态。如果处于再哈希过程中，\_dictKeyIndex() 函数将返回 ht[1] 中的索引。

## dictRehash()
触发哈希表重新散列的条件如下：
- 服务器目前没有在执行 BGSAVE 或者 BGREWERITEAOF 命令，并且哈希表的负载因子大于等于 1；
- 服务器目前正在执行 BGSAVE 或者 BGREWERITEAOF 命令，并且哈希表的负载因子大于等于 5；
- 哈希表的负载因子小于 0.1；

其中，负载因子计算方式如下：
```
load_factor = ht[0].used / ht[0].size
```

再哈希时，新哈希表按照如下策略选取：
- 如果需要容量扩展，ht[1] 的大小为第一个不小于 ht[0].used * 2 并且是 2^N 的值。
- 如果需要搜索容量，ht[1] 的大小为第一个不小于 ht[0].used 并且是 2^N 的值。

另外，扩展或者收缩哈希表并不是一次性、集中式地完成，而是分多次，渐进式完成的。过程如下：
1. 为 ht[1] 分配空间，此时字典同时拥有 ht[0] 和 ht[1] 两个哈希表；
2. 将 rehashidx 赋值为 0，表示重新散列工作正式开始；
3. 每次对字典执行增删改查时，除了执行指定的操作外，还顺带将 ht[0] 哈希表在 rehashidx 索引上的数据转移到 ht[1]，转移结束后 rehashidx 递增 1；
4. 完成再哈希后，rehashidx 的值赋值为 -1，表示重新散列工作结束；

需要注意的是，再哈希期间，字典的删除、查找和更新操作会在两个哈希表上进行，而插入操作一律保存到 ht[1] 哈希表上，ht[0] 上不接受任何插入操作。这一措施确保 ht[0] 哈希表的数据只减不增。

再哈希处理函数 dictRehash() 定义如下：
```
// dict.c
int dictRehash(dict *d, int n) {
    int empty_visits = n*10; /* Max number of empty buckets to visit. */
    if (!dictIsRehashing(d)) return 0;

    while(n-- && d->ht[0].used != 0) {
        dictEntry *de, *nextde;

        /* Note that rehashidx can't overflow as we are sure there are more
         * elements because ht[0].used != 0 */
        assert(d->ht[0].size > (unsigned long)d->rehashidx);
        while(d->ht[0].table[d->rehashidx] == NULL) {
            d->rehashidx++;
            if (--empty_visits == 0) return 1;
        }
        de = d->ht[0].table[d->rehashidx];
        /* Move all the keys in this bucket from the old to the new hash HT */
        while(de) {
            uint64_t h;

            nextde = de->next;
            /* Get the index in the new hash table */
            h = dictHashKey(d, de->key) & d->ht[1].sizemask;
            de->next = d->ht[1].table[h];
            d->ht[1].table[h] = de;
            d->ht[0].used--;
            d->ht[1].used++;
            de = nextde;
        }
        d->ht[0].table[d->rehashidx] = NULL;
        d->rehashidx++;
    }

    /* Check if we already rehashed the whole table... */
    if (d->ht[0].used == 0) {
        zfree(d->ht[0].table);
        d->ht[0] = d->ht[1];
        _dictReset(&d->ht[1]);
        d->rehashidx = -1;
        return 0;
    }

    /* More to rehash... */
    return 1;
}
```

## dictAdd()
dictAdd() 函数为 dict 添加 Key-Value 数据，首先调用 dictAddRaw() 函数在 dict.ht 中获取一个 dictEntry，然后将 Value 添加到 dictEntry 中。
```
// dict.c
int dictAdd(dict *d, void *key, void *val)
{
    dictEntry *entry = dictAddRaw(d,key,NULL); /* 添加 Key */

    if (!entry) return DICT_ERR; /* 已经存在 */
    dictSetVal(d, entry, val); /* 设置 Value */
    return DICT_OK;
}
```
dictAddRaw() 函数仅仅向 dict.ht 中添加（或者查找）一个 dictEntry 并将 dictEntry 的地址返回。dictAddRaw 并不为 dictEntry 添加 Value ，而是返回 dictEntry 的地址，由调用者处理。
```
// dict.c
dictEntry *dictAddRaw(dict *d, void *key, dictEntry **existing)
{
    long index;
    dictEntry *entry;
    dictht *ht;

    if (dictIsRehashing(d)) _dictRehashStep(d);

    if ((index = _dictKeyIndex(d, key, dictHashKey(d,key), existing)) == -1)
        return NULL;
/* 再哈希时使用 ht[1] */
    ht = dictIsRehashing(d) ? &d->ht[1] : &d->ht[0];
    entry = zmalloc(sizeof(*entry));
    entry->next = ht->table[index];
    ht->table[index] = entry;
    ht->used++;

    dictSetKey(d, entry, key); /* 添加 Key */
    return entry;
}
```

## dictDelete()
dictDelete() 函数直接调用 dictGenericDelete() 函数从 dict 中删除一个 Key-Value 数据，并将删除的 dictEntry 返回。
```
// dict.c
int dictDelete(dict *ht, const void *key) {
    return dictGenericDelete(ht,key,0) ? DICT_OK : DICT_ERR;
}
```
dictGenericDelete() 将 Key 对应的 dictEntry 从 ht 中删除。由于 dict 使用链地址法处理冲突，实际逻辑是链表的删除。
```
// dict.c
static dictEntry *dictGenericDelete(dict *d, const void *key, int nofree) {
    uint64_t h, idx;
    dictEntry *he, *prevHe;
    int table;

    if (d->ht[0].used == 0 && d->ht[1].used == 0) return NULL;

    if (dictIsRehashing(d)) _dictRehashStep(d);
    h = dictHashKey(d, key);

    for (table = 0; table <= 1; table++) {
        idx = h & d->ht[table].sizemask;
        he = d->ht[table].table[idx];
        prevHe = NULL;
        while(he) {
            if (key==he->key || dictCompareKeys(d, key, he->key)) {
                /* Unlink the element from the list */
                if (prevHe)
                    prevHe->next = he->next;
                else
                    d->ht[table].table[idx] = he->next;
                if (!nofree) {
                    dictFreeKey(d, he);
                    dictFreeVal(d, he);
                    zfree(he);
                }
                d->ht[table].used--;
                return he;
            }
            prevHe = he;
            he = he->next;
        }
        if (!dictIsRehashing(d)) break;
    }
    return NULL; /* not found */
}
```

## dictFind()
dictFind() 函数用于在 dict 中查找某个 Key-Value，如果 dict 正在再哈希，dictFind() 函数返回 NULL。
```
// dict.c
dictEntry *dictFind(dict *d, const void *key)
{
    dictEntry *he;
    uint64_t h, idx, table;

    if (d->ht[0].used + d->ht[1].used == 0) return NULL; /* dict is empty */
    if (dictIsRehashing(d)) _dictRehashStep(d);
    h = dictHashKey(d, key);
    for (table = 0; table <= 1; table++) {
        idx = h & d->ht[table].sizemask;
        he = d->ht[table].table[idx];
        while(he) {
            if (key==he->key || dictCompareKeys(d, key, he->key))
                return he;
            he = he->next;
        }
        if (!dictIsRehashing(d)) return NULL;
    }
    return NULL;
}
```