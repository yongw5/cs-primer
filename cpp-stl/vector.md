## vector
动态数组，自动控制存储空间，根据需要进行扩展和收缩。通常拥有额外的空间以处理未来的增长，这样，每次插入元素时，不需要每次重新分配内存，仅在附加内存耗尽时才重新分配。可使用 capacity() 查看分配的内存总量，也可以调用 shrink_to_fit() 将额外的内存返回给系统。如果事先知道元素数量，可调用 reserve(n) 一次性分配内存。

## traits 相关
```
/// @file bits/stl_vector.h
213   template<typename _Tp, typename _Alloc = std::allocator<_Tp> >
214     class vector : protected _Vector_base<_Tp, _Alloc>
215     {
225     public:
226       typedef _Tp                                        value_type;
227       typedef typename _Base::pointer                    pointer;
228       typedef typename _Alloc_traits::const_pointer      const_pointer;
229       typedef typename _Alloc_traits::reference          reference;
230       typedef typename _Alloc_traits::const_reference    const_reference;
231       typedef __gnu_cxx::__normal_iterator<pointer, vector> iterator;
232       typedef __gnu_cxx::__normal_iterator<const_pointer, vector>
233       const_iterator;
234       typedef std::reverse_iterator<const_iterator>  const_reverse_iterator;
235       typedef std::reverse_iterator<iterator>            reverse_iterator;
236       typedef size_t                                     size_type;
237       typedef ptrdiff_t                                  difference_type;
238       typedef _Alloc                                     allocator_type;
```
提供的迭代器有两种，一种是封装指针的的 iterator，一种是反向迭代器 reverse_iterator。\_Vector_base 保存用三个指针 \_M_start、\_M_finish 和 \_M_end_of_storage，标志一段连续内存区域。\_M_start 标志内存开始位置，\_M_end_of_storage 标志内存结束位置，而 \_M_finish 标志存有有用数据的内存的结尾位置。

#### vector构造函数原型
构造时，内存分配的操作在 _Vector_base 中执行
```
vector();
explicit vector(const allocator_type& __a);\
explicit vector(size_type __n, const allocator_type& __a = allocator_type())
vector(size_type __n, const value_type& __value, const allocator_type& __a = allocator_type());
vector(const vector& __x);
vector(vector&& __x) noexcept;
vector(const vector& __x, const allocator_type& __a);
vector(vector&& __rv, const allocator_type& __m)
vector(initializer_list<value_type> __l, const allocator_type& __a = allocator_type())

template<typename _InputIterator,
typename = std::_RequireInputIter<_InputIterator>>
vector(_InputIterator __first, _InputIterator __last, const allocator_type& __a = allocator_type());
```

## 析构函数
vector析构函数调用 std::_Destroy，前面已经介绍，其行为跟元素类型以及使用的分配其有关.在默认情况下（使用标准分配器 std::allocator）,如果元素 \__has_trivial_destructor（可以理解为可以用一个默认析构函数替代，即使一个空类，其析构函数中调用某个函数，其也不是trival的，因为没法用默认析构函数替代），什么也不做。否则逐一调用元素的析构函数，释放内存。如果没有使用标准分配器，一律逐一调用析构函数。如果存储的元素是指针，指针所指的对象不会被析构。
```
423       ~vector() _GLIBCXX_NOEXCEPT
424       { std::_Destroy(this->_M_impl._M_start, this->_M_impl._M_finish,
425                       _M_get_Tp_allocator()); }
```

## 动态增长，以 push_back() 为例
如果还有额外的内存（\_M_finish != \_M_end_of_storage），调用拷贝构造函数直接添加。
```
/// @file bits/stl_vector.h
913       push_back(const value_type& __x)
914       {
915         if (this->_M_impl._M_finish != this->_M_impl._M_end_of_storage)
916           {
917             _Alloc_traits::construct(this->_M_impl, this->_M_impl._M_finish,
918                                      __x);
919             ++this->_M_impl._M_finish;
920           }
921         else
922 #if __cplusplus >= 201103L
923           _M_emplace_back_aux(__x);
924 #else
925           _M_insert_aux(end(), __x);
926 #endif
927       }
```
否则，需要重新分配内存，然后将现有元素拷贝至新内存，再添加，其操作在 \_M_emplace_back_aux(\__x) 完成。

首先是重新分配一块容量更大的内存
```
/// @file bits/vector.tcc
404   template<typename _Tp, typename _Alloc>
405     template<typename... _Args>
406       void
407       vector<_Tp, _Alloc>::
408       _M_emplace_back_aux(_Args&&... __args)
409       {
410    const size_type __len =
411      _M_check_len(size_type(1), "vector::_M_emplace_back_aux");
412    pointer __new_start(this->_M_allocate(__len)); // 分配内存
413    pointer __new_finish(__new_start);
```
此次分配内存的大小通过调用 \_M_check_len() 获取，定义如下
```
/// @file bits/stl_vector.h
1421       size_type
1422       _M_check_len(size_type __n, const char* __s) const
1423       {
1424         if (max_size() - size() < __n)
1425           __throw_length_error(__N(__s));
1426 
1427         const size_type __len = size() + std::max(size(), __n);
1428         return (__len < size() || __len > max_size()) ? max_size() : __len;
1429       }
```
可以看到，\_M_check_len(size_type(1), "vector::\_M_emplace_back_aux") 返回值最小为 1（初始为空，size() 为零时），否则返回 2 倍于当前 size() 的值。

