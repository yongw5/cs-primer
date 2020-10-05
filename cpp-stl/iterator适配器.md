## \__gnu_cxx::\__normal_iterator
该适配器不改变其迭代器的任何操作，主要的目的是将一个指针这样的“迭代器”转换成 STL 迭代器。指针支持的操作，该适配器都支持。
```
/// @@file bits/stl_iterator.h
707 namespace __gnu_cxx _GLIBCXX_VISIBILITY(default)
708 {
718   using std::iterator_traits;
719   using std::iterator;
720   template<typename _Iterator, typename _Container>
721     class __normal_iterator
722     {
723     protected:
724       _Iterator _M_current;
725 
726       typedef iterator_traits<_Iterator>        __traits_type;
727 
728     public:
729       typedef _Iterator                 iterator_type;
730       typedef typename __traits_type::iterator_category iterator_category;
731       typedef typename __traits_type::value_type    value_type;
732       typedef typename __traits_type::difference_type   difference_type;
733       typedef typename __traits_type::reference     reference;
734       typedef typename __traits_type::pointer       pointer;
735 
736       _GLIBCXX_CONSTEXPR __normal_iterator() _GLIBCXX_NOEXCEPT
737       : _M_current(_Iterator()) { }
738 
739       explicit
740       __normal_iterator(const _Iterator& __i) _GLIBCXX_NOEXCEPT
741       : _M_current(__i) { }
742 
743       // Allow iterator to const_iterator conversion
744       template<typename _Iter>
745         __normal_iterator(const __normal_iterator<_Iter,
746               typename __enable_if<
747                (std::__are_same<_Iter, typename _Container::pointer>::__value),
748               _Container>::__type>& __i) _GLIBCXX_NOEXCEPT
749         : _M_current(__i.base()) { }
750  // .....
936 } // namespace
```

## std::reverse_iterator
双向和随机访问的迭代器可以使用此适配器，目的是改变迭代器的移动方向。该适配器的移动方向其底层装饰的迭代器的移动方向相反。值得注意的是，它的解引用操作的是先移动，在求解。因为迭代器范围是前闭后开区间，最后的迭代器指向的是最后一个元素的下一个位置。
```
/// @@file bits/stl_iterator.h
 68 namespace std _GLIBCXX_VISIBILITY(default)
 69 {
 96   template<typename _Iterator>
 97     class reverse_iterator
 98     : public iterator<typename iterator_traits<_Iterator>::iterator_category,
 99               typename iterator_traits<_Iterator>::value_type,
100               typename iterator_traits<_Iterator>::difference_type,
101               typename iterator_traits<_Iterator>::pointer,
102                       typename iterator_traits<_Iterator>::reference>
103     {
104     protected:
105       _Iterator current;
106 
107       typedef iterator_traits<_Iterator>        __traits_type;
108 
109     public:
110       typedef _Iterator                 iterator_type;
111       typedef typename __traits_type::difference_type   difference_type;
112       typedef typename __traits_type::pointer       pointer;
113       typedef typename __traits_type::reference     reference;
114 
115       /**
116        *  The default constructor value-initializes member @p current.
117        *  If it is a pointer, that means it is zero-initialized.
118       */
119       // _GLIBCXX_RESOLVE_LIB_DEFECTS
120       // 235 No specification of default ctor for reverse_iterator
121       reverse_iterator() : current() { }
122 
123       /**
124        *  This %iterator will move in the opposite direction that @p x does.
125       */
126       explicit
127       reverse_iterator(iterator_type __x) : current(__x) { }
128 
129       /**
130        *  The copy constructor is normal.
131       */
132       reverse_iterator(const reverse_iterator& __x)
133       : current(__x.current) { }
134 
135       /**
136        *  A %reverse_iterator across other types can be copied if the
137        *  underlying %iterator can be converted to the type of @c current.
138       */
139       template<typename _Iter>
140         reverse_iterator(const reverse_iterator<_Iter>& __x)
141     : current(__x.base()) { }
160       reference
161       operator*() const
162       {
163     _Iterator __tmp = current;
164     return *--__tmp; // 先减1
165       }

397   template<typename _Iterator>
398     inline reverse_iterator<_Iterator>
399     make_reverse_iterator(_Iterator __i)
400     { return reverse_iterator<_Iterator>(__i); }
```

