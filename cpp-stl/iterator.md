## Iterator Tags
Iterator Tags标志类迭代器能够进行的操作，比如只能前向移动，或双向移动，或随机访问
```
/// @@file bits/stl_iterator_base_types.h
 70 namespace std _GLIBCXX_VISIBILITY(default)
 71 {
 72 _GLIBCXX_BEGIN_NAMESPACE_VERSION
 88   ///  Marking input iterators.
 89   struct input_iterator_tag { }; // 支持operator++
 90 
 91   ///  Marking output iterators.
 92   struct output_iterator_tag { }; // 支持operator++
 93 
 94   /// Forward iterators support a superset of input iterator operations.
 95   struct forward_iterator_tag : public input_iterator_tag { };  // 支持operator++
 96 
 97   /// Bidirectional iterators support a superset of forward iterator
 98   /// operations.  // 支持operator++ operator--
 99   struct bidirectional_iterator_tag : public forward_iterator_tag { };
100 
101   /// Random-access iterators support a superset of bidirectional
102   /// iterator operations.  // 支持operator++ operator-- p+n 等待
103   struct random_access_iterator_tag : public bidirectional_iterator_tag { };
```

## 通用Iterator
每个迭代器都必须提供5类信息：iterator_category、value_type、difference_type、pointer、reference。
第一个属于迭代器本身的信息，表示iterator_tag，标识迭代器本身的操作类型，后四种标志迭代器所指数据的相关信息。
```
116   template<typename _Category, typename _Tp, typename _Distance = ptrdiff_t,
117            typename _Pointer = _Tp*, typename _Reference = _Tp&>
118     struct iterator
119     {
120       /// One of the @link iterator_tags tag types@endlink.
121       typedef _Category  iterator_category;
122       /// The type "pointed to" by the iterator.
123       typedef _Tp        value_type;
124       /// Distance between iterators is represented as this type.
125       typedef _Distance  difference_type;
126       /// This type represents a pointer-to-value_type.
127       typedef _Pointer   pointer;
128       /// This type represents a reference-to-value_type.
129       typedef _Reference reference;
130     };
```

##  Iterator Traits
标志迭代器所指数据的相关信息（比如数据类型、引用类型）以及迭代器自身信息（tags）。主要是为了使迭代器和指针能够兼容，比如说标准库算法传入迭代器和传入指针都可以。手段是使用类模板的偏特化（Partial Specialization），偏特化指“针对（任何）template参数更进一步的条件限制所设计出来的一个特化版本”。其本身仍然是类模板，只是模板参数的类型受到了限制。
```
/// @@file bits/stl_iterator_base_types.h
143   template<typename _Iterator, typename = __void_t<>>
144     struct __iterator_traits { };
145 
146   template<typename _Iterator>
147     struct __iterator_traits<_Iterator,
148                  __void_t<typename _Iterator::iterator_category,
149                       typename _Iterator::value_type,
150                       typename _Iterator::difference_type,
151                       typename _Iterator::pointer,
152                       typename _Iterator::reference>>
153     {
154       typedef typename _Iterator::iterator_category iterator_category;
155       typedef typename _Iterator::value_type        value_type;
156       typedef typename _Iterator::difference_type   difference_type;
157       typedef typename _Iterator::pointer           pointer;
158       typedef typename _Iterator::reference         reference;
159     };
160 
161   template<typename _Iterator>
162     struct iterator_traits
163     : public __iterator_traits<_Iterator> { };

176   /// Partial specialization for pointer types.
177   template<typename _Tp> 
178     struct iterator_traits<_Tp*> // 接受指针
179     {
180       typedef random_access_iterator_tag iterator_category;
181       typedef _Tp                         value_type;
182       typedef ptrdiff_t                   difference_type;
183       typedef _Tp*                        pointer;
184       typedef _Tp&                        reference;
185     };
186 
187   /// Partial specialization for const pointer types.
188   template<typename _Tp>
189     struct iterator_traits<const _Tp*> // 接受引用
190     {
191       typedef random_access_iterator_tag iterator_category;
192       typedef _Tp                         value_type;
193       typedef ptrdiff_t                   difference_type;
194       typedef const _Tp*                  pointer;
195       typedef const _Tp&                  reference;
196     };

202   template<typename _Iter>
203     inline typename iterator_traits<_Iter>::iterator_category
204     __iterator_category(const _Iter&)
205     { return typename iterator_traits<_Iter>::iterator_category(); }
```