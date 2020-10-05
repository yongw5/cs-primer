## std::search()
```
/// @file bits/stl_algo.h
4020   template<typename _ForwardIterator1, typename _ForwardIterator2>
4021     inline _ForwardIterator1
4022     search(_ForwardIterator1 __first1, _ForwardIterator1 __last1,
4023            _ForwardIterator2 __first2, _ForwardIterator2 __last2)
4024     {
4025       // concept requirements
4026       __glibcxx_function_requires(_ForwardIteratorConcept<_ForwardIterator1>)
4027       __glibcxx_function_requires(_ForwardIteratorConcept<_ForwardIterator2>)
4028       __glibcxx_function_requires(_EqualOpConcept<
4029             typename iterator_traits<_ForwardIterator1>::value_type,
4030             typename iterator_traits<_ForwardIterator2>::value_type>)
4031       __glibcxx_requires_valid_range(__first1, __last1);
4032       __glibcxx_requires_valid_range(__first2, __last2);
4033 
4034       return std::__search(__first1, __last1, __first2, __last2,
4035                            __gnu_cxx::__ops::__iter_equal_to_iter());
4036     }
4037 
4059   template<typename _ForwardIterator1, typename _ForwardIterator2,
4060            typename _BinaryPredicate>
4061     inline _ForwardIterator1
4062     search(_ForwardIterator1 __first1, _ForwardIterator1 __last1,
4063            _ForwardIterator2 __first2, _ForwardIterator2 __last2,
4064            _BinaryPredicate  __predicate)
4065     {
4066       // concept requirements
4067       __glibcxx_function_requires(_ForwardIteratorConcept<_ForwardIterator1>)
4068       __glibcxx_function_requires(_ForwardIteratorConcept<_ForwardIterator2>)
4069       __glibcxx_function_requires(_BinaryPredicateConcept<_BinaryPredicate,
4070             typename iterator_traits<_ForwardIterator1>::value_type,
4071             typename iterator_traits<_ForwardIterator2>::value_type>)
4072       __glibcxx_requires_valid_range(__first1, __last1);
4073       __glibcxx_requires_valid_range(__first2, __last2);
4074 
4075       return std::__search(__first1, __last1, __first2, __last2,
4076                            __gnu_cxx::__ops::__iter_comp_iter(__predicate));
4077     }
```
调用 \__search()
```
/// @file bits/stl_algo.h
202   template<typename _ForwardIterator1, typename _ForwardIterator2,
203            typename _BinaryPredicate>
204     _ForwardIterator1
205     __search(_ForwardIterator1 __first1, _ForwardIterator1 __last1,
206              _ForwardIterator2 __first2, _ForwardIterator2 __last2,
207              _BinaryPredicate  __predicate)
208     {
209       // Test for empty ranges
210       if (__first1 == __last1 || __first2 == __last2)
211         return __first1;
212 
213       // Test for a pattern of length 1.
214       _ForwardIterator2 __p1(__first2);
215       if (++__p1 == __last2)
216         return std::__find_if(__first1, __last1,
217                 __gnu_cxx::__ops::__iter_comp_iter(__predicate, __first2));
218 
219       // General case.
220       _ForwardIterator2 __p;
221       _ForwardIterator1 __current = __first1;
222 
223       for (;;)
224         {
225           __first1 =
226             std::__find_if(__first1, __last1,
227                 __gnu_cxx::__ops::__iter_comp_iter(__predicate, __first2));
228 
229           if (__first1 == __last1)
230             return __last1;
231 
232           __p = __p1;
233           __current = __first1;
234           if (++__current == __last1)
235             return __last1;
236 
237           while (__predicate(__current, __p))
238             {
239               if (++__p == __last2)
240                 return __first1;
241               if (++__current == __last1)
242                 return __last1;
243             }
244           ++__first1;
245         }
246       return __first1;
247     }
```