## zset 
zset 表示有序集合，其定义如下：
```
// server.h
typedef struct zset {
    dict *dict;
    zskiplist *zsl;
} zset;
```
可以看到，zset 需要借助 dict 和 zskiplist 数据结构来实现，dict 实现元素唯一，而 zskiplist 保持元素有序。zskiplist 表示跳表，其定义如下。
```
// server.h
typedef struct zskiplistNode {
    sds ele;
    double score;
    struct zskiplistNode *backward;
    struct zskiplistLevel {
        struct zskiplistNode *forward;
        unsigned long span;
    } level[];
} zskiplistNode;

typedef struct zskiplist {
    struct zskiplistNode *header, *tail;
    unsigned long length;
    int level;
} zskiplist;
```
