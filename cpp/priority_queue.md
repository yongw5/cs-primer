## 定义 <priority_queue>
```
template <
    class T,
    class Container = std::deque<T>
    clas Compare = std::less<typename Container::value_type>
> class priority_queue;
```

## 成员类型
|类型|定义|
|:-|:-|
|container_type|Container|
|value_type|T|
|size_type|std::size_t|
|reference|value_type&|
|const_reference|const value_type&|

## 构造和析构函数
```
priority_queue() : priority_queue(Compare(), Container()) { }
explicit priority_queue(const Compare& compare)
    : priority_queue(compare, Container()) { }
explicit priority_queue( const Compare& compare = Compare(),
                         const Container& cont = Container() );
priority_queue( const Compare& compare, const Container& cont );
priority_queue( const Compare& compare, Container&& cont );
priority_queue( const priority_queue& other );
priority_queue( priority_queue&& other );
template< class Alloc >
explicit priority_queue( const Alloc& alloc );
template< class Alloc >
priority_queue( const Compare& compare, const Alloc& alloc );
template< class Alloc >
priority_queue( const Compare& compare, const Container& cont,
                const Alloc& alloc );
template< class Alloc >
priority_queue( const Compare& compare, Container&& cont,
                const Alloc& alloc );
template< class Alloc >
priority_queue( const priority_queue& other, const Alloc& alloc );
template< class Alloc >
priority_queue( priority_queue&& other, const Alloc& alloc );
template< class InputIt >
priority_queue( InputIt first, InputIt last,
                const Compare& compare, const Container& cont );
template< class InputIt >
priority_queue( InputIt first, InputIt last,
                const Compare& compare = Compare(),
                Container&& cont = Container() );

~priority_queue()
```
|函数|定义|
|:-|:-|
|operator=|赋值|

## 成员访问函数
|函数|定义|
|:-|:-|
|top|队首元素|

## 容量
|函数|定义|
|:-|:-|
|empty|是否为空|
|size|元素个数|

## 修改
|函数|定义|
|:-|:-|
|push|入队|
|emplace|入队|
|pop|出队|
|swap|和其他 queue 交换内容|

## 非成员函数
|函数|定义|
|:-|:-|
|operator==|逻辑判断|
|std::swap(std::array)|交换两个 array 内容|

TODO(用法)