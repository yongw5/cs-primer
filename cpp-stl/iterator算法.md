## std::distance
返回两个迭代器的距离n（__first + n == __last），会根据迭代器的类型，调用高效的算法。对于随机访问的迭代器，可在常数时间内完成。其他迭代器具有线性时间复杂度。
```
/// @@file bits/stl_iterator_base_funcs.h
112   template<typename _InputIterator>
113     inline typename iterator_traits<_InputIterator>::difference_type
114     distance(_InputIterator __first, _InputIterator __last)
115     {
116       // concept requirements -- taken care of in __distance
117       return std::__distance(__first, __last,
118                  std::__iterator_category(__first));
119     }
```
对于随机访问迭代器，可以 __last - __first
```
/// @@file bits/stl_iterator_base_funcs.h
88   template<typename _RandomAccessIterator>
89     inline typename iterator_traits<_RandomAccessIterator>::difference_type
90     __distance(_RandomAccessIterator __first, _RandomAccessIterator __last,
91                random_access_iterator_tag)
92     {
93       // concept requirements
94       __glibcxx_function_requires(_RandomAccessIteratorConcept<
95                   _RandomAccessIterator>)
96       return __last - __first;
97     }
```
但是，其他类型的迭代器，只能通过移动判断的方式
```
/// @@file bits/stl_iterator_base_funcs.h
71   template<typename _InputIterator>
72     inline typename iterator_traits<_InputIterator>::difference_type
73     __distance(_InputIterator __first, _InputIterator __last,
74                input_iterator_tag)
75     {
76       // concept requirements
77       __glibcxx_function_requires(_InputIteratorConcept<_InputIterator>)
78 
79       typename iterator_traits<_InputIterator>::difference_type __n = 0;
80       while (__first != __last)
81     {
82       ++__first;
83       ++__n;
84     }
85       return __n;
86     }
```

## std::advance
将迭代器移动n个距离。
```
/// @@file bits/stl_iterator_base_funcs.h
171   template<typename _InputIterator, typename _Distance>
172     inline void
173     advance(_InputIterator& __i, _Distance __n)
174     {
175       // concept requirements -- taken care of in __advance
176       typename iterator_traits<_InputIterator>::difference_type __d = __n;
177       std::__advance(__i, __d, std::__iterator_category(__i));
178     }
```
随机访问类型的迭代器直接用加法
```
/// @@file bits/stl_iterator_base_funcs.h
148   template<typename _RandomAccessIterator, typename _Distance>
149     inline void
150     __advance(_RandomAccessIterator& __i, _Distance __n,
151               random_access_iterator_tag)
152     {
153       // concept requirements
154       __glibcxx_function_requires(_RandomAccessIteratorConcept<
155                   _RandomAccessIterator>)
156       __i += __n;
157     }
```
其他类型的迭代器用移动判断的方式
```
/// @@file bits/stl_iterator_base_funcs.h
121   template<typename _InputIterator, typename _Distance>
122     inline void
123     __advance(_InputIterator& __i, _Distance __n, input_iterator_tag)
124     {
125       // concept requirements
126       __glibcxx_function_requires(_InputIteratorConcept<_InputIterator>)
127       _GLIBCXX_DEBUG_ASSERT(__n >= 0);
128       while (__n--)
129     ++__i;
130     }
131 
132   template<typename _BidirectionalIterator, typename _Distance>
133     inline void
134     __advance(_BidirectionalIterator& __i, _Distance __n,
135           bidirectional_iterator_tag)
136     {
137       // concept requirements
138       __glibcxx_function_requires(_BidirectionalIteratorConcept<
139                   _BidirectionalIterator>)
140       if (__n > 0)
141         while (__n--)
142       ++__i;
143       else
144         while (__n++)
145       --__i;
146     }
```

## std::next 和 std::prev
```
/// @@file bits/stl_iterator_base_funcs.h
182   template<typename _ForwardIterator>
183     inline _ForwardIterator
184     next(_ForwardIterator __x, typename
185      iterator_traits<_ForwardIterator>::difference_type __n = 1)
186     {
187       std::advance(__x, __n);
188       return __x;
189     }
190 
191   template<typename _BidirectionalIterator>
192     inline _BidirectionalIterator
193     prev(_BidirectionalIterator __x, typename
194      iterator_traits<_BidirectionalIterator>::difference_type __n = 1)
195     {
196       std::advance(__x, -__n);
197       return __x;
198     }
```