## std::make_shared()
调用 allocate_shared 构造一个 shared_ptr
```
/// @file bits/shared_ptr.h
609   template<typename _Tp, typename _Alloc, typename... _Args>
610     inline shared_ptr<_Tp>
611     allocate_shared(const _Alloc& __a, _Args&&... __args)
612     {
613       return shared_ptr<_Tp>(_Sp_make_shared_tag(), __a,
614                              std::forward<_Args>(__args)...);
615     }

624   template<typename _Tp, typename... _Args>
625     inline shared_ptr<_Tp>
626     make_shared(_Args&&... __args)
627     {
628       typedef typename std::remove_const<_Tp>::type _Tp_nc;
629       return std::allocate_shared<_Tp>(std::allocator<_Tp_nc>(),
630                                        std::forward<_Args>(__args)...);
631     }
```