## 定义 <array>
```
template <
    class T,
    class Allocator = std::allocator<T>
> class forward_list;
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
|iterator|LegacyForwardIterator|
|const_iterator|const LegacyForwardIterator|

## 构造和析构函数
```
forward_list();
explicit forward_list( const Allocator& alloc );
forward_list( size_type count,
              const T& value,
              const Allocator& alloc = Allocator());
explicit forward_list( size_type count );
template< class InputIt >
forward_list( InputIt first, InputIt last,
              const Allocator& alloc = Allocator() );
forward_list( const forward_list& other );
forward_list( const forward_list& other, const Allocator& alloc );
forward_list( forward_list&& other );
forward_list( forward_list&& other, const Allocator& alloc );
forward_list( std::initializer_list<T> init,
              const Allocator& alloc = Allocator() );

~forward_list();
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

## 迭代器
|函数|定义|
|:-|:-|
|before_begin<br>cbefore_begin()|返回指向头部的迭代器|
|begin<br>cbegin()|返回指向首元素的迭代器|
|end<br>cend()|返回指向末端的迭代器|

## 容量
|函数|定义|
|:-|:-|
|empty|是否为空|
|max_size|最多容纳元素|

## 修改
|函数|定义|
|:-|:-|
|clear|清空|
|insert_after|插入元素|
|emplace_after|就地构造一个元素|
|erase_after|删除元素|
|push_front|在头部插入|
|emplace_front|在头部构造|
|pop_front|删除头部元素|
|resize|改变容量大小|
|swap|和其他 vector 交换内容|

## 操作
|函数|定义|
|:-|:-|
|merge|合并两个 list|
|splice_after|从其他 forward_list 移动元素|
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