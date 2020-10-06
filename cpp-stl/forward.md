## std::forward()
是为了支持 C++11 转发。某些函数需要将其一个或多个实参连同类型不变地转发给其他函数，在此情况下，我们需要保持被转发实参的所有性质，包括实参类型是否是 const 的以及实参是左值还是右值。需要注意的是，std::forward() 必须通过显式模板实参来调用。std::forward() 返回该显式实参类型的右值引用。通过其返回类型上的引用折叠，std::forward() 可以保持给定实参的左值/右值属性。
```
/// @file include/bits/move.h
74   template<typename _Tp>
75     constexpr _Tp&&
76     forward(typename std::remove_reference<_Tp>::type& __t) noexcept
77     { return static_cast<_Tp&&>(__t); }

85   template<typename _Tp>
86     constexpr _Tp&&
87     forward(typename std::remove_reference<_Tp>::type&& __t) noexcept
88     {
89       static_assert(!std::is_lvalue_reference<_Tp>::value, "template argument"
90             " substituting _Tp is an lvalue reference type");
91       return static_cast<_Tp&&>(__t);
92     }
```