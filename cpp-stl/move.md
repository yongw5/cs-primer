## std::move()
对迭代器的要求很低，其作用是将区间 [\__first, \__last) 的数据移动到 [\__result, \__result + \__last - \__frist)。底层会尽力调用 memmove()。可以处理区间重叠的情况。
```
/// @file bits/stl_algobase.h
491   template<typename _II, typename _OI>
492     inline _OI
493     move(_II __first, _II __last, _OI __result)
494     {
495       // concept requirements
496       __glibcxx_function_requires(_InputIteratorConcept<_II>)
497       __glibcxx_function_requires(_OutputIteratorConcept<_OI,
498             typename iterator_traits<_II>::value_type>)
499       __glibcxx_requires_valid_range(__first, __last);
500 
501       return std::__copy_move_a2<true>(std::__miter_base(__first),
502                                        std::__miter_base(__last), __result);
503     }
```

## std::move_backward()
move_backward() 传入的迭代器必须是双向迭代器（bidirectional iterator），作用是从最后一个元素开始将区间 [\__first, \__last) 的数据移动到区间 [\__result + \__first - \__last, \__result)，返回指向最后一个移动的元素的迭代器，即 \__result + \__first - \__last 的结果。move_backward() 底层函数会尽一切努力去调用 memmove() 函数。可以处理区间重叠的情况。
```
/// @file bits/stl_algobase.h
668   template<typename _BI1, typename _BI2>
669     inline _BI2
670     move_backward(_BI1 __first, _BI1 __last, _BI2 __result)
671     {
672       // concept requirements
673       __glibcxx_function_requires(_BidirectionalIteratorConcept<_BI1>)
674       __glibcxx_function_requires(_Mutable_BidirectionalIteratorConcept<_BI2>)
675       __glibcxx_function_requires(_ConvertibleConcept<
676             typename iterator_traits<_BI1>::value_type,
677             typename iterator_traits<_BI2>::value_type>)
678       __glibcxx_requires_valid_range(__first, __last);
679 
680       return std::__copy_move_backward_a2<true>(std::__miter_base(__first),
681                                                 std::__miter_base(__last),
682                                                 __result);
683     }
```