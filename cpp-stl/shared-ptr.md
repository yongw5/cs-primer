## std::shared_ptr
```
/// @file bits/shared_ptr.h
92   template<typename _Tp>
93     class shared_ptr : public __shared_ptr<_Tp>
94     {
95       template<typename _Ptr>
96         using _Convertible
97           = typename enable_if<is_convertible<_Ptr, _Tp*>::value>::type;
```
模板类 shared_ptr 继承于 __shared_ptr，具体的实现都在基类

## 构造函数
需要注意的是，构造函数 shared_ptr( Y* ptr ) 是 explicit。
```
/// TODO
```

## 析构函数
没有显式定义析构函数

## 接口
在基类中定义

## __shared_ptr
有两个数据域，一个存放对象指针，一个存放引用计数相关的结构。
```
/// @file bits/shared_ptr_base.h
 866   template<typename _Tp, _Lock_policy _Lp>
 867     class __shared_ptr
 868     {
 869       template<typename _Ptr>
 870         using _Convertible
 871           = typename enable_if<is_convertible<_Ptr, _Tp*>::value>::type;
 872 
 873     public:
 874       typedef _Tp   element_type;

1154     private:
1175       _Tp*                 _M_ptr;         // Contained pointer.
1176       __shared_count<_Lp>  _M_refcount;    // Reference counter.
```

### 构造函数
只介绍形参是 weak_ptr 的构造函数，该函数用于实现 weak_ptr::lock()。当 \_M_refcount.\_M_get_use_count() 返回 0（没有 shared_ptr 指针指向，被管理的对象已经释放）时，\_M_ptr 被设置为 nullptr，表示不能提升。
```
1144       // This constructor is used by __weak_ptr::lock() and
1145       // shared_ptr::shared_ptr(const weak_ptr&, std::nothrow_t).
1146       __shared_ptr(const __weak_ptr<_Tp, _Lp>& __r, std::nothrow_t)
1147       : _M_refcount(__r._M_refcount, std::nothrow)
1148       {
1149         _M_ptr = _M_refcount._M_get_use_count() ? __r._M_ptr : nullptr;
1150       }
```

### 析构函数
使用默认析构函数，对象的管理在 __shared_count<_Lp>::\_M_refcount 实现
```
/// @file bits/shared_ptr_base.h
925       ~__shared_ptr() = default;
```

### 接口函数
```
/// @file bits/shared_ptr_base.h
1020       void
1021       reset() noexcept
1022       { __shared_ptr().swap(*this); } // swap 用法值得学习
1023 
1024       template<typename _Tp1>
1025         void
1026         reset(_Tp1* __p) // _Tp1 must be complete.
1027         {
1028           // Catch self-reset errors.
1029           _GLIBCXX_DEBUG_ASSERT(__p == 0 || __p != _M_ptr);
1030           __shared_ptr(__p).swap(*this);
1031         }
1032 
1033       template<typename _Tp1, typename _Deleter>
1034         void
1035         reset(_Tp1* __p, _Deleter __d)
1036         { __shared_ptr(__p, __d).swap(*this); }
1037 
1038       template<typename _Tp1, typename _Deleter, typename _Alloc>
1039         void
1040         reset(_Tp1* __p, _Deleter __d, _Alloc __a)
1041         { __shared_ptr(__p, __d, std::move(__a)).swap(*this); }
1042 
1043       // Allow class instantiation when _Tp is [cv-qual] void.
1044       typename std::add_lvalue_reference<_Tp>::type
1045       operator*() const noexcept
1046       {
1047         _GLIBCXX_DEBUG_ASSERT(_M_ptr != 0);
1048         return *_M_ptr;
1049       }
1050 
1051       _Tp*
1052       operator->() const noexcept
1053       {
1054         _GLIBCXX_DEBUG_ASSERT(_M_ptr != 0);
1055         return _M_ptr;
1056       }
1057 
1058       _Tp*
1059       get() const noexcept
1060       { return _M_ptr; }
1061 
1062       explicit operator bool() const // 操作符号重载，可以用于条件判断
1063       { return _M_ptr == 0 ? false : true; }
1064 
1065       bool
1066       unique() const noexcept
1067       { return _M_refcount._M_unique(); }
1068 
1069       long
1070       use_count() const noexcept
1071       { return _M_refcount._M_get_use_count(); }
1072 
1073       void
1074       swap(__shared_ptr<_Tp, _Lp>& __other) noexcept
1075       {
1076         std::swap(_M_ptr, __other._M_ptr);
1077         _M_refcount._M_swap(__other._M_refcount);
1078       }
1079 
1080       template<typename _Tp1>
1081         bool
1082         owner_before(__shared_ptr<_Tp1, _Lp> const& __rhs) const
1083         { return _M_refcount._M_less(__rhs._M_refcount); }
1084 
1085       template<typename _Tp1>
1086         bool
1087         owner_before(__weak_ptr<_Tp1, _Lp> const& __rhs) const
1088         { return _M_refcount._M_less(__rhs._M_refcount); }
```

