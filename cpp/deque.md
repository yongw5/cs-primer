## 定义 <deque>
```
template <
    class T,
    class Allocator = std::allocator<T>
> class deque;
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
|iterator|LegacyRandomAccessIterator|
|const_iterator|const LegacyRandomAccessIterator|
|reverse_iterator|std::reverse_iterator<iterator>|
|const_reverse_iterator|std::reverse_iterator<const_iterator>|

## 构造和析构函数
```
deque();
explicit deque( const Allocator& alloc );
explicit deque( size_type count,
                const T& value = T(),
                const Allocator& alloc = Allocator());
         deque( size_type count,
                const T& value,
                const Allocator& alloc = Allocator());
explicit deque( size_type count );
explicit deque( size_type count, const Allocator& alloc = Allocator() );
template< class InputIt >
deque( InputIt first, InputIt last,
       const Allocator& alloc = Allocator() );
deque( const deque& other );
deque( const deque& other, const Allocator& alloc );
deque( deque&& other );
deque( deque&& other, const Allocator& alloc );
deque( std::initializer_list<T> init,
       const Allocator& alloc = Allocator() );

~deque();
```
|函数|定义|
|:-|:-|
|operator=|赋值|
|assign|赋值|
|get_allocator|返回 Allocator|

## 成员访问函数
|函数|定义|
|:-|:-|
|at|会执行边界检查|
|operator[]|不会进行边界检查|
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
|shrink_to_fit|收缩|

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
|emplace_front|在首部构造|
|pop_front|删除首元素|
|resize|改变容量大小|
|swap|和其他 vector 交换内容|

## 非成员函数
|函数|定义|
|:-|:-|
|operator==|逻辑判断|
|std::swap(std::array)|交换两个 array 内容|

## 会造成迭代器失效的操作
|操作|失效|
|:-|:-|
|swap<br>std::swap|end()|
|shrink_to_fit<br>clear<br>insert<br>emplace<br>push_front, push_back<br>emplace_front<br>emplace_back|总是|
|erase|如果在头部删除，只是删除的元素<br>如果在尾部删除，只有删除的元素和 end()<br>否则，全部|
|push_back<br>emplace_back|如果扩容，全部；否则只有 end()|
|insert<br>emplace|如果扩容，全部；否则插入点之后的所有元素，包括 end()|
|resize|如果扩容，全部<br>如果缩小，删除的元素和 end()<br>否则，所有|
|pop_front|删除的元素|
|pop_back|删除元素及其 end()|

TODO(用法)