## ziplist
ziplist 是 Redis 为了节约内存而开发的、由一系列特殊编码的连续内存块组成的数据结构。一个 ziplist 可以包含任意多个结点，每个结点可以保存一个字节数组或者一个整数值，其布局如下：
```
<zlbytes> <zltail> <zllen> <entry> <entry> ... <entry> <zlend>
```
各部分（如果没有明确指定，都采用小端字节序存储）含义如下：

|字段|数据类型|含义|
|:-:|:-:|:-|
|zlbytes|uint32_t|整个 ziplist 字节数|
|zltail|uint32_t|最后一个 entry 的偏移|
|zllen|uint16_t|entry 数量，如果为 UINT16_MAX，需要通过遍历才能计算 entry 数量|
|extryX|-|ziplit 的单个结点|
|zlend|uint8_t|特殊值 0xFF，标记 ziplist 结束|

ziplist 中每个 entry 都包含固定长度的元数据（metadata），记录了两个必要信息，一是前一个 entry 的长度以方便逆向遍历；另一个是当前 entry 的编码，指示 entry 的类型，整数或者字符串（记录长度）。因此，entry 布局如下：
```
<prevlen> <encoding> <entry-data>
```
不过，有的时候，encoding 中已经记录了 entry 本身，此时 entry-data 可以不用，如下所示：
```
<prevlen> <encoding>
```

prevlen 并不是简单地用一个整数记录前一个 entry 的长度，而是采用如下方式：
- 如果前一个 entry 的长度小于 254 字节，用一个 uint8_t 表示；
- 否则，采用 1B + 4B 表示。其中，前 1B 设置固定值 0xFE，指示真正的长度值在后面四个字节；

因此，更正确地说，一个 entry 通常如下所示：
```
<prevlen from 0 to 253> <encoding> <entry>
```
或者
```
0xFE <4 bytes unsigned little endian prevlen> <encoding> <entry>
```
encoding 字段和 entry 的内容有关，编码规则如下：

|编码|长度|含义|
|:-|:-:|:-|
|00[6bit]|1B|长度小于或者等于 63 字节的字符串，其中后 6 位 pppppp 表示字符串长度|
|01[14bit]|2B|长度小于等于 16363 字节的字符串，其中后 14 位表示字符串长度（大端字）|
|10000000[32bit]|5B|长度大于等于 16384 字节的字符串，其中后 32 位表示字符串长度（大端序）|
|11000000|1B|int16_t 类型整数|
|11010000|1B|int32_t 类型整数|
|11100000|1B|int64_t 类型整数|
|11110000|1B|24 位有符号整数|
|11111110|1B|8 位有符号整数|
|1111[4bit]|1B|后 4 位表示二进制 0000 到 1101，表示 从 0 到 12 的 4 位无符号整数（减 1）|
|1111111|-|结束符|

在 ziplist 中添加或者删除 entry，可能触发连锁更新。比如某个 ziplist 中所有 entry 的长度都是 253 字节，则每个 entry 中 prevlen 字段只需要 1B 就可以。此时如果在某个位置（不是最后）添加一个 254 字节的 entry，则后续 entry prevlen 需要 5B。并且这种影响会传递，影响到后面的 entry。同理，删除某个 entry 也会引起连锁更新。
