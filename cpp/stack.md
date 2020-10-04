## 定义 <array>
```
template <
    class T,
    class Container = std::deque<T>
> class stack;
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
stack() : stack(Container()) { }
explicit stack( const Container& cont = Container() );
explicit stack( const Container& cont );
explicit stack( Container&& cont );
stack( const stack& other );
stack( stack&& other );
template< class Alloc >
explicit stack( const Alloc& alloc );
template< class Alloc >
stack( const Container& cont, const Alloc& alloc );
template< class Alloc >
stack( Container&& cont, const Alloc& alloc );
template< class Alloc >
stack( const stack& other, const Alloc& alloc );
template< class Alloc >
stack( stack&& other, const Alloc& alloc );

~stack()
```
|函数|定义|
|:-|:-|
|operator=|赋值|

## 成员访问函数
|函数|定义|
|:-|:-|
|top|栈顶元素|

## 容量
|函数|定义|
|:-|:-|
|empty|是否为空|
|size|元素个数|

## 修改
|函数|定义|
|:-|:-|
|push|入栈|
|emplace|入栈|
|pop|出栈|
|swap|和其他 stack 交换内容|

## 非成员函数
|函数|定义|
|:-|:-|
|operator==|逻辑判断|
|std::swap(std::array)|交换两个 array 内容|

TODO(用法)