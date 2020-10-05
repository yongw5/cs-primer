## uninitialized_copy(first, last, d_first)
和 copy 类似，不过不需要目的地址是已经构造的内存。将元素从 [first, last) 拷贝到 d_first 所指的未构造的内存中。如果拷贝过程中抛出异常，所有已拷贝的对象都会析构。
```
/// @file bits/stl_uninitialized.h
105   template<typename _InputIterator, typename _ForwardIterator>
106     inline _ForwardIterator
107     uninitialized_copy(_InputIterator __first, _InputIterator __last,
108                _ForwardIterator __result)
109     {
110       typedef typename iterator_traits<_InputIterator>::value_type
111     _ValueType1;
112       typedef typename iterator_traits<_ForwardIterator>::value_type
113     _ValueType2;
114 #if __cplusplus < 201103L
115       const bool __assignable = true;
116 #else
117       // trivial types can have deleted assignment
118       typedef typename iterator_traits<_InputIterator>::reference _RefType1;
119       typedef typename iterator_traits<_ForwardIterator>::reference _RefType2;
120       const bool __assignable = is_assignable<_RefType2, _RefType1>::value;
121 #endif
122 
123       return std::__uninitialized_copy<__is_trivial(_ValueType1)
124                        && __is_trivial(_ValueType2)
125                        && __assignable>::
126     __uninit_copy(__first, __last, __result);
127     }
```
\__uninitialized_copy 会根据元素的类型具体执行，如果不是 TrivalType，会逐一执行构造函数。
```
/// @file bits/stl_uninitialized.h
63   template<bool _TrivialValueTypes>
64     struct __uninitialized_copy
65     {
66       template<typename _InputIterator, typename _ForwardIterator>
67         static _ForwardIterator
68         __uninit_copy(_InputIterator __first, _InputIterator __last,
69                       _ForwardIterator __result)
70         {
71           _ForwardIterator __cur = __result;
72           __try
73             {
74               for (; __first != __last; ++__first, ++__cur)
75                 std::_Construct(std::__addressof(*__cur), *__first);
76               return __cur;
77             }
78           __catch(...)
79             {
80               std::_Destroy(__result, __cur);
81               __throw_exception_again;
82             }
83         }
84     };
```
如果是 TrivalType，直接用 std::copy 拷贝
```
/// @file bits/stl_uninitialized.h
86   template<>
87     struct __uninitialized_copy<true>
88     {
89       template<typename _InputIterator, typename _ForwardIterator>
90         static _ForwardIterator
91         __uninit_copy(_InputIterator __first, _InputIterator __last,
92                       _ForwardIterator __result)
93         { return std::copy(__first, __last, __result); }
94     };
```
std::copy 会进行优化，包括运用移动 std::move，或者调用更高效的 memmove（memmove 能够保证源串在被覆盖之前将重叠区域的字节拷贝到目标区域中，但复制后源内容会被更改）

## uninitialized_copy_n(first, n, d_first)
从 first 拷贝n个元素到 d_first
```
/// @file bits/stl_uninitialized.h
677   template<typename _InputIterator, typename _Size, typename _ForwardIterator>
678     inline _ForwardIterator
679     uninitialized_copy_n(_InputIterator __first, _Size __n,
680                          _ForwardIterator __result)
681     { return std::__uninitialized_copy_n(__first, __n, __result,
682                                          std::__iterator_category(__first)); }
```
\__uninitialized_copy_n 会根据迭代器类型进行优化，如果是随机访问迭代器，直接调用 uninitialized_copy(first, first+n, d_first)
```
/// @file bits/stl_uninitialized.h
660   template<typename _RandomAccessIterator, typename _Size,
661            typename _ForwardIterator>
662     inline _ForwardIterator
663     __uninitialized_copy_n(_RandomAccessIterator __first, _Size __n,
664                            _ForwardIterator __result,
665                            random_access_iterator_tag)
666     { return std::uninitialized_copy(__first, __first + __n, __result); }
```
其他迭代器，逐一拷贝
```
/// @file bits/stl_uninitialized.h
640   template<typename _InputIterator, typename _Size,
641            typename _ForwardIterator>
642     _ForwardIterator
643     __uninitialized_copy_n(_InputIterator __first, _Size __n,
644                            _ForwardIterator __result, input_iterator_tag)
645     {
646       _ForwardIterator __cur = __result;
647       __try
648         {
649           for (; __n > 0; --__n, ++__first, ++__cur)
650             std::_Construct(std::__addressof(*__cur), *__first);
651           return __cur;
652         }
653       __catch(...)
654         {
655           std::_Destroy(__result, __cur);
656           __throw_exception_again;
657         }
658     }
```

