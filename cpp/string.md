## 定义 <string>
```
template <
    class CharT,
    class Traits = std::char_traits<CharT>
    class Allocator = std::allocator<T>
> class basic_string;
```

## 成员类型
|类型|定义|
|:-|:-|
|traints_type|Traits|
|value_type|CharT|
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
basic_string();
explicit basic_string( const Allocator& alloc );
basic_string( size_type count,
              CharT ch,
              const Allocator& alloc = Allocator() );
basic_string( const basic_string& other,
              size_type pos,
              size_type count = std::basic_string::npos,
              const Allocator& alloc = Allocator() );
basic_string( const CharT* s,
              size_type count,
              const Allocator& alloc = Allocator() );
basic_string( const CharT* s,
              const Allocator& alloc = Allocator() );
template< class InputIt >
basic_string( InputIt first, InputIt last,
              const Allocator& alloc = Allocator() );
basic_string( const basic_string& other );
basic_string( const basic_string& other, const Allocator& alloc );
basic_string( basic_string&& other ) noexcept;
basic_string( basic_string&& other, const Allocator& alloc );
basic_string( std::initializer_list<CharT> ilist,
              const Allocator& alloc = Allocator() );

~basic_string();
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
|data|返回数组起始地址|
|c_str|返回 C 字符串|

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
|size<br>length|元素个数|
|max_size|最多容纳元素|
|reserve|预分配内存，不构造|
|capacity|容量|
|shrink_to_fit|收缩|

## 修改
|函数|定义|
|:-|:-|
|clear|清空|
|insert|插入元素|
|erase|删除元素|
|push_back|在尾部插入|
|pop_back|删除尾部元素|
|append|在尾部添加|
|operator+|尾部添加|
|compare|和其他 string 比较|
|replace|替换|
|substr|子串|
|copy|拷贝|
|resize|改变容量大小|
|swap|和其他 vector 交换内容|

## 查找
|函数|定义|
|:-|:-|
|find|查找字符|
|rfind|逆向查找|
|find_first_of|查找首次出现的字符|
|find_first_not_of|查找首个不是字符|
|find_last_of|查找最后出现的字符|
|find_last_not_of|查找最后不是某个的字符|

## 常量
npos 非法位置

## 非成员函数
|函数|定义|
|:-|:-|
|operator+|字符串拼接|
|operator==|逻辑判断|
|std::swap(std::array)|交换两个 array 内容|

## 输入输出
|函数|定义|
|:-|:-|
|operator<<<br>operator>>|流输入或者输出到流|
|getline|从 IO 流中获取一行|

## 数值转换
|函数|定义|
|:-|:-|
|stoi<br>stol<stoul>stoll|字符串转换成整型|
|stoul<stoul>stoull|字符串转换成无符号整型|
|stof<br>stod<br>stold|字符串转换成浮点数|
|to_string|整型或者字符串转换成字符串|

TODO(用法)