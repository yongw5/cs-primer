## zipmap
zipmap 用于实现 (string, string) 字符串对，Redis 中并没有对应的数据结构。zipmap 利用一段地址连续的内存保存 (string, string) 字符串对，通过遍历的方式（O(n) 复杂度）查找目标字符串对。Redis哈希类型使用此数据结构处理由少量元素组成的哈希，一旦达到给定数量的元素，便切换到哈希表。鉴于很多时候使用 Redis 哈希来表示由几个字段组成的对象，因此使用 zipmap 可以节省内存。

例如，保存了字符串对 "foo" => "bar"，"hello" => "world" 的 zipmap 内存布局如下：
```
<zmlen><len>"foo"<len><free>"bar"<len>"hello"<len><free>"world"<zmend>
```
其中：
- zmlen 固定长度，1 个字节，表示 zipmap 长度（元素个数）。当 zipmap 总长超过 254，zmlen 填入 0xFE，表示需要遍历才能获取 zipmap 长度。
- len 表示后续字符串的长度，占用 1 个字节或者 5 个字节。如果字符串长度在 [0, 253] 范围，用 1 个字节表示；否则，用 5 个字节表示，即 <0xFExxx>，xxx 是 4 字节的长度值，按照宿主机字节序排列。
- free 表示一部分未使用的内存，是修改 Key 或者 Value 的结果。free 总是只有 1 个字节，因为如果某个修改操作使得某个 Key-Value 中空闲空间超过 1 个字节，zipmap 就会重新分配内存，保证 zipmap 尽可能紧凑。
- zmend 表示 zipmap 结束，固定一个字节，填入 0xFF 值。