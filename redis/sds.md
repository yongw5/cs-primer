## sds
Redis 利用简单动态数组（Simple Dynamic String，SDS）实现实现了自己的字符串（遵循 C 库字符串以 '\0' 结尾的规定）。SDS 也就是柔性数组，其定义如下：
```
// shs.h
struct __attribute__ ((__packed__)) sdshdr5 {
    unsigned char flags;   /* 3 lsb of type, and 5 msb of string length */
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr8 {
    uint8_t len;           /* used */
    uint8_t alloc;         /* excluding the header and null terminator */
    unsigned char flags;   /* 3 lsb of type, 5 unused bits */
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr16 {
    uint16_t len;         /* used */
    uint16_t alloc;       /* excluding the header and null terminator */
    unsigned char flags;  /* 3 lsb of type, 5 unused bits */
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr32 {
    uint32_t len;         /* used */
    uint32_t alloc;       /* excluding the header and null terminator */
    unsigned char flags;  /* 3 lsb of type, 5 unused bits */
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr64 {
    uint64_t len;         /* used */
    uint64_t alloc;       /* excluding the header and null terminator */
    unsigned char flags;  /* 3 lsb of type, 5 unused bits */
    char buf[];
};

#define SDS_TYPE_5  0
#define SDS_TYPE_8  1
#define SDS_TYPE_16 2
#define SDS_TYPE_32 3
#define SDS_TYPE_64 4
#define SDS_TYPE_MASK 7
#define SDS_TYPE_BITS 3
```
`__attribute__ ((__packed__))` 是禁止编译器进行字节对齐，结构体中各成员变量紧凑排列。sdshdr# 各成员含义如下：
- len：字符串长度（不包含 '\0' 字符）；
- alloc：可用空间大小（不含头部以及 '\0 字符）；
- flags：标志位，低三位表示 sdshdr 类型，其他位未使用；
- buf：指向字符串存储空间；

柔性数组在使用时，根据字符串的长度（或者目标字符串长度）动态分配内存。例如，将 Redis 设置为 Key 时
```
char redis[] = "Redis";
uint8_t size = sizeof(struct sdshdr8) + strlen(redis) + 1;
struct sdshdr8 *key = (struct sdshdr8*)(malloc(size));
key->len = strlen(redis);
key->alloc = 0;
memcpy(key->buf, redis, sizeof(redis));
```
在 Redis，不需要这样底层的操作，sds.h 中提供了很多函数，封装 sdshdr# 赋值、复制和裁剪等操作。

为了避免字符串操作过程中频繁地进行内存分配和释放，Redis 在分配内存空间时，会额外分配一部分内存供将来使用，其数量按照以下规则计算：
- 如果对 sds 修改后，其长度小于 1MB，额外分配 len 个字节的空间。例如，修改后字符串长度为 13，则额外分配 13 字节的内存，buf 总共 27 字节。
- 如果对 sds 修改后，其长度大于等于 1MB，额外分配 1MB 内存空间。例如修改后 30 MB，则总共分配 30MB + 1 MB + 1byte 的内存。

另外，对于 sds 内存的回收，采用延迟处理。当 sds 中字符串被裁剪或者被一个更短的字符串替换时，并不会立即重新分配内存，将原来有空余的内存进行回收，而是保留以供将来使用。

另外，sds 并不仅仅将 buf 当作字符串处理，而是当作二进制数据处理。Redis 不会 sds 中的数据做任何限制、过滤或者假设，不仅可以保存文本数据，还可以保存任意格式的二进制数据。