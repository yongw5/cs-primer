## redisObject 初探
为了支持不同的数据类型以及更有效地利用内存，Redis 实现了不同的基本数据结构（后面介绍）。但是 Redis 一般不会直接使用这些基本的数据结构，而是将他们统一封装成 redisObject。redisObject 定义如下：
```
// server.h
typedef struct redisObject {
    unsigned type:4;
    unsigned encoding:4;
    unsigned lru:LRU_BITS;
    int refcount;
    void *ptr;
} robj;
```
其中:
- type：记录对象的类型
- encoding：标记 redisObject 底层实现方式
- lru：LRU 时间或者 LFU 数据 
- refcount：引用计数，用于内存回收
- ptr：指向底层实现数据结构

Redis 中，redisObject 有 7 中类型，由如下宏表示：

|宏|值|含义|
|:-|:-:|:-|
|OBJ_STRING|0|字符串对象|
|OBJ_LIST|1|链表对象|
|OBJ_SET|2|集合对象|
|OBJ_ZSET|3|有序集合对象|
|OBJ_HASH|4|哈希表对象|
|OBJ_MODULE|5|Module 对象|
|OBJ_STREAM|6|流对象|

诸如字符串和哈希表之类对象可以用多种基础数据结构实现，redisObject 用 encoding 字段标志底层具体的数据结构。编码方式如下：

|宏|值|含义|
|:-|:-:|:-|
|OBJ_ENCODING_RAW|0|sds|
|OBJ_ENCODING_INT|1|整数类型|
|OBJ_ENCODING_HT|2|哈希表|
|OBJ_ENCODING_ZIPMAP|3|zipmap|
|OBJ_ENCODING_LINKEDLIST|4|已经弃用|
|OBJ_ENCODING_ZIPLIST|5|ziplist|
|OBJ_ENCODING_INTSET|6|intset|
|OBJ_ENCODING_SKIPLIST|7|zset|
|OBJ_ENCODING_EMBSTR|8|embstr 编码字符串|
|OBJ_ENCODING_QUICKLIST|9|quicklist|
|OBJ_ENCODING_STREAM|10|listpack 的基数树|

## getObjectTypeName()
```
char* getObjectTypeName(robj *o) {
    char* type;
    if (o == NULL) {
        type = "none";
    } else {
        switch(o->type) {
        case OBJ_STRING: type = "string"; break;
        case OBJ_LIST: type = "list"; break;
        case OBJ_SET: type = "set"; break;
        case OBJ_ZSET: type = "zset"; break;
        case OBJ_HASH: type = "hash"; break;
        case OBJ_STREAM: type = "stream"; break;
        case OBJ_MODULE: {
            moduleValue *mv = o->ptr;
            type = mv->type->name;
        }; break;
        default: type = "unknown"; break;
        }
    }
    return type;
}
```