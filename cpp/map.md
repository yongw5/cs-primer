## 定义 <map>
```
template<
    class Key,
    class T,
    class Compare = std::less<Key>,
    class Allocator = std::allocator<std::pair<const Key, T> >
> class map;
```

## 成员类型
|类型|定义|
|:-|:-|
|key_type|Key|
|mapped_type|T|
|value_type|std::pair<const Key, T>|
|size_type|std::size_t|
|difference_type|std::ptrdiff_t|
|key_compare|Compare|
|allocate_type|Allocator|
|reference|value_type&|
|const_reference|const value_type|
|pointer|std::alocator_traits<Allocator>::pointer|
|const pointer|std::alocator_traits<Allocator>::const_pointer|
|iterator|LegacyBidirectionalIterator|
|const_iterator|const LegacyBidirectionalIterator|
|reverse_iterator|std::reverse_iterator<iterator>|
|const_reverse_iterator|std::reverse_iterator<const_iterator>|

## 成员类
value_compare：比较 value_type 对象

## 构造和析构函数
```
map();
explicit map( const Compare& comp,
              const Allocator& alloc = Allocator() );
explicit map( const Allocator& alloc );
template< class InputIt >
map( InputIt first, InputIt last,
     const Compare& comp = Compare(),
     const Allocator& alloc = Allocator() );
map( const map& other );
map( const map& other, const Allocator& alloc );
map( map&& other );
map( map&& other, const Allocator& alloc );
map( std::initializer_list<value_type> init,
     const Compare& comp = Compare(),
     const Allocator& alloc = Allocator() );

~map()
```
|函数|定义|
|:-|:-|
|operator=|赋值|
|get_allocator|返回 Allocator|

## 成员访问函数
|函数|定义|
|:-|:-|
|at|会执行边界检查|
|operator[]|访问或者插入元素|

## 迭代器
|函数|定义|
|:-|:-|
|begin<br>cbegin()|返回指向首元素的迭代器|
|end<br>cend()|返回指向末端的迭代器|
|rbgein<br>crbegin()|返回指向逆序首元素的迭代器|
|rend()<br>crend()|返回指向逆序末端的迭代器|

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
|lower_bound|第一个不小于指定 key 的元素|
|upper_bound|第一个大于指定 key 的元素|

## 非成员函数
|函数|定义|
|:-|:-|
|operator==|逻辑判断|
|std::swap(std::array)|交换两个 array 内容|

TODO(用法)