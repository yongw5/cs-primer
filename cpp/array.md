## 定义 <array>
```
template <
    class T, 
    std::size_t N
> sruct array;
```

## 成员类型
|类型|定义|
|:-|:-|
|value_type|T|
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
|函数|定义|
|:-|:-|
|(constructor)|(implicitly declared)|
|(destructor)|(implicitly declared)|
|operator=|(implicitly declared)|

## 成员访问函数
|函数|定义|
|:-|:-|
|at|会执行边界检查|
|operator[]|不会进行边界检查|
|front|第一个元素|
|back|最后一个元素|
|data|返回数组起始地址|

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

## 操作
|函数|定义|
|:-|:-|
|fill|用指定值填充空间|
|swap|和其他 array 交换内容|

## 非成员函数
|函数|定义|
|:-|:-|
|operator==|逻辑判断|
|std::get(std::array)|访问元素 get<0>(arr) = 1|
|std::swap(std::array)|交换两个 array 内容|

TODO(用法)