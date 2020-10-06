## std::enable_shared_from_this
拥有一个 weak_ptr
```
/// @file bits/shared_ptr.h
556   template<typename _Tp>
557     class enable_shared_from_this
558     {
559     protected:
560       constexpr enable_shared_from_this() noexcept { }
561 
562       enable_shared_from_this(const enable_shared_from_this&) noexcept { }
563 
564       enable_shared_from_this&
565       operator=(const enable_shared_from_this&) noexcept
566       { return *this; }
567 
568       ~enable_shared_from_this() { }
578 
579     private:
580       template<typename _Tp1>
581         void
582         _M_weak_assign(_Tp1* __p, const __shared_count<>& __n) const noexcept
583         { _M_weak_this._M_assign(__p, __n); }
584 
585       template<typename _Tp1>
586         friend void
587         __enable_shared_from_this_helper(const __shared_count<>& __pn,
588                                          const enable_shared_from_this* __pe,
589                                          const _Tp1* __px) noexcept
590         {
591           if (__pe != 0)
592             __pe->_M_weak_assign(const_cast<_Tp1*>(__px), __pn);
593         }
594 
595       mutable weak_ptr<_Tp>  _M_weak_this;
596     };
```

## shared_from_this()
```
/// @file bits/shared_ptr.h
570     public:
571       shared_ptr<_Tp>
572       shared_from_this()
573       { return shared_ptr<_Tp>(this->_M_weak_this); }
574 
575       shared_ptr<const _Tp>
576       shared_from_this() const
577       { return shared_ptr<const _Tp>(this->_M_weak_this); }
```