## __shared_count
__shared_count 不仅仅存放引用计数器，还**负责管理对象的释放**。只有一个数据域 \_M_pi，是模板类 \_Sp_counted_base 类型的指针。\_Sp_counted_base 是一个基类，其派生类有 \_Sp_counted_ptr（只有管理对象的指针，默认用 delete 释放管理的函数）、\_Sp_counted_deleter（除了管理对象的指针，还有善后函数 deleter()，在析构时调用 deleter()，不再调用 delete 释放对象。相当于用用户指定的方式释放对象）或者 \_Sp_counted_ptr_inplace（std::make_shared() 申请的对象）。所以指针 \_M_pi 可能指向不同的派生类。
```
/// @file bits/shared_ptr_base.h
561   template<_Lock_policy _Lp>
562     class __shared_count
563     {

717     private:
720       _Sp_counted_base<_Lp>*  _M_pi;
721     };
```

### 构造函数
可以传入不同的参数，对象指针，Deleter 或者分配器，此时会分配引用计数的存储空间。无参构造函数不分配引用计数空间
```
/// @file bits/shared_ptr_base.h
565       constexpr __shared_count() noexcept : _M_pi(0)
566       { }
```
当用如下的方式（只传入指针）创建一个 std::shared 对象
```
class Foo {
 public:
  Foo(int val): val(val), next(nullptr) {}
  int val;
  Foo* next;
};

std::shared_ptr<Foo> ptr(new Foo());
```
__shared_count 会执行如下构造函数，构造函数完成后 \_M_pi 指向的是 \_Sp_counted_ptr 类型的对象。
```
/// @file bits/shared_ptr_base.h
568       template<typename _Ptr>
569         explicit
570         __shared_count(_Ptr __p) : _M_pi(0)
571         {
572           __try
573             {
574               _M_pi = new _Sp_counted_ptr<_Ptr, _Lp>(__p);
575             }
576           __catch(...)
577             {
578               delete __p;
579               __throw_exception_again;
580             }
581         }
```
当用如下方式（传入指针和 Deleter ）创建一个 std::shared 对象
```
struct Deleter {
  void operator()(Foo* p) const {
	std::cout << "Call delete from function object...\n";
	delete p;
  }
};

std::shared_ptr<Foo> ptr(new Foo(-1), Deleter());
```
__shared_count 最终会执行如下构造函数，构造函数完成后 \_M_pi 指向的是 \_Sp_counted_deleter 类型的对象。
```
/// @file bits/shared_ptr_base.h
588       template<typename _Ptr, typename _Deleter, typename _Alloc>
589         __shared_count(_Ptr __p, _Deleter __d, _Alloc __a) : _M_pi(0)
590         {
591           typedef _Sp_counted_deleter<_Ptr, _Deleter, _Alloc, _Lp> _Sp_cd_type;
592           __try
593             {
594               typename _Sp_cd_type::__allocator_type __a2(__a);
595               auto __guard = std::__allocate_guarded(__a2);
596               _Sp_cd_type* __mem = __guard.get();
597               ::new (__mem) _Sp_cd_type(__p, std::move(__d), std::move(__a)); // placement new
598               _M_pi = __mem;
599               __guard = nullptr;
600             }
601           __catch(...)
602             {
603               __d(__p); // Call _Deleter on __p.
604               __throw_exception_again;
605             }
606         }
```
当用 std::make_shared() 函数创建一个 std::shared_ptr 对象的时候，会执行如下构造函数，构造函数完成后 \_M_pi 指向的是 \_Sp_counted_ptr_inplace 类型的对象。
```
/// @file bits/shared_ptr_base.h
608       template<typename _Tp, typename _Alloc, typename... _Args>
609         __shared_count(_Sp_make_shared_tag, _Tp*, const _Alloc& __a,
610                        _Args&&... __args)
611         : _M_pi(0)
612         {
613           typedef _Sp_counted_ptr_inplace<_Tp, _Alloc, _Lp> _Sp_cp_type;
614           typename _Sp_cp_type::__allocator_type __a2(__a);
615           auto __guard = std::__allocate_guarded(__a2);
616           _Sp_cp_type* __mem = __guard.get();
617           ::new (__mem) _Sp_cp_type(std::move(__a),
618                                     std::forward<_Args>(__args)...); 
619           _M_pi = __mem;
620           __guard = nullptr;
621         }
```
拷贝构造函数不分配引用计数空间，而是拷贝传入对象的 \_M_pi，并且将计数加 1
```
/// @file bits/shared_ptr_base.h
662       __shared_count(const __shared_count& __r) noexcept
663       : _M_pi(__r._M_pi)
664       {
665         if (_M_pi != 0)
666           _M_pi->_M_add_ref_copy();
667       }
```
拷贝赋值运算符增加右侧对象的引用计数，减少左侧的引用计数，如果左侧引用计数变为 0，调用 \_M_pi->\_M_release() 释放管理的对象
```
/// @file bits/shared_ptr_base.h
669       __shared_count&
670       operator=(const __shared_count& __r) noexcept
671       {
672         _Sp_counted_base<_Lp>* __tmp = __r._M_pi;
673         if (__tmp != _M_pi)
674           {
675             if (__tmp != 0)
676               __tmp->_M_add_ref_copy();
677             if (_M_pi != 0)
678               _M_pi->_M_release();
679             _M_pi = __tmp;
680           }
681         return *this;
682       }
```

