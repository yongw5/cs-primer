## array(C++11)
对C语言式数组的封装，定义时指定长度，定义后不能更改长度。与内置数据不同的是，数组名不是指向首元素地址。
```
/// @file include/array
80   template<typename _Tp, std::size_t _Nm>
81     struct array
82     {
83       typedef _Tp                                value_type;
84       typedef value_type*                        pointer;
85       typedef const value_type*                       const_pointer;
86       typedef value_type&                                reference;
87       typedef const value_type&                          const_reference;
88       typedef value_type*                                iterator;
89       typedef const value_type*                          const_iterator;
90       typedef std::size_t                                size_type;
91       typedef std::ptrdiff_t                             difference_type;
92       typedef std::reverse_iterator<iterator>            reverse_iterator;
93       typedef std::reverse_iterator<const_iterator>   const_reverse_iterator;
94 
95       // Support for zero-sized arrays mandatory.
96       typedef _GLIBCXX_STD_C::__array_traits<_Tp, _Nm> _AT_Type;
97       typename _AT_Type::_Type                         _M_elems;
```