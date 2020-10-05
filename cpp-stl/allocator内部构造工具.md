## std::_Construct
底层调用 placement new，在指针所指的内存显式调用构造函数构造对象
```
/// @file bits/stl_construct.h
72   template<typename _T1, typename... _Args>
73     inline void
74     _Construct(_T1* __p, _Args&&... __args)
75     { ::new(static_cast<void*>(__p)) _T1(std::forward<_Args>(__args)...); }
```
placement new 简单地将传入指针返回，没有分配内存的操作
```
/// @file new
// Default placement versions of operator new.
inline void* operator new(std::size_t, void* __p) _GLIBCXX_USE_NOEXCEPT
{ return __p; }
```
用 placement new 可以显式的调用构造函数，比如
```
void* ptr = malloc(sizeof(int));
::new(ptr)int(10);
```

## std::_Destroy
单个对象，直接调用析构函数，没有内存释放的操作
```
/// @file bits/stl_construct.h
90   template<typename _Tp>
91     inline void
92     _Destroy(_Tp* __pointer)
93     { __pointer->~_Tp(); }
```
如果是多个对象，视数据类型不同，操作不同：

一是不借助分配器
```
/// @file bits/stl_construct.h
120   template<typename _ForwardIterator>
121     inline void
122     _Destroy(_ForwardIterator __first, _ForwardIterator __last)
123     {
124       typedef typename iterator_traits<_ForwardIterator>::value_type
125                        _Value_type;
126       std::_Destroy_aux<__has_trivial_destructor(_Value_type)>::
127     __destroy(__first, __last);
128     }
```
其中 \_Destroy_aux 是一个模板类，会根据 \_Value_type 对象是否有 trivival 析构函数（析构函数是 trivival 可以理解为可以用一个默认析构函数替代。即使一个空类，其析构函数中调用某个函数，其也不是 trival 的，因为没法用默认析构函数替代）调用不同实例。如果待析构对象的析构函数是 trivial 的话，什么也不做，不然的化，逐一调用析构函数
```
/// @file bits/stl_construct.h
 90   template<typename _Tp>
 91     inline void
 92     _Destroy(_Tp* __pointer)
 93     { __pointer->~_Tp(); }
 94 
 95   template<bool>
 96     struct _Destroy_aux
 97     {
 98       template<typename _ForwardIterator>
 99         static void
100         __destroy(_ForwardIterator __first, _ForwardIterator __last)
101     { // 逐一析构
102       for (; __first != __last; ++__first)
103         std::_Destroy(std::__addressof(*__first));
104     }
105     };
106 
107   template<>
108     struct _Destroy_aux<true>
109     {
110       template<typename _ForwardIterator>
111         static void
112         __destroy(_ForwardIterator, _ForwardIterator) { } // 什么也不做
113     };
```

二是借助分配器
```
/// @file bits/stl_construct.h
136   template<typename _ForwardIterator, typename _Allocator>
137     void
138     _Destroy(_ForwardIterator __first, _ForwardIterator __last,
139          _Allocator& __alloc)
140     {
141       typedef __gnu_cxx::__alloc_traits<_Allocator> __traits;
142       for (; __first != __last; ++__first)
143     __traits::destroy(__alloc, std::__addressof(*__first)); // 逐一析构
144     }
145 
146   template<typename _ForwardIterator, typename _Tp>
147     inline void
148     _Destroy(_ForwardIterator __first, _ForwardIterator __last,
149          allocator<_Tp>&)
150     {
151       _Destroy(__first, __last); // 会有优化
152     }
```
对于没有使用标准分配器 std::allocator 的，无论其构造函数是否是 trivial 的，不做优化，都逐一调用析构函数。比如说如果使用 vector<int, \__gnu_cxx::\__pool_alloc<int>> vec，在析构的时候就调用 \__gnu_cxx::__pool_alloc<int> 