### 析构函数
调用虚函数 \_M_release()，不同派生类有其自己的实现
```
/// @file bits/shared_ptr_base.h
656       ~__shared_count() noexcept
657       {
658         if (_M_pi != nullptr)
659           _M_pi->_M_release();
660       }
```

## _Sp_counted_base
是一个基类，有两个数据成员 \_M_use_count 和 \_M_weak_count，分别表示有多少个 shared_ptr 和 weak_ptr 指向管理的对象（如果 \_M_use_count 不为 0，\_M_weak_count 额外需要加 1）。此外引用计数的相关操作是原子操作。
```
/// @file bits/shared_ptr_base.h
107   template<_Lock_policy _Lp = __default_lock_policy>
108     class _Sp_counted_base
109     : public _Mutex_base<_Lp>
110     {
203     private:  

207       _Atomic_word  _M_use_count;     // #shared
208       _Atomic_word  _M_weak_count;    // #weak + (#shared != 0)
209     };
```
主要定义定义引用计数的递增函数和递减函数、释放资源的函数 \_M_release() 以及三个虚函数 \_M_dispose()、\_M_destroy() 和 \_M_get_deleter()。

### 构造函数
只有无参构造函数，无法指定引用计数。引用计数都必须通过定义的函数接口改变，另外，它的拷贝构造函数和赋值操作是删除的。
```
/// @file bits/shared_ptr_base.h
111     public:  
112       _Sp_counted_base() noexcept
113       : _M_use_count(1), _M_weak_count(1) { }
  
203     private:
204       _Sp_counted_base(_Sp_counted_base const&) = delete;
205       _Sp_counted_base& operator=(_Sp_counted_base const&) = delete;
209     };
```

