## 定义 <queue>
```
template <
    class T,
    class Container = std::deque<T>
> class queue;
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
queue() : queue(Container()) { }
explicit queue( const Container& cont = Container() );
explicit queue( const Container& cont );
explicit queue( Container&& cont );
queue( const queue& other );
queue( queue&& other );
template< class Alloc >
explicit queue( const Alloc& alloc );
template< class Alloc >
queue( const Container& cont, const Alloc& alloc );
template< class Alloc >
queue( Container&& cont, const Alloc& alloc );
template< class Alloc >
queue( const queue& other, const Alloc& alloc );
template< class Alloc >
queue( queue&& other, const Alloc& alloc );

~queue()
```
|函数|定义|
|:-|:-|
|operator=|赋值|

## 成员访问函数
|函数|定义|
|:-|:-|
|front|队首元素|
|back|队尾元素|

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