## std::allocator<_Tp>
定义 \__allocator_base，定义为 \__gnu_cxx::new_allocator<_Tp> 的别名
```
namespace std {
/// @@file bits/c++allocator.h
35 #if __cplusplus >= 201103L
36 namespace std
37 {
47   template<typename _Tp>
48     using __allocator_base = __gnu_cxx::new_allocator<_Tp>;
49 }
```

定义 allocator<_Tp>。仅仅继承 \__allocator_base<_Tp>，没有过多的修改
```
/// @@file bits/allocator.h
 52 namespace std _GLIBCXX_VISIBILITY(default)
 61   /// allocator<void> specialization.
 62   template<>
 63     class allocator<void> // 是一个空类
 64     {
 65     public:
 66       typedef size_t      size_type;
 67       typedef ptrdiff_t   difference_type;
 68       typedef void*       pointer;
 69       typedef const void* const_pointer;
 70       typedef void        value_type;
 71 
 72       template<typename _Tp1>
 73         struct rebind
 74         { typedef allocator<_Tp1> other; };
 75 
 76 #if __cplusplus >= 201103L
 77       // _GLIBCXX_RESOLVE_LIB_DEFECTS
 78       // 2103. std::allocator propagate_on_container_move_assignment
 79       typedef true_type propagate_on_container_move_assignment;
 80 #endif
 81     };
 91   template<typename _Tp>
 92     class allocator: public __allocator_base<_Tp>
 93     {
 94    public:
 95       typedef size_t     size_type;
 96       typedef ptrdiff_t  difference_type;
 97       typedef _Tp*       pointer;
 98       typedef const _Tp* const_pointer;
 99       typedef _Tp&       reference;
100       typedef const _Tp& const_reference;
101       typedef _Tp        value_type;
102 
103       template<typename _Tp1>
104         struct rebind
105         { typedef allocator<_Tp1> other; };
106 
107 #if __cplusplus >= 201103L
108       // _GLIBCXX_RESOLVE_LIB_DEFECTS
109       // 2103. std::allocator propagate_on_container_move_assignment
110       typedef true_type propagate_on_container_move_assignment;
111 #endif
112 
113       allocator() throw() { }
114 
115       allocator(const allocator& __a) throw()
116       : __allocator_base<_Tp>(__a) { }
117 
118       template<typename _Tp1>
119         allocator(const allocator<_Tp1>&) throw() { }
120 
121       ~allocator() throw() { }
122 
123       // Inherit everything else.
124     };
}
```

### \__gnu_cxx::new_allocator<_Tp>
分配和释放分别调用 ::operator new 和 ::operator delete，没有接手内存管理
```
/// @@file ext/new_allocator.h
 40 namespace __gnu_cxx _GLIBCXX_VISIBILITY(default)
 41 {
 42 _GLIBCXX_BEGIN_NAMESPACE_VERSION
 43 
 44   using std::size_t;
 45   using std::ptrdiff_t;
 57   template<typename _Tp>
 58     class new_allocator
 59     {
 60     public:
 61       typedef size_t     size_type;
 62       typedef ptrdiff_t  difference_type;
 63       typedef _Tp*       pointer;
 64       typedef const _Tp* const_pointer;
 65       typedef _Tp&       reference;
 66       typedef const _Tp& const_reference;
 67       typedef _Tp        value_type;
 68 
 69       template<typename _Tp1>
 70         struct rebind
 71         { typedef new_allocator<_Tp1> other; };
 72 
 73 #if __cplusplus >= 201103L
 74       // _GLIBCXX_RESOLVE_LIB_DEFECTS
 75       // 2103. propagate_on_container_move_assignment
 76       typedef std::true_type propagate_on_container_move_assignment;
 77 #endif
 78 
 79       new_allocator() _GLIBCXX_USE_NOEXCEPT { }
 80 
 81       new_allocator(const new_allocator&) _GLIBCXX_USE_NOEXCEPT { }
 82 
 83       template<typename _Tp1>
 84         new_allocator(const new_allocator<_Tp1>&) _GLIBCXX_USE_NOEXCEPT { }
 85 
 86       ~new_allocator() _GLIBCXX_USE_NOEXCEPT { }
 87 
 88       pointer
 89       address(reference __x) const _GLIBCXX_NOEXCEPT
 90       { return std::__addressof(__x); }
 91 
 92       const_pointer
 93       address(const_reference __x) const _GLIBCXX_NOEXCEPT
 94       { return std::__addressof(__x); }
 95 
 96       // NB: __n is permitted to be 0.  The C++ standard says nothing
 97       // about what the return value is when __n == 0.
 98       pointer
 99       allocate(size_type __n, const void* = 0)
100       { 
101         if (__n > this->max_size())
102           std::__throw_bad_alloc();
103 
104         return static_cast<_Tp*>(::operator new(__n * sizeof(_Tp)));
105       }
106 
107       // __p is not permitted to be a null pointer.
108       void
109       deallocate(pointer __p, size_type)
110       { ::operator delete(__p); }
111 
112       size_type
113       max_size() const _GLIBCXX_USE_NOEXCEPT
114       { return size_t(-1) / sizeof(_Tp); }
115 
116 #if __cplusplus >= 201103L
117       template<typename _Up, typename... _Args>
118         void
119         construct(_Up* __p, _Args&&... __args)
120         { ::new((void *)__p) _Up(std::forward<_Args>(__args)...); }
121 
122       template<typename _Up>
123         void 
124         destroy(_Up* __p) { __p->~_Up(); }
125 #else
126       // _GLIBCXX_RESOLVE_LIB_DEFECTS
127       // 402. wrong new expression in [some_] allocator::construct
128       void 
129       construct(pointer __p, const _Tp& __val) 
130       { ::new((void *)__p) _Tp(__val); }
131 
132       void 
133       destroy(pointer __p) { __p->~_Tp(); }
134 #endif
135     };
```
## 总结
标准库的分配器 std::allocator<_Tp> 的分配和释放对象最终分别调用 ::operator new 和 ::operator delete，本身没有内存管理。具有内存管理的分配器是 \__gnu_cxx::__pool_alloc<_Tp>。此外 std::allocator 将内存申请、构造、析构和内存释放分开，在内存释放前需要考虑析构函数，不然会引起内存泄漏。