### 析构函数
```
/// @file bits/shared_ptr_base.h
115       virtual
116       ~_Sp_counted_base() noexcept
117       { }
```

### 资源释放 \_M_release() 和 \_M_weak_release()
\_M_release() 首先引用计数 \_M_use_count减 1（__exchange_and_add_dispatch()，原子操作，将第二个参数加到第一个参数，返回返回第一个参数的旧值），如果引用计数变为 0，调用虚函数 \_M_dispose() 析构对象，释放内存。另外将 \_M_weak_count 减 1，如果变为 0，执行虚函数 \_M_destroy() 释放引用计数本身对象。
```
/// @file bits/shared_ptr_base.h
142       void
143       _M_release() noexcept
144       {
145         // Be race-detector-friendly.  For more info see bits/c++config.
146         _GLIBCXX_SYNCHRONIZATION_HAPPENS_BEFORE(&_M_use_count);
147         if (__gnu_cxx::__exchange_and_add_dispatch(&_M_use_count, -1) == 1)
148           {
149             _GLIBCXX_SYNCHRONIZATION_HAPPENS_AFTER(&_M_use_count);
150             _M_dispose(); // 虚函数，执行派生类实现

155             if (_Mutex_base<_Lp>::_S_need_barriers)
156               {
157                 _GLIBCXX_READ_MEM_BARRIER;
158                 _GLIBCXX_WRITE_MEM_BARRIER;
159               }
160 
161             // Be race-detector-friendly.  For more info see bits/c++config.
162             _GLIBCXX_SYNCHRONIZATION_HAPPENS_BEFORE(&_M_weak_count);
163             if (__gnu_cxx::__exchange_and_add_dispatch(&_M_weak_count,
164                                                        -1) == 1)
165               {
166                 _GLIBCXX_SYNCHRONIZATION_HAPPENS_AFTER(&_M_weak_count);
167                 _M_destroy(); // 虚函数，执行派生类实现
168               }
169           }
170       }
```
\_M_weak_release() 是跟 weak_ptr 相关的操作。\_M_weak_count 减 1，如果变为 0，调用 \_M_destroy() 释放引用计数本身对象。
```
/// @file bits/shared_ptr_base.h
176       void
177       _M_weak_release() noexcept
178       {
179         // Be race-detector-friendly. For more info see bits/c++config.
180         _GLIBCXX_SYNCHRONIZATION_HAPPENS_BEFORE(&_M_weak_count);
181         if (__gnu_cxx::__exchange_and_add_dispatch(&_M_weak_count, -1) == 1)
182           {
183             _GLIBCXX_SYNCHRONIZATION_HAPPENS_AFTER(&_M_weak_count);
184             if (_Mutex_base<_Lp>::_S_need_barriers)
185               {
186                 // See _M_release(),
187                 // destroy() must observe results of dispose()
188                 _GLIBCXX_READ_MEM_BARRIER;
189                 _GLIBCXX_WRITE_MEM_BARRIER;
190               }
191             _M_destroy();
192           }
193       }
```
\_M_weak_count = #weak + (#shared != 0) 为了确保 shared_ptr 管理的对象析构了，weak_ptr 仍然可以使用（引用计数动态内存对象没有被析构）。例如当出现 \_M_use_count = 1, \_M_weak_count = 1 的时候执行 \_M_release()，管理的对象会释放，引用计数对象也会被释放，仍然存在的一个 weak_ptr 将不能使用。

