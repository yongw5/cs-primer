## quicklist
quicklist 是双向链表和 ziplist 的结合体，quicklist 中每个结点 quicklistNode 都指向一个 ziplist。而且，quicklistNode 仅仅用 32 字节（64 位系统）就表征了一个 ziplist。其中 zl 指向 ziplist 或者 ziplist 压缩后的 quicklistLZF 数据结构。不过无论 zl 指向何种数据，sz 都是 ziplist 数据的长度。
```
typedef struct quicklistNode {
    struct quicklistNode *prev;
    struct quicklistNode *next;
    unsigned char *zl;           // 指向 ziplist 或 quicklistLZF
    unsigned int sz;             // ziplist 数据字节数（未压缩）
    unsigned int count : 16;     // ziplist 中元素个数
    unsigned int encoding : 2;   // RAW==1 or LZF==2
    unsigned int container : 2;  // NONE==1 or ZIPLIST==2
    unsigned int recompress : 1; // 是否解压缩
    unsigned int attempted_compress : 1; // 测试使用
    unsigned int extra : 10;     // 保留，未来使用
} quicklistNode;

typedef struct quicklistLZF { // ziplist 压缩后数据结构
    unsigned int sz;          // LZF 字节数
    char compressed[];        // LZF 数据
} quicklistLZF;
```
quicklistBookmark 

只有在数千个节点的多余内存使用量可以忽略不计的情况下，才应将它们用于非常大的列表，并且确实需要分批迭代它们。当不使用它们时，它们不会增加任何内存开销，但是当使用然后删除它们时，会保留一些开销（以避免共振）。 使用的书签数量应保持最小，因为这还会增加节点删除（搜索要更新的书签）的开销。
```
typedef struct quicklistBookmark {
    quicklistNode *node;
    char *name;
} quicklistBookmark;

typedef struct quicklist { // 定长 40 字节
    quicklistNode *head;
    quicklistNode *tail;
    unsigned long count;        // 所有 ziplist 中元素数目
    unsigned long len;          // quicklistNode 数量
    int fill : QL_FILL_BITS;    // 16 位 fill factor for individual nodes
    unsigned int compress : QL_COMP_BITS; // 16 位，quicklist 末尾不压缩结点的数量
    unsigned int bookmark_count: QL_BM_BITS; // 4 位，bookmarks 长度
    quicklistBookmark bookmarks[];
} quicklist;

typedef struct quicklistIter {
    const quicklist *quicklist;
    quicklistNode *current;
    unsigned char *zi;
    long offset; /* offset in current ziplist */
    int direction;
} quicklistIter;

typedef struct quicklistEntry {
    const quicklist *quicklist;
    quicklistNode *node;
    unsigned char *zi;
    unsigned char *value;
    long long longval;
    unsigned int sz;
    int offset;
} quicklistEntry;
```