## adlist
adlist 是 Redis 双向链表的实现。adlist 在 Redis 中的应用非常广泛，比如列表的底层实现之一就是链表。另外，发布、订阅、慢查询以及监视器等功能也用到了链表，Redis 服务器本身还使用链表来保存多个客户端的状态信息，以及使用链表来构建客户端输出缓冲区。adlist 的定义如下：
```
// adlist.h
typedef struct listNode {
    struct listNode *prev;
    struct listNode *next;
    void *value;
} listNode;

typedef struct listIter {
    listNode *next;
    int direction;
} listIter;

typedef struct list {
    listNode *head;
    listNode *tail;
    void *(*dup)(void *ptr); // 复制结点
    void (*free)(void *ptr); // 释放结点
    int (*match)(void *ptr, void *key); // 匹配
    unsigned long len;
} list;
```