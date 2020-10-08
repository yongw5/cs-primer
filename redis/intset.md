## intset
inset 是 Redis 用于保存整数值的抽象数据结构，它可以保存类型为 int16_t、int32_t 和 int64_t 的整数值，并且保证集合中不会出现重复元素。其定义如下：
```
typedef struct intset {
    uint32_t encoding;
    uint32_t length;
    int8_t contents[];
} intset;
```
其中 encoding 字段指示 contents 数组的元素类型，具有如下三个值：INTSET_ENC_INT16、INTSET_ENC_INT32 和INTSET_ENC_INT64，分别表示 contents 的数据元素为 int16_t、int32_t 和 int64_t。而 length 字段表示集合的大小。

在整数集合上插入和删除元素，会引起整数数据的升级或者降级。比如新元素类型比整数集合现有所有的类型都要长时，整数集合需要先进行升级，然后才能将新元素添加到整数集合中。升级的步骤如下：
1. 根据新元素的类型，扩展 contents 数组的空间；
2. 将原来的元素都转换成新元素类型并放到新空间的正确位置上（保持有序性）；
3. 将新元素添加到 contents 数组。

降级具有类似的操作过程。