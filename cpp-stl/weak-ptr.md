## std::weak_ptr
不控制所指向对象生命周期的智能指针，它指向一个 shared_ptr 管理的对象
```
/// @file bits/shared_ptr.h
469   template<typename _Tp>
470     class weak_ptr : public __weak_ptr<_Tp>
471     {
472       template<typename _Ptr>
473         using _Convertible
474           = typename enable_if<is_convertible<_Ptr, _Tp*>::value>::type;
```
具体实现在基类 __weak_ptr<_Tp> 中

## 构造函数
/// TODO

## 析构函数
没有显式定义的析构函数

## lock()
```
/// @file bits/shared_ptr.h
525       shared_ptr<_Tp>
526       lock() const noexcept
527       { return shared_ptr<_Tp>(*this, std::nothrow); }
```

## __weak_ptr
有两个数据域，一个存放对象指针，一个存放引用计数相关的结构。
```
/// @file bits/shared_ptr_base.h
1339   template<typename _Tp, _Lock_policy _Lp>
1340     class __weak_ptr
1341     {
1342       template<typename _Ptr>
1343         using _Convertible
1344           = typename enable_if<is_convertible<_Ptr, _Tp*>::value>::type;

1463     private:
1477       _Tp*               _M_ptr;         // Contained pointer.
1478       __weak_count<_Lp>  _M_refcount;    // Reference counter.
```

### 析构函数
默认析构函数
```
/// @file bits/shared_ptr_base.h
1346     public:
1355       ~__weak_ptr() = default;
```

### 接口
```
/// @file bits/shared_ptr_base.h
1430       __shared_ptr<_Tp, _Lp>
1431       lock() const noexcept
1432       { return __shared_ptr<element_type, _Lp>(*this, std::nothrow); }
1433 
1434       long
1435       use_count() const noexcept
1436       { return _M_refcount._M_get_use_count(); }
1437 
1438       bool
1439       expired() const noexcept
1440       { return _M_refcount._M_get_use_count() == 0; }
1441 
1442       template<typename _Tp1>
1443         bool
1444         owner_before(const __shared_ptr<_Tp1, _Lp>& __rhs) const
1445         { return _M_refcount._M_less(__rhs._M_refcount); }
1446 
1447       template<typename _Tp1>
1448         bool
1449         owner_before(const __weak_ptr<_Tp1, _Lp>& __rhs) const
1450         { return _M_refcount._M_less(__rhs._M_refcount); }
1451 
1452       void
1453       reset() noexcept
1454       { __weak_ptr().swap(*this); }
```

## __weak_count
只有一个数据成员 \_M_pi，是 \_Sp_counted_base 类型，和 \__shared_count 一样
```
/// @file bits/shared_ptr_base.h
724   template<_Lock_policy _Lp>
725     class __weak_count

814     private:
817       _Sp_counted_base<_Lp>*  _M_pi;
```

### 构造函数
需要判断 \_M_pi 的有效性（是否为空指针）
```
/// @file bits/shared_ptr_base.h
727     public:
728       constexpr __weak_count() noexcept : _M_pi(nullptr)
729       { }
730 
731       __weak_count(const __shared_count<_Lp>& __r) noexcept
732       : _M_pi(__r._M_pi)
733       {
734         if (_M_pi != nullptr)
735           _M_pi->_M_weak_add_ref();
736       }
737 
738       __weak_count(const __weak_count& __r) noexcept
739       : _M_pi(__r._M_pi)
740       {
741         if (_M_pi != nullptr)
742           _M_pi->_M_weak_add_ref();
743       }
744 
745       __weak_count(__weak_count&& __r) noexcept
746       : _M_pi(__r._M_pi)
747       { __r._M_pi = nullptr; }
```

### 析构函数
```
/// @file bits/shared_ptr_base.h
749       ~__weak_count() noexcept
750       {
751         if (_M_pi != nullptr)
752           _M_pi->_M_weak_release();
753       }
```

### \_M_get_use_count()
```
/// @file bits/shared_ptr_base.h
797       long
798       _M_get_use_count() const noexcept
799       { return _M_pi != nullptr ? _M_pi->_M_get_use_count() : 0; }
```