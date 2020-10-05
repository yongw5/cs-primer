## traits 相关
```
/// @file bits/stl_list.h
506   template<typename _Tp, typename _Alloc = std::allocator<_Tp> >
507     class list : protected _List_base<_Tp, _Alloc>
508     {
518     public:
519       typedef _Tp                                        value_type;
520       typedef typename _Tp_alloc_type::pointer           pointer;
521       typedef typename _Tp_alloc_type::const_pointer     const_pointer;
522       typedef typename _Tp_alloc_type::reference         reference;
523       typedef typename _Tp_alloc_type::const_reference   const_reference;
524       typedef _List_iterator<_Tp>                        iterator;
525       typedef _List_const_iterator<_Tp>                  const_iterator;
526       typedef std::reverse_iterator<const_iterator>      const_reverse_iterator;
527       typedef std::reverse_iterator<iterator>            reverse_iterator;
528       typedef size_t                                     size_type;
529       typedef ptrdiff_t                                  difference_type;
530       typedef _Alloc                                     allocator_type;
```
list是一个双向链表，可以正向和反向遍历。iterator是 \_List_iterator<_Tp> 的别名，是一个双向迭代器。默认分配器是 std::allocator

## 构造函数
```
list();
explicit list(const Allocator& alloc);
explicit list(size_type count, const T& value = T(),
              const Allocator& alloc = Allocator());
list(size_type count, const T& value, const Allocator& alloc = Allocator());
explicit list(size_type count);
list(const list& other);
list(const list& other, const Allocator& alloc);
list(list&& other);
list(list&& other, const Allocator& alloc);
list(std::initializer_list<T> init, const Allocator& alloc = Allocator());

template< class InputIt >
list(InputIt first, InputIt last, const Allocator& alloc = Allocator() );
```

## 析构函数
list类中没有数据成员，没有显式定义析构函数。数据成员在其基类 \_List_base<_Tp, _Alloc> 中析构。
```
/// @file bits/stl_list.h
445       ~_List_base() _GLIBCXX_NOEXCEPT
446       { _M_clear(); }
```
\_List_base 的析构函数中调用 \_M_clear() 进行清理。在 \_M_clear() 里面，遍历链表 while (\__cur != &\_M_impl._M_node)。对于每个结点，调用分配器（默认的分配器是 std::new_allocator）的 destroy 将结点析构，然后调用 \_M_put_node() 释放内存。和 vector 不同的是，无论什么类型，list 都会先执行析构函数，再释放内存。如果存储的元素是指针，指针所指的对象不会被析构。
```
/// @file bits/list.tcc
63   template<typename _Tp, typename _Alloc>
64     void
65     _List_base<_Tp, _Alloc>::
66     _M_clear() _GLIBCXX_NOEXCEPT
67     {
68       typedef _List_node<_Tp>  _Node;
69       __detail::_List_node_base* __cur = _M_impl._M_node._M_next;
70       while (__cur != &_M_impl._M_node)
71     {
72       _Node* __tmp = static_cast<_Node*>(__cur);
73       __cur = __tmp->_M_next;
74 #if __cplusplus >= 201103L
75       _M_get_Node_allocator().destroy(__tmp);
76 #else
77       _M_get_Tp_allocator().destroy(std::__addressof(__tmp->_M_data));
78 #endif
79       _M_put_node(__tmp);
80     }
81     }
```

## List_base
主要接口有结点的分配 \_M_get_node() 和释放 \_M_put_node(\_List_node<_Tp>* \__p) 函数。此外定义计算两个结点距离的函数 \_S_distance。
```
/// @file bits/stl_list.h
297   template<typename _Tp, typename _Alloc>
298     class _List_base
299     {
300     protected:
314       typedef typename _Alloc::template rebind<_List_node<_Tp> >::other
315         _Node_alloc_type;
316 
317       typedef typename _Alloc::template rebind<_Tp>::other _Tp_alloc_type;
318 
319       static size_t
320       _S_distance(const __detail::_List_node_base* __first,
321                   const __detail::_List_node_base* __last)
322       {
323         size_t __n = 0;
324         while (__first != __last)
325           {
326             __first = __first->_M_next;
327             ++__n;
328           }
329         return __n;
330       }

356       _List_impl _M_impl;
390       _List_node<_Tp>*
391       _M_get_node()
392       { return _M_impl._Node_alloc_type::allocate(1); }
393 
394       void
395       _M_put_node(_List_node<_Tp>* __p) _GLIBCXX_NOEXCEPT
396       { _M_impl._Node_alloc_type::deallocate(__p, 1); }

445       ~_List_base() _GLIBCXX_NOEXCEPT
446       { _M_clear(); }
447 
448       void
449       _M_clear() _GLIBCXX_NOEXCEPT;
450 
451       void
452       _M_init() _GLIBCXX_NOEXCEPT
453       { // 初始化，头结点指向自己
454         this->_M_impl._M_node._M_next = &this->_M_impl._M_node;
455         this->_M_impl._M_node._M_prev = &this->_M_impl._M_node;
456         _M_set_size(0);
457       }
```
\_List_impl 定义一个结点，此结点为头结点，数据域存放链表结点个数（不包括头结点）
```
/// @file bits/stl_list.h
332       struct _List_impl
333       : public _Node_alloc_type
334       {
335 #if _GLIBCXX_USE_CXX11_ABI
336         _List_node<size_t> _M_node;
337 #else
338         __detail::_List_node_base _M_node;
339 #endif
```
结点类型为 \_List_node<size_t>，\_List_node 是一个模板类，继承于 \_List_node_base。模板参数决定结点数据域类型
```
/// @file bits/stl_list.h
105   template<typename _Tp>
106     struct _List_node : public __detail::_List_node_base
107     {
108       ///< User's data.
109       _Tp _M_data;
110 
111 #if __cplusplus >= 201103L
112       template<typename... _Args>
113         _List_node(_Args&&... __args)
114         : __detail::_List_node_base(), _M_data(std::forward<_Args>(__args)...) 
115         { }
116 #endif
117     };
```
\_List_node_base 是定义了两个指针 \_M_next 和 \_M_prev，以及添加结点和删除结点的函数，分别为 \_M_hook() 和 \_M_unhook()。
```
/// @file bits/stl_list.h
77     struct _List_node_base
78     {
79       _List_node_base* _M_next;
80       _List_node_base* _M_prev;

92       void
93       _M_hook(_List_node_base* const __position) _GLIBCXX_USE_NOEXCEPT;
94 
95       void
96       _M_unhook() _GLIBCXX_USE_NOEXCEPT;
```
\_M_hook() 是将自己插入到 \__position 指向的前面。\_M_unhook() 删除自己。
```
/// @file src/c++98/list.cc
126     void
127     _List_node_base::
128     _M_hook(_List_node_base* const __position) _GLIBCXX_USE_NOEXCEPT
129     {
130       this->_M_next = __position;
131       this->_M_prev = __position->_M_prev;
132       __position->_M_prev->_M_next = this;
133       __position->_M_prev = this;
134     }
135 
136     void
137     _List_node_base::_M_unhook() _GLIBCXX_USE_NOEXCEPT
138     {
139       _List_node_base* const __next_node = this->_M_next;
140       _List_node_base* const __prev_node = this->_M_prev;
141       __prev_node->_M_next = __next_node;
142       __next_node->_M_prev = __prev_node;
143     }
```

## 总结：
\_List_node 是一个模板类，类的参数决定数据域的类型。并且体够将自己添加到一个链表中或者将自己从一个链表中删除的操作。只有三个数据成员：两个指针域 \_M_next 和 \_M_prev 和一个数据域 \_M_data