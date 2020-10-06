## 管理单个对象的 std::uniqe_ptr
拷贝赋值构造函数和赋值运算符是删除的
```
@file bits/unique_ptr.h
128   template <typename _Tp, typename _Dp = default_delete<_Tp> >
129     class unique_ptr
130     {
146       typedef std::tuple<typename _Pointer::type, _Dp>  __tuple_type;
147       __tuple_type                                      _M_t; // 指针和Deletor
149     public:
148 
150       typedef typename _Pointer::type   pointer;
151       typedef _Tp                       element_type;
152       typedef _Dp                       deleter_type;

355       // Disable copy from lvalue.
356       unique_ptr(const unique_ptr&) = delete;
357       unique_ptr& operator=(const unique_ptr&) = delete;
```

## 析构函数
```
@file bits/unique_ptr.h
232       ~unique_ptr() noexcept
233       {
234         auto& __ptr = std::get<0>(_M_t);
235         if (__ptr != nullptr)
236           get_deleter()(__ptr);
237         __ptr = pointer();
238       }
```

## 接口
```
@file bits/unique_ptr.h
287       typename add_lvalue_reference<element_type>::type
288       operator*() const
289       {
290         _GLIBCXX_DEBUG_ASSERT(get() != pointer());
291         return *get();
292       }
295       pointer
296       operator->() const noexcept
297       {
298         _GLIBCXX_DEBUG_ASSERT(get() != pointer());
299         return get();
300       }
303       pointer
304       get() const noexcept
305       { return std::get<0>(_M_t); }
308       deleter_type&
309       get_deleter() noexcept
310       { return std::get<1>(_M_t); }
313       const deleter_type&
314       get_deleter() const noexcept
315       { return std::get<1>(_M_t); }
318       explicit operator bool() const noexcept
319       { return get() == pointer() ? false : true; }
324       pointer
325       release() noexcept
326       {
327         pointer __p = get();
328         std::get<0>(_M_t) = pointer();
329         return __p;
330       }
338       void
339       reset(pointer __p = pointer()) noexcept
340       {
341         using std::swap;
342         swap(std::get<0>(_M_t), __p);
343         if (__p != pointer())
344           get_deleter()(__p);
345       }
348       void
349       swap(unique_ptr& __u) noexcept
350       {
351         using std::swap;
352         swap(_M_t, __u._M_t);
353       }
```

## 管理数组的std::uniqe_ptr
增加 operator[] 方法