## uninitialized_fill(first, last, value)
在 [first, last) 指定的原始内存中构造对象，对象的值为 value。如果构造过程中抛出异常，已构造的对象会析构
```
/// @file bits/stl_uninitialized.h
171   template<typename _ForwardIterator, typename _Tp>
172     inline void
173     uninitialized_fill(_ForwardIterator __first, _ForwardIterator __last,
174                        const _Tp& __x)
175     {
176       typedef typename iterator_traits<_ForwardIterator>::value_type
177         _ValueType;
178 #if __cplusplus < 201103L
179       const bool __assignable = true;
180 #else
181       // trivial types can have deleted assignment
182       const bool __assignable = is_copy_assignable<_ValueType>::value;
183 #endif
184 
185       std::__uninitialized_fill<__is_trivial(_ValueType) && __assignable>::
186         __uninit_fill(__first, __last, __x);
187     }
```
\__uninitialized_fill 会根据元素的类型具体执行，如果不是 TrivalType，会逐一执行构造函数。
```
/// @file bits/stl_uninitialized.h
130   template<bool _TrivialValueType>
131     struct __uninitialized_fill
132     {
133       template<typename _ForwardIterator, typename _Tp>
134         static void
135         __uninit_fill(_ForwardIterator __first, _ForwardIterator __last,
136                       const _Tp& __x)
137         {
138           _ForwardIterator __cur = __first;
139           __try
140             {
141               for (; __cur != __last; ++__cur)
142                 std::_Construct(std::__addressof(*__cur), __x);
143             }
144           __catch(...)
145             {
146               std::_Destroy(__first, __cur);
147               __throw_exception_again;
148             }
149         }
150     };
```
如果是 TrivalType，直接调用 std::fill
```
/// @file bits/stl_uninitialized.h
152   template<>
153     struct __uninitialized_fill<true>
154     {
155       template<typename _ForwardIterator, typename _Tp>
156         static void
157         __uninit_fill(_ForwardIterator __first, _ForwardIterator __last,
158                       const _Tp& __x)
159         { std::fill(__first, __last, __x); }
160     };
```

## uninitialized_fill_n(first, n, value)
在[first, first+n)指定的原始内存中构造对象，对象的值为value
```
/// @file bits/stl_uninitialized.h
234   template<typename _ForwardIterator, typename _Size, typename _Tp>
235     inline _ForwardIterator
236     uninitialized_fill_n(_ForwardIterator __first, _Size __n, const _Tp& __x)
237     {
238       typedef typename iterator_traits<_ForwardIterator>::value_type
239         _ValueType;
240 #if __cplusplus < 201103L
241       const bool __assignable = true;
242 #else
243       // trivial types can have deleted assignment
244       const bool __assignable = is_copy_assignable<_ValueType>::value;
245 #endif
246       return __uninitialized_fill_n<__is_trivial(_ValueType) && __assignable>::
247         __uninit_fill_n(__first, __n, __x);
248     }
```
\__uninitialized_fill_n 根据元素是否是 TrivalType 进行优化，如果不是，逐一调用构造函数
```
/// @file bits/stl_uninitialized.h
190   template<bool _TrivialValueType>
191     struct __uninitialized_fill_n
192     {
193       template<typename _ForwardIterator, typename _Size, typename _Tp>
194         static _ForwardIterator
195         __uninit_fill_n(_ForwardIterator __first, _Size __n,
196                         const _Tp& __x)
197         {
198           _ForwardIterator __cur = __first;
199           __try
200             {
201               for (; __n > 0; --__n, ++__cur)
202                 std::_Construct(std::__addressof(*__cur), __x);
203               return __cur;
204             }
205           __catch(...)
206             {
207               std::_Destroy(__first, __cur);
208               __throw_exception_again;
209             }
210         }
211     };
```
否则，直接调用 std::fill_n
```
/// @file bits/stl_uninitialized.h
213   template<>
214     struct __uninitialized_fill_n<true>
215     {
216       template<typename _ForwardIterator, typename _Size, typename _Tp>
217         static _ForwardIterator
218         __uninit_fill_n(_ForwardIterator __first, _Size __n,
219                         const _Tp& __x)
220         { return std::fill_n(__first, __n, __x); }
221     };
```