## std::initlizer_list
std::initializer_list<T> 类型的对象是一个轻量级代理对象，它提供对常量类型对象数组的访问。

在以下情况会自动构造 std::initializer_list 对象
1. 用花括号 {} 的方式初始化一个对象且该对象有接受 std::initializer_list 参数的构造函数
2. 用花括号 {} 的方式给一个对象赋值或者函数返回且该对象重载了接受 std::initializer_list 参数的赋值运算符
3. 在 Range-based for loop 用于指定范围

其底层实现是一个数组。
```
/// @file initializer_list
46   template<class _E>
47     class initializer_list
48     {
49     public:
50       typedef _E           value_type;
51       typedef const _E&    reference;
52       typedef const _E&    const_reference;
53       typedef size_t               size_type;
54       typedef const _E*    iterator;
55       typedef const _E*    const_iterator;
56 
57     private:
58       iterator                     _M_array;
59       size_type                    _M_len;
60 
61       // The compiler can call a private constructor.
62       constexpr initializer_list(const_iterator __a, size_type __l)
63       : _M_array(__a), _M_len(__l) { }
64 
65     public:
66       constexpr initializer_list() noexcept
67       : _M_array(0), _M_len(0) { }
```
主要由编译器调用。主要有 begin()、end() 和 size() 函数接口
```
/// @file initializer_list
69       // Number of elements.
70       constexpr size_type
71       size() const noexcept { return _M_len; }
72 
73       // First element.
74       constexpr const_iterator
75       begin() const noexcept { return _M_array; }
76 
77       // One past the last element.
78       constexpr const_iterator
79       end() const noexcept { return begin() + size(); }
80     };
```