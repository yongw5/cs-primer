## 定义 <array>
```
template <
    class T,
    class Allocator = std::allocator<T>
> class list;
```

## 成员类型
|类型|定义|
|:-|:-|
|value_type|T|
|allocate_type|Allocator|
|size_type|std::size_t|
|difference_type|std::ptrdiff_t|
|reference|value_type&|
|const_reference|const value_type&|
|pointer|value_type*|
|const pointer|const value_type*|
|iterator|LegacyBidirectionalIterator|
|const_iterator|const LegacyBidirectionalIterator|
|reverse_iterator|std::reverse_iterator<iterator>|
|const_reverse_iterator|std::reverse_iterator<const_iterator>|

## 构造和析构函数
```
list();
explicit list( const Allocator& alloc );
explicit list( size_type count,
               const T& value = T(),
               const Allocator& alloc = Allocator());
         list( size_type count,
               const T& value,
               const Allocator& alloc = Allocator());
explicit list( size_type count );
explicit list( size_type count, const Allocator& alloc = Allocator() );

template< class InputIt >
list( InputIt first, InputIt last,
      const Allocator& alloc = Allocator() );
list( const list& other );
list( const list& other, const Allocator& alloc );
list( list&& other );
list( list&& other, const Allocator& alloc );
list( std::initializer_list<T> init,
      const Allocator& alloc = Allocator() );

~list();
```
|函数|定义|
|:-|:-|
|operator=|赋值|
|assign|赋值|
|get_allocator|返回 Allocator|

## 成员访问函数
|函数|定义|
|:-|:-|
|front|第一个元素|
|back|最后一个元素|

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
|erase|删除元素|
|push_back|在尾部插入|
|emplace_back|在尾部构造|
|pop_back|删除尾部元素|
|push_front|在首部插入|
|pop_front|在首部删除|
|resize|改变容量大小|
|swap|和其他 vector 交换内容|

## 操作
|函数|定义|
|:-|:-|
|merge|合并两个 list|
|splice|从其他 list 移动元素|
|remove<br>remove_if|移除元素|
|reverse|逆序|
|unique|去重|
|sort|排序|

## 非成员函数
|函数|定义|
|:-|:-|
|operator==|逻辑判断|
|std::swap(std::array)|交换两个 array 内容|

TODO(用法)