### \_M_dispose() 和 \_M_destroy()
\_M_dispose() 用于当 \_M_use_count 减为 0 的时候，释放自己 this 管理的对象（管理的动态内存）。\_M_destroy() 用于当 \_M_weak_count 减为 0 的时候，释放 this 对象（用于引用计数的动态内存）。
```
/// @file bits/shared_ptr_base.h
119       // Called when _M_use_count drops to zero, to release the resources
120       // managed by *this.
121       virtual void
122       _M_dispose() noexcept = 0; // 派生类实现
123       
124       // Called when _M_weak_count drops to zero.
125       virtual void
126       _M_destroy() noexcept
127       { delete this; }
```

### \_M_add_ref_copy() 和 \_M_weak_add_ref()
\_M_add_ref_copy() 用于将 \_M_use_count 加 1，\_M_weak_add_ref() 用于将 \_M_weak_count 加 1
```
/// @file bits/shared_ptr_base.h
132       void
133       _M_add_ref_copy()
134       { __gnu_cxx::__atomic_add_dispatch(&_M_use_count, 1); }

172       void
173       _M_weak_add_ref() noexcept
174       { __gnu_cxx::__atomic_add_dispatch(&_M_weak_count, 1); }
```

### \_M_get_use_count() 和 \_M_get_deleter()
```
/// @file bits/shared_ptr_base.h
129       virtual void*
130       _M_get_deleter(const std::type_info&) noexcept = 0; // 派生类实现

195       long
196       _M_get_use_count() const noexcept
197       {
198         // No memory barrier is used here so there is no synchronization
199         // with other threads.
200         return __atomic_load_n(&_M_use_count, __ATOMIC_RELAXED);
201       }
```

### \_M_add_ref_lock() 和 \_M_add_ref_lock_nothrow()
这两个函数用于 weak_ptr::lock() 操作。下面仅仅是一部分，还有其他偏特化版本。当 \_M_use_count 为 0，是提升失败
```
/// @file bits/shared_ptr_base.h
221   template<>
222     inline void
223     _Sp_counted_base<_S_mutex>::
224     _M_add_ref_lock()
225     {
226       __gnu_cxx::__scoped_lock sentry(*this);
227       if (__gnu_cxx::__exchange_and_add_dispatch(&_M_use_count, 1) == 0)
228         {
229           _M_use_count = 0;
230           __throw_bad_weak_ptr();
231         }
232     }

264   template<>
265     inline bool
266     _Sp_counted_base<_S_mutex>::
267     _M_add_ref_lock_nothrow()
268     {
269       __gnu_cxx::__scoped_lock sentry(*this);
270       if (__gnu_cxx::__exchange_and_add_dispatch(&_M_use_count, 1) == 0)
271         {
272           _M_use_count = 0;
273           return false;
274         }
275       return true;
276     }
```