然后在新分配内存的正确位置处构造待添加元素，之后将元素拷贝到新分配的内存位置。在此过程中，如果有任何异常抛出，
```
/// @file bits/vector.tcc
414    __try
415      {
416        _Alloc_traits::construct(this->_M_impl, __new_start + size(),
417                                 std::forward<_Args>(__args)...);
418        __new_finish = pointer();
419 
420        __new_finish
421          = std::__uninitialized_move_if_noexcept_a
422          (this->_M_impl._M_start, this->_M_impl._M_finish,
423           __new_start, _M_get_Tp_allocator());
424 
425        ++__new_finish;
426      }
427    __catch(...)
428      {
429        if (!__new_finish)
430          _Alloc_traits::destroy(this->_M_impl, __new_start + size());
431        else
432          std::_Destroy(__new_start, __new_finish, _M_get_Tp_allocator());
433        _M_deallocate(__new_start, __len);
434        __throw_exception_again;
435      }
```
上面 Alloc_traits::construct 就是 placement new 的封装，\_Alloc_traits::destroy 是调用析构函数的封装。

最后调用 std::_Destroy 进行析构，然后调用 \_M_deallocate 释放内存，，更改三个指针 \_M_start、
\_M_finish 和 \_M_end_of_storage
```
/// @file bits/vector.tcc
436    std::_Destroy(this->_M_impl._M_start, this->_M_impl._M_finish,
437                  _M_get_Tp_allocator());
438    _M_deallocate(this->_M_impl._M_start,
439                  this->_M_impl._M_end_of_storage
440                  - this->_M_impl._M_start);
441    this->_M_impl._M_start = __new_start;
442    this->_M_impl._M_finish = __new_finish;
443    this->_M_impl._M_end_of_storage = __new_start + __len;
444       }
```

## _Vector_base
定义内存分配和释放相关的操作
```
/// @file bits/stl_vector.h
66 namespace std _GLIBCXX_VISIBILITY(default)
67 {
68 _GLIBCXX_BEGIN_NAMESPACE_CONTAINER
69 
70   /// See bits/stl_deque.h's _Deque_base for an explanation.
71   template<typename _Tp, typename _Alloc>
72     struct _Vector_base
73     {
74       typedef typename __gnu_cxx::__alloc_traits<_Alloc>::template
75         rebind<_Tp>::other _Tp_alloc_type;
76       typedef typename __gnu_cxx::__alloc_traits<_Tp_alloc_type>::pointer
77         pointer;
/// 省略构造函数
159       ~_Vector_base() _GLIBCXX_NOEXCEPT
160       { _M_deallocate(this->_M_impl._M_start, this->_M_impl._M_end_of_storage
161                       - this->_M_impl._M_start); }
162 
163     public:
164       _Vector_impl _M_impl;
165 
166       pointer
167       _M_allocate(size_t __n)
168       {
169         typedef __gnu_cxx::__alloc_traits<_Tp_alloc_type> _Tr;
170         return __n != 0 ? _Tr::allocate(_M_impl, __n) : pointer();
171       }
172 
173       void
174       _M_deallocate(pointer __p, size_t __n)
175       {
176         typedef __gnu_cxx::__alloc_traits<_Tp_alloc_type> _Tr;
177         if (__p)
178           _Tr::deallocate(_M_impl, __p, __n);
179       }
180 
181     private:
182       void
183       _M_create_storage(size_t __n) // 构造函数中调用，分配内存
184       {
185         this->_M_impl._M_start = this->_M_allocate(__n);
186         this->_M_impl._M_finish = this->_M_impl._M_start;
187         this->_M_impl._M_end_of_storage = this->_M_impl._M_start + __n;
188       }
189     };
```
\_Vector_impl 继承类内存分配器，并定义内存管理的三个指针 \_M_start、
\_M_finish 和 \_M_end_of_storage
```
79       struct _Vector_impl 
80       : public _Tp_alloc_type
81       {
82         pointer _M_start;
83         pointer _M_finish;
84         pointer _M_end_of_storage;
/// 省略
```

## 总结：
vector 默认使用标准分配器 std::allocator，当分配内存用完，重新申请内存时，新申请的内存大小是当前内存大小的 2 倍（两倍增长）。将元素拷贝到新内存时，先将待添加的元素在新内存的正确位置构造好，再拷贝现有元素，最后进行析构和内存释放。元素的析构函数并不一定会执行。