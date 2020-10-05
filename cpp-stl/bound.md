## std::lower_bound()
返回区间 [\__first，\__last) 中不小于（即大于或等于）\__val 的第一个元素的迭代器，如果没有找到这样的元素，则返回\__last。直接调用 \__lower_bound() 函数
```
/// @file bits/stl_algobase.h
 992   template<typename _ForwardIterator, typename _Tp>
 993     inline _ForwardIterator
 994     lower_bound(_ForwardIterator __first, _ForwardIterator __last,
 995                 const _Tp& __val)
 996     {
 997       // concept requirements
 998       __glibcxx_function_requires(_ForwardIteratorConcept<_ForwardIterator>)
 999       __glibcxx_function_requires(_LessThanOpConcept<
1000             typename iterator_traits<_ForwardIterator>::value_type, _Tp>)
1001       __glibcxx_requires_partitioned_lower(__first, __last, __val);
1002 
1003       return std::__lower_bound(__first, __last, __val,
1004                                 __gnu_cxx::__ops::__iter_less_val());
1005     }
```
\__lower_bound() 函数会根据不同迭代器类型进行优化。传入随机迭代器最后的算法是二分查找
```
/// @file bits/stl_algobase.h
954   template<typename _ForwardIterator, typename _Tp, typename _Compare>
955     _ForwardIterator
956     __lower_bound(_ForwardIterator __first, _ForwardIterator __last,
957                   const _Tp& __val, _Compare __comp)
958     {
959       typedef typename iterator_traits<_ForwardIterator>::difference_type
960         _DistanceType;
961       // 不同迭代器类型 distance性能不同
962       _DistanceType __len = std::distance(__first, __last);
963 
964       while (__len > 0)
965         {
966           _DistanceType __half = __len >> 1;
967           _ForwardIterator __middle = __first;
968           std::advance(__middle, __half); // 不同迭代器类型性能不同
969           if (__comp(__middle, __val))
970             {
971               __first = __middle;
972               ++__first;
973               __len = __len - __half - 1;
974             }
975           else
976             __len = __half;
977         }
978       return __first;
979     }
```

## std::upper_bound()
返回区间 [\__first，\__last) 中不大于（即小于或等于）\__val 的第一个元素的迭代器，如果没有找到这样的元素，则返回\__last。直接调用 \__upper_bound() 函数
```
/// @file bits/stl_algo.h
2035   template<typename _ForwardIterator, typename _Tp, typename _Compare>
2036     _ForwardIterator
2037     __upper_bound(_ForwardIterator __first, _ForwardIterator __last,
2038                   const _Tp& __val, _Compare __comp)
2039     {
2040       typedef typename iterator_traits<_ForwardIterator>::difference_type
2041         _DistanceType;
2042 
2043       _DistanceType __len = std::distance(__first, __last);
2044 
2045       while (__len > 0)
2046         {
2047           _DistanceType __half = __len >> 1;
2048           _ForwardIterator __middle = __first;
2049           std::advance(__middle, __half);
2050           if (__comp(__val, __middle))
2051             __len = __half;
2052           else
2053             {
2054               __first = __middle;
2055               ++__first;
2056               __len = __len - __half - 1;
2057             }
2058         }
2059       return __first;
2060     }
2061 
2073   template<typename _ForwardIterator, typename _Tp>
2074     inline _ForwardIterator
2075     upper_bound(_ForwardIterator __first, _ForwardIterator __last,
2076                 const _Tp& __val)
2077     {
2078       typedef typename iterator_traits<_ForwardIterator>::value_type
2079         _ValueType;
2080 
2081       // concept requirements
2082       __glibcxx_function_requires(_ForwardIteratorConcept<_ForwardIterator>)
2083       __glibcxx_function_requires(_LessThanOpConcept<_Tp, _ValueType>)
2084       __glibcxx_requires_partitioned_upper(__first, __last, __val);
2085 
2086       return std::__upper_bound(__first, __last, __val,
2087                                 __gnu_cxx::__ops::__val_less_iter());
2088     }
2089 
2105   template<typename _ForwardIterator, typename _Tp, typename _Compare>
2106     inline _ForwardIterator
2107     upper_bound(_ForwardIterator __first, _ForwardIterator __last,
2108                 const _Tp& __val, _Compare __comp)
2109     {
2110       typedef typename iterator_traits<_ForwardIterator>::value_type
2111         _ValueType;
2112 
2113       // concept requirements
2114       __glibcxx_function_requires(_ForwardIteratorConcept<_ForwardIterator>)
2115       __glibcxx_function_requires(_BinaryPredicateConcept<_Compare,
2116                                   _Tp, _ValueType>)
2117       __glibcxx_requires_partitioned_upper_pred(__first, __last,
2118                                                 __val, __comp);
2119 
2120       return std::__upper_bound(__first, __last, __val,
2121                                 __gnu_cxx::__ops::__val_comp_iter(__comp));
2122     }
```