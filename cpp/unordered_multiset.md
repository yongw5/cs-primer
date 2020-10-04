## 定义 <unordered_set>
```
template<
    class Key,
    class Hash = std::hash<Key>,
    class KeyEqual = std::equal_to<Key>,
    class Allocator = std::allocator<Key>
> class unordered_multiset;
```

## 成员类型
|类型|定义|
|:-|:-|
|key_type|Key|
|value_type|std::pair<const Key, T>|
|size_type|std::size_t|
|difference_type|std::ptrdiff_t|
|hasher|Hash|
|key_equal|KeyEqual|
|allocate_type|Allocator|
|reference|value_type&|
|const_reference|const value_type|
|pointer|std::alocator_traits<Allocator>::pointer|
|const pointer|std::alocator_traits<Allocator>::const_pointer|
|iterator|LegacyForwardIterator|
|const_iterator|const LegacyForwardIterator|
|local_iterator|TODO|
|const_local_iterator|TODO|

## 构造和析构函数
```
unordered_multiset() : unordered_multiset( size_type(/*implementation-defined*/) ) {}
explicit unordered_multiset( size_type bucket_count,
                             const Hash& hash = Hash(),
                             const key_equal& equal = key_equal(),
                             const Allocator& alloc = Allocator() );
explicit unordered_multiset( const Allocator& alloc );
template< class InputIt >
unordered_multiset( InputIt first, InputIt last,
                    size_type bucket_count = /*implementation-defined*/,
                    const Hash& hash = Hash(),
                    const key_equal& equal = key_equal(),
                    const Allocator& alloc = Allocator() );
unordered_multiset( const unordered_multiset& other );
unordered_multiset( const unordered_multiset& other, const Allocator& alloc );
unordered_multiset( unordered_multiset&& other );
unordered_multiset( unordered_multiset&& other, const Allocator& alloc );
unordered_multiset( std::initializer_list<value_type> init,
                    size_type bucket_count = /*implementation-defined*/,
                    const Hash& hash = Hash(),
                    const key_equal& equal = key_equal(),
                    const Allocator& alloc = Allocator() );

~unordered_multiset()
```
|函数|定义|
|:-|:-|
|operator=|赋值|
|get_allocator|返回 Allocator|

## 迭代器
|函数|定义|
|:-|:-|
|begin<br>cbegin()|返回指向首元素的迭代器|
|end<br>cend()|返回指向末端的迭代器|

## 容量
|函数|定义|
|:-|:-|
|empty|是否为空|
|size|元素个数|
|max_size|最多容纳元素|

## 修改
|函数|定义|
|:-|:-|
|clear|清空|
|insert|插入元素|
|emplace|就地构造一个元素|
|emplace_hint|就地构造一个元素|
|erase|删除元素|
|swap|和其他 map 交换内容|

## 查找
|函数|定义|
|:-|:-|
|count|返回等于指定 key 的元素个数|
|find|根据 key 查找|
|equal_range|等于指定 key 的元素范围|

## bucket 接口
|函数|定义|
|:-|:-|
|begin(size_type)<br>cbegin(size_type)|返回指向桶头部的迭代器|
|end(size_type)<br>cend(size_type)|返回指向桶尾部的迭代器|
|bucket_count|桶的个数|
|max_bucket_count|全部桶的个数|
|bucket_size|某个桶元素的个数|
|bucket|返回桶|

## 哈希策略
|函数|定义|
|:-|:-|
|load_factor|每个桶平均元素个数|
|max_load_factor|平均每个桶最多元素个数|
|rehash||
|reserve||

## 非成员函数
|函数|定义|
|:-|:-|
|operator==|逻辑判断|
|std::swap(std::array)|交换两个 array 内容|

TODO(用法)