## std::shared_ptr<void> 工作方式
下面的方式是可以正常工作的
```
class Foo {
 public:
  Foo(int val): val(val), next(nullptr) {
    std::cout << "Foo\n";
  }
  ~Foo() {
    std::cout << "~Foo\n";
  }
  int val;
  Foo* next;
};

struct Deleter {
  void operator()(Foo* p) const {
	std::cout << "Call delete from function object...\n";
	delete p;
  }
};

int main() {
  shared_ptr<void> ptr;
  ptr.reset(new Foo(-1));
  return 0;
}
```
即使定义的时候，std::shared_ptr 的类模板类型是 void 类型，我们在 reset() 函数中传入一个 Foo 类型的指针，std::shared_ptr 也可以自动地析构 Foo 的对象。如果是 std::shared_ptr\<int> 没有这种用法。
```
shared_ptr<int> ptr;
ptr.reset(new Foo(-1)); // cannot convert ‘Foo*’ to ‘int*’ in initialization
```
根据分析的继承关系，shared_ptr 继承于 __shared_ptr，回头看一下 __shared_ptr 的实现
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
可以看到，__shared_ptr::\_M_ptr 跟模板参数类型相关，而 __shared_ptr::\_M_refcount 跟模板参数是无关的。所以当模板参数是 void 的时候，void 指针可以指向任何对象，而其他指针则不行。根据前面的分析 \_M_refcount 是负责释放管理的对象的，那即使定义为 std::shared_ptr\<void>，也可以释放对象，它是如何做到的？在回头看一下 __shared_count 的定义
```
/// @file bits/shared_ptr_base.h
561   template<_Lock_policy _Lp>
562     class __shared_count
563     {

717     private:
720       _Sp_counted_base<_Lp>*  _M_pi;
721     };
```
\_Sp_counted_base 是一个基类，只有两个表示引用计数的成员
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
如前面所说，_Sp_counted_base 的 \_M_release() 会调用派生类的 \_M_dispose() 进行对象的释放。并且 __shared_count 会根据不同的传入参数，创建不同的 \_Sp_counted_base 对象。接下来分析三个派生类的构造函数。

首先是 \_Sp_counted_ptr
```
/// @file bits/shared_ptr.h
364   template<typename _Ptr, _Lock_policy _Lp>
365     class _Sp_counted_ptr final : public _Sp_counted_base<_Lp>
366     {
367     public:
368       explicit
369       _Sp_counted_ptr(_Ptr __p) noexcept
370       : _M_ptr(__p) { }
386 
387     private:
388       _Ptr             _M_ptr;
389     };
```
然后是 \_Sp_counted_deleter
```
/// @file bits/shared_ptr.h
432   template<typename _Ptr, typename _Deleter, typename _Alloc, _Lock_policy _Lp>
433     class _Sp_counted_deleter final : public _Sp_counted_base<_Lp>
434     {
435       class _Impl : _Sp_ebo_helper<0, _Deleter>, _Sp_ebo_helper<1, _Alloc>
436       {
437         typedef _Sp_ebo_helper<0, _Deleter>     _Del_base;
438         typedef _Sp_ebo_helper<1, _Alloc>       _Alloc_base;
439 
440       public:
441         _Impl(_Ptr __p, _Deleter __d, const _Alloc& __a) noexcept
442         : _M_ptr(__p), _Del_base(__d), _Alloc_base(__a)
443         { }
444 
445         _Deleter& _M_del() noexcept { return _Del_base::_S_get(*this); }
446         _Alloc& _M_alloc() noexcept { return _Alloc_base::_S_get(*this); }
447 
448         _Ptr _M_ptr;
449       };
450 
451     public:
455       _Sp_counted_deleter(_Ptr __p, _Deleter __d) noexcept
456       : _M_impl(__p, __d, _Alloc()) { }
457 
458       // __d(__p) must not throw.
459       _Sp_counted_deleter(_Ptr __p, _Deleter __d, const _Alloc& __a) noexcept
460       : _M_impl(__p, __d, __a) { }
489 
490     private:
491       _Impl _M_impl;
492     };
```
最后是 \_Sp_counted_ptr_inplace
```
/// @file bits/shared_ptr.h
498   template<typename _Tp, typename _Alloc, _Lock_policy _Lp>
499     class _Sp_counted_ptr_inplace final : public _Sp_counted_base<_Lp>
500     {
501       class _Impl : _Sp_ebo_helper<0, _Alloc>
502       {
503         typedef _Sp_ebo_helper<0, _Alloc>       _A_base;
504 
505       public:
506         explicit _Impl(_Alloc __a) noexcept : _A_base(__a) { }
507 
508         _Alloc& _M_alloc() noexcept { return _A_base::_S_get(*this); }
509 
510         __gnu_cxx::__aligned_buffer<_Tp> _M_storage;
511       };
512 
513     public:
514       using __allocator_type = __alloc_rebind<_Alloc, _Sp_counted_ptr_inplace>;
515
516       template<typename... _Args>
517         _Sp_counted_ptr_inplace(_Alloc __a, _Args&&... __args)
518         : _M_impl(__a)
519         {
520           // _GLIBCXX_RESOLVE_LIB_DEFECTS
521           // 2070.  allocate_shared should use allocator_traits<A>::construct
522           allocator_traits<_Alloc>::construct(__a, _M_ptr(),
523               std::forward<_Args>(__args)...); // might throw
524         }
```
可以知道，三个派生类都是模板类，模板参数 _Ptr 就是实际管理的对象的类型指针。所以即使在定义 std::shared_ptr 指定类模板参数为 void。可以看到 reset() 函数也是模板函数
```
1024       template<typename _Tp1>
1025         void
1026         reset(_Tp1* __p) // _Tp1 must be complete.
1027         {
1028           // Catch self-reset errors.
1029           _GLIBCXX_DEBUG_ASSERT(__p == 0 || __p != _M_ptr);
1030           __shared_ptr(__p).swap(*this);
1031         }
```
不仅如此，\__shared_ptr、\__shared_count 的有参构造函数都是模板函数。所以通过模板推断，可以推断出 reset() 传入指针的类型，然后传入相应的派生类，因此可以正常析构。