### \_M_dispose()、\_M_destroy() 和 \_M_get_deleter() 在派生类中的实现
在 \_Sp_counted_ptr 中的实现
```
/// @file bits/shared_ptr.h
364   template<typename _Ptr, _Lock_policy _Lp>
365     class _Sp_counted_ptr final : public _Sp_counted_base<_Lp>
366     {
367     public:
/// ...
372       virtual void
373       _M_dispose() noexcept
374       { delete _M_ptr; } // 调用 delete
375 
376       virtual void
377       _M_destroy() noexcept
378       { delete this; }
379 
380       virtual void*
381       _M_get_deleter(const std::type_info&) noexcept
382       { return nullptr; } // 没有
```
在 \_Sp_counted_deleter 中的实现
```
/// @file bits/shared_ptr.h
432   template<typename _Ptr, typename _Deleter, typename _Alloc, _Lock_policy _Lp>
433     class _Sp_counted_deleter final : public _Sp_counted_base<_Lp>
434     {
451     public:
464       virtual void
465       _M_dispose() noexcept
466       { _M_impl._M_del()(_M_impl._M_ptr); } // 调用指定的可调用对象
467 
468       virtual void
469       _M_destroy() noexcept
470       {
471         __allocator_type __a(_M_impl._M_alloc());
472         __allocated_ptr<__allocator_type> __guard_ptr{ __a, this };
473         this->~_Sp_counted_deleter(); // 调用析构函数（空），基类delete
474       }
475 
476       virtual void*
477       _M_get_deleter(const std::type_info& __ti) noexcept
478       {
479 #if __cpp_rtti
480         // _GLIBCXX_RESOLVE_LIB_DEFECTS
481         // 2400. shared_ptr's get_deleter() should use addressof()
482         return __ti == typeid(_Deleter)
483           ? std::__addressof(_M_impl._M_del())
484           : nullptr;
485 #else
486         return nullptr;
487 #endif
```
在 \_Sp_counted_ptr_inplace 中的实现
```
/// @file bits/shared_ptr.h
498   template<typename _Tp, typename _Alloc, _Lock_policy _Lp>
499     class _Sp_counted_ptr_inplace final : public _Sp_counted_base<_Lp>
500     {
/// ...
512 
513     public:
514       using __allocator_type = __alloc_rebind<_Alloc, _Sp_counted_ptr_inplace>;
/// ...
525 
526       ~_Sp_counted_ptr_inplace() noexcept { }
527 
528       virtual void
529       _M_dispose() noexcept
530       { // 调用分配器的 destroy()
531         allocator_traits<_Alloc>::destroy(_M_impl._M_alloc(), _M_ptr());
532       }
533 
534       // Override because the allocator needs to know the dynamic type
535       virtual void
536       _M_destroy() noexcept
537       {
538         __allocator_type __a(_M_impl._M_alloc());
539         __allocated_ptr<__allocator_type> __guard_ptr{ __a, this };
540         this->~_Sp_counted_ptr_inplace(); // 析构函数为空，基类 delete
541       }
542 
543       // Sneaky trick so __shared_ptr can get the managed pointer
544       virtual void*
545       _M_get_deleter(const std::type_info& __ti) noexcept
546       {
547 #if __cpp_rtti
548         if (__ti == typeid(_Sp_make_shared_tag))
549           return const_cast<typename remove_cv<_Tp>::type*>(_M_ptr());
550 #endif
551         return nullptr; // 没有
552       }
```

## hash
根据 get() 返回的指针进行 hash
```
/// @file bits/shared_ptr.h
634   template<typename _Tp>
635     struct hash<shared_ptr<_Tp>>
636     : public __hash_base<size_t, shared_ptr<_Tp>>
637     {
638       size_t
639       operator()(const shared_ptr<_Tp>& __s) const noexcept
640       { return std::hash<_Tp*>()(__s.get()); }
641     };
```

## shared_ptr 指针转换
创建新的 std::shared_ptr 的实例，将管理对象的类型从 _Tp1 转换成 _Tp。底层仍然共享管理的对象
```
/// @file bits/shared_ptr.h
444   template<typename _Tp, typename _Tp1>
445     inline shared_ptr<_Tp>
446     static_pointer_cast(const shared_ptr<_Tp1>& __r) noexcept
447     { return shared_ptr<_Tp>(__r, static_cast<_Tp*>(__r.get())); }
448 
449   template<typename _Tp, typename _Tp1>
450     inline shared_ptr<_Tp>
451     const_pointer_cast(const shared_ptr<_Tp1>& __r) noexcept
452     { return shared_ptr<_Tp>(__r, const_cast<_Tp*>(__r.get())); }
453 
454   template<typename _Tp, typename _Tp1>
455     inline shared_ptr<_Tp>
456     dynamic_pointer_cast(const shared_ptr<_Tp1>& __r) noexcept
457     {
458       if (_Tp* __p = dynamic_cast<_Tp*>(__r.get()))
459         return shared_ptr<_Tp>(__r, __p);
460       return shared_ptr<_Tp>();
461     }
```