## std::back_insert_iterator
底层是容器，将对 back_insert_iterator 的赋值操作转变为对容器 push_back 的操作。此外，只有 operator++()、operator++(int) 和 operator* 操作。
```
/// @@file bits/stl_iterator.h
414   template<typename _Container>
415     class back_insert_iterator
416     : public iterator<output_iterator_tag, void, void, void, void>
417     {
418     protected:
419       _Container* container;
420 
421     public:
422       /// A nested typedef for the type of whatever container you used.
423       typedef _Container          container_type;
424 
425       /// The only way to create this %iterator is with a container.
426       explicit
427       back_insert_iterator(_Container& __x) : container(&__x) { }

448       back_insert_iterator&
449       operator=(const typename _Container::value_type& __value)
450       {
451     container->push_back(__value);
452     return *this;
453       }

490   template<typename _Container>
491     inline back_insert_iterator<_Container>
492     back_inserter(_Container& __x)
493     { return back_insert_iterator<_Container>(__x); }
```

## std::front_insert_iterator
底层是容器，将对 front_insert_iterator 的赋值操作转变为对容器 push_front 的操作。此外，只有 operator++()、operator++(int) 和 operator* 操作。
```
/// @@file bits/stl_iterator.h
505   template<typename _Container>
506     class front_insert_iterator
507     : public iterator<output_iterator_tag, void, void, void, void>
508     {
509     protected:
510       _Container* container;
511 
512     public:
513       /// A nested typedef for the type of whatever container you used.
514       typedef _Container          container_type;
515 
516       /// The only way to create this %iterator is with a container.
517       explicit front_insert_iterator(_Container& __x) : container(&__x) { }

538       front_insert_iterator&
539       operator=(const typename _Container::value_type& __value)
540       {
541     container->push_front(__value);
542     return *this;
543       }

580   template<typename _Container>
581     inline front_insert_iterator<_Container>
582     front_inserter(_Container& __x)
583     { return front_insert_iterator<_Container>(__x); }
```

## std::insert_iterator
底层是容器，将对 insert_iterator 的赋值操作转变为对容器 insert 的操作，赋值完成后，insert_iterator 中的 iter 迭代器指向新插入元素的下一个元素。此外，只有 operator++()、operator++(int) 和 operator* 操作。
```
599   template<typename _Container>
600     class insert_iterator
601     : public iterator<output_iterator_tag, void, void, void, void>
602     {
603     protected:
604       _Container* container;
605       typename _Container::iterator iter;
606 
607     public:
608       /// A nested typedef for the type of whatever container you used.
609       typedef _Container          container_type;
610 
611       /**
612        *  The only way to create this %iterator is with a container and an
613        *  initial position (a normal %iterator into the container).
614       */
615       insert_iterator(_Container& __x, typename _Container::iterator __i)
616       : container(&__x), iter(__i) {}

650       insert_iterator&
651       operator=(const typename _Container::value_type& __value)
652       {
653     iter = container->insert(iter, __value);
654     ++iter; // 移动
655     return *this;
656       }

694   template<typename _Container, typename _Iterator>
695     inline insert_iterator<_Container>
696     inserter(_Container& __x, _Iterator __i)
697     {
698       return insert_iterator<_Container>(__x,
699                      typename _Container::iterator(__i));
700     }
```

## std::move_iterator
除了它的 [] 运算符隐式地将底层迭代器的解引用运算符返回的值转换为右值引用，其他具有与底层迭代器相同的行为。可以使用移动迭代器调用一些通用算法来替换使用移动的复制。
```
/// @@file bits/stl_iterator.h
 940 namespace std _GLIBCXX_VISIBILITY(default)
 941 {
 958   template<typename _Iterator>
 959     class move_iterator
 960     {
 961     protected:
 962       _Iterator _M_current;
 963 
 964       typedef iterator_traits<_Iterator>        __traits_type;
 965       typedef typename __traits_type::reference     __base_ref;
 966 
 967     public:
 968       typedef _Iterator                 iterator_type;
 969       typedef typename __traits_type::iterator_category iterator_category;
 970       typedef typename __traits_type::value_type    value_type;
 971       typedef typename __traits_type::difference_type   difference_type;
 972       // NB: DR 680.
 973       typedef _Iterator                 pointer;
 974       // _GLIBCXX_RESOLVE_LIB_DEFECTS
 975       // 2106. move_iterator wrapping iterators returning prvalues
 976       typedef typename conditional<is_reference<__base_ref>::value,
 977              typename remove_reference<__base_ref>::type&&,
 978              __base_ref>::type      reference;
 979 
 980       move_iterator()
 981       : _M_current() { }
 982 
 983       explicit
 984       move_iterator(iterator_type __i)
 985       : _M_current(__i) { }
 986 
 987       template<typename _Iter>
 988     move_iterator(const move_iterator<_Iter>& __i)
 989     : _M_current(__i.base()) { }

1055       reference
1056       operator[](difference_type __n) const
1057       { return std::move(_M_current[__n]); } // 返回右值
1058     };

1156   template<typename _Iterator>
1157     inline move_iterator<_Iterator>
1158     make_move_iterator(_Iterator __i)
1159     { return move_iterator<_Iterator>(__i); } // 返回适配器

1172 } // namespace
```