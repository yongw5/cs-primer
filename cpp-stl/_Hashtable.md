## _Hashtable
_Hashtable 有很多模板参数，其含义如下：
- _Key：结点的键值域类型
- _Value：结点的数据树类型
- _Alloc：空间配置器
- _ExtractKey：从结点中取出键值的方法（函数或者仿函数）
- _Equal：判断键值是否相同的方法
- _H1：哈希函数，调用形式 size_t(_Key)
- _H2：范围哈希函数，调用形式 size_t(size_t, size_t)，返回指定范围的哈希值
- _Hash：范围哈希函数，调用形式 size_t(size_t, size_t)，返回指定范围的哈希值。默认为 \_H2(_H1(k), N)，如果指定了这个哈希函数，\_H1 和 \_H2 将忽略
- _RehashPolicy：当冲突过多，需要更大的空间时间，需要此类中计算出新的bucket的大小。三个有用的成员函数：
  - \_M_next_bkt(n)：返回一个不小于 n 的 bucket count
  - \_M_bkt_for_elements(n)：返回一个在 n 附近的 bucket count
  - \_M_need_rehash(n_bkt, n_elt, n_ins)：判断是否需要再哈希，是返回 make_pair(true, n)，否返回make_pair(false, <anything>)
- _Traits：类型萃取机

```
/// @file bits/hashtable.h
166   template<typename _Key, typename _Value, typename _Alloc,
167            typename _ExtractKey, typename _Equal,
168            typename _H1, typename _H2, typename _Hash,
169            typename _RehashPolicy, typename _Traits>
170     class _Hashtable
171     : public __detail::_Hashtable_base<_Key, _Value, _ExtractKey, _Equal,
172                                        _H1, _H2, _Hash, _Traits>,
173       public __detail::_Map_base<_Key, _Value, _Alloc, _ExtractKey, _Equal,
174                                  _H1, _H2, _Hash, _RehashPolicy, _Traits>,
175       public __detail::_Insert<_Key, _Value, _Alloc, _ExtractKey, _Equal,
176                                _H1, _H2, _Hash, _RehashPolicy, _Traits>,
177       public __detail::_Rehash_base<_Key, _Value, _Alloc, _ExtractKey, _Equal,
178                                     _H1, _H2, _Hash, _RehashPolicy, _Traits>,
179       public __detail::_Equality<_Key, _Value, _Alloc, _ExtractKey, _Equal,
180                                  _H1, _H2, _Hash, _RehashPolicy, _Traits>,
181       private __detail::_Hashtable_alloc<
182         typename __alloctr_rebind<_Alloc,
183           __detail::_Hash_node<_Value,
184                                _Traits::__hash_cached::value> >::__type
```
注意的是 \_Hashtable 没有选定默认空间配置器，使用的是 \_Hashtable_alloc 分配器，该分配器是对传入的分配器的包装。其基类 \__detail::_Hashtable_base 主要定义了迭代器。\_Hashtable 中类型相关的定义如下
```
/// @file bits/hashtable.h
201     public:
202       typedef _Key                                              key_type;
203       typedef _Value                                            value_type;
204       typedef _Alloc                                            allocator_type;
205       typedef _Equal                                            key_equal;
206 
207       // mapped_type, if present, comes from _Map_base.
208       // hasher, if present, comes from _Hash_code_base/_Hashtable_base.
209       typedef typename __value_alloc_traits::pointer            pointer;
210       typedef typename __value_alloc_traits::const_pointer      const_pointer;
211       typedef value_type&                                       reference;
212       typedef const value_type&                                 const_reference;

301     public:
302       using size_type = typename __hashtable_base::size_type;
303       using difference_type = typename __hashtable_base::difference_type;
304 
305       using iterator = typename __hashtable_base::iterator;
306       using const_iterator = typename __hashtable_base::const_iterator;
307 
308       using local_iterator = typename __hashtable_base::local_iterator;
309       using const_local_iterator = typename __hashtable_base::
310 				   const_local_iterator;
```
其中迭代器是 \__detail::_Node_iterator<value_type, \__constant_iterators::value, \__hash_cached::value>，是一个 forward_iterator_tag 迭代器。_Hashtable 的数据结构相关的定义如下：
```
/// @file bits/hashtable.h
312     private:
313       __bucket_type*            _M_buckets              = &_M_single_bucket;
314       size_type                 _M_bucket_count         = 1;
315       __node_base               _M_before_begin;
316       size_type                 _M_element_count        = 0;
317       _RehashPolicy             _M_rehash_policy;
325       __bucket_type             _M_single_bucket        = nullptr;
```
其中，\__node_base 的定义如下：
```
/// @file bits/hashtable_policy.h
227   struct _Hash_node_base
228   {
229     _Hash_node_base* _M_nxt;
230 
231     _Hash_node_base() noexcept : _M_nxt() { }
232 
233     _Hash_node_base(_Hash_node_base* __next) noexcept : _M_nxt(__next) { }
234   };
```
\__bucket_type 是 \_node_base* 的别名。每个结点的数类类型为 \_Hash_node，定义如下：
```
/// @file bits/hashtable_policy.h
276   template<typename _Value>
277     struct _Hash_node<_Value, true> : _Hash_node_value_base<_Value>
278     {
279       std::size_t  _M_hash_code;
280 
281       _Hash_node*
282       _M_next() const noexcept
283       { return static_cast<_Hash_node*>(this->_M_nxt); }
284     };

291   template<typename _Value>
292     struct _Hash_node<_Value, false> : _Hash_node_value_base<_Value>
293     {
294       _Hash_node*
295       _M_next() const noexcept
296       { return static_cast<_Hash_node*>(this->_M_nxt); }
297     };
```
如果指定缓冲哈希值，会有一个 \_M_hash_code。\_Hash_node 继承于 \_Hash_node_value_base，定义如下：
```
/// @file bits/hashtable_policy.h
241   template<typename _Value>
242     struct _Hash_node_value_base : _Hash_node_base
243     {
244       typedef _Value value_type;
245 
246       __gnu_cxx::__aligned_buffer<_Value> _M_storage;
247 
248       _Value*
249       _M_valptr() noexcept
250       { return _M_storage._M_ptr(); }
251 
252       const _Value*
253       _M_valptr() const noexcept
254       { return _M_storage._M_ptr(); }
255 
256       _Value&
257       _M_v() noexcept
258       { return *_M_valptr(); }
259 
260       const _Value&
261       _M_v() const noexcept
262       { return *_M_valptr(); }
263     };
```
因此每个 \_Hash_node 结点中的数据成员：
```
_Hash_node_base* _M_nxt;
__gnu_cxx::__aligned_buffer<_Value> _M_storage;
std::size_t  _M_hash_code; //        _M_hash_code if cache_hash_code is true
```

## \_M_insert_unique_node 实例
传入的参数包括bucket的序号 \__bkt，哈希值 \__code 以及待插入结点 \__node。返回指向插入结点的迭代器。
```
/// @file bits/hashtable.h
1581   template<typename _Key, typename _Value,
1582            typename _Alloc, typename _ExtractKey, typename _Equal,
1583            typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
1584            typename _Traits>
1585     auto
1586     _Hashtable<_Key, _Value, _Alloc, _ExtractKey, _Equal,
1587                _H1, _H2, _Hash, _RehashPolicy, _Traits>::
1588     _M_insert_unique_node(size_type __bkt, __hash_code __code,
1589                           __node_type* __node)
1590     -> iterator
1591     {
```
首先是判断是否需要进行扩容，再哈希，调用 \_Prime_rehash_policy::\_M_need_rehash 进行判断是否需要再哈希，是返回 make_pair(true, n)，否返回 make_pair(false, <anything>)
```
/// @file bits/hashtable.h
1592       const __rehash_state& __saved_state = _M_rehash_policy._M_state();
1593       std::pair<bool, std::size_t> __do_rehash
1594         = _M_rehash_policy._M_need_rehash(_M_bucket_count, _M_element_count, 1);
```
\_M_need_rehash 参数含义：
- \_\_n_bkt 表示桶（bucket）的总数量；
- \_\_n_elt 表示当前桶（将元素插入进去）的元素个数；
- \_\_n_ins 表示插入的元素的个数；
- \_Prime_rehash_policy::\_M_next_resize 是一个阈值，表示单个桶能够储存元素的上界，由 \_M_max_load_factor 控制。如果某个桶的元素个数达到这个阈值，就可能需要扩容；
- \_M_max_load_factor 表示一个负载系数上限，表示单个桶的元素个数与桶的个数的最大比例（默认是1）。因此，默认情况下，某个桶的元素个数大于等于桶的个数，需要扩容。

由于 \_M_max_load_factor 是可以改变的（例如 std::unordered_set::max_load_factor(float ml) 就可以改变该值），所以即使超过 \__M\_next_resize，也不一定扩容。只有 \__min_bkts >= \__n\_bkt 才真正需要扩容，调用 \_M_next_bkt 返回一个桶的数量。\_S_growth_factor 是一个常量，大小为 2，所有扩容大致上是二倍增长。
```
/// src/c++11/hashtable-c++0x.cc
75   std::pair<bool, std::size_t>
76   _Prime_rehash_policy::
77   _M_need_rehash(std::size_t __n_bkt, std::size_t __n_elt,
78          std::size_t __n_ins) const
79   {
80     if (__n_elt + __n_ins >= _M_next_resize)
81       {
82     long double __min_bkts = (__n_elt + __n_ins)
83                    / (long double)_M_max_load_factor;
84     if (__min_bkts >= __n_bkt)
85       return std::make_pair(true,
86         _M_next_bkt(std::max<std::size_t>(__builtin_floor(__min_bkts) + 1,
87                           __n_bkt * _S_growth_factor)));
88     // _M_max_load_factor可能改变，所有每次需要更新_M_next_resize
89     _M_next_resize
90       = __builtin_floor(__n_bkt * (long double)_M_max_load_factor);
91     return std::make_pair(false, 0);
92       }
93     else
94       return std::make_pair(false, 0);
95   }
```

如果需要扩容，先扩容，（\_M_rehash 调用）\_M_rehash_aux() 函数申请新空间并转移元素。
```
/// @file bits/hashtable.h
1596       __try
1597         {
1598           if (__do_rehash.first)
1599             {
1600               _M_rehash(__do_rehash.second, __saved_state);
1601               __bkt = _M_bucket_index(this->_M_extract()(__node->_M_v()), __code);
1602             }
```
\_M_rehash_aux() 输入参数两个，\__n 表示大小，另一个表示键值是否唯一（分别对应两个函数\_M_rehash_aux(size_type \__n, std::true_type) 和 \_M_rehash_aux(size_type \__n, std::false_type)）
```
/// @file bits/hashtable.h
1965   template<typename _Key, typename _Value,
1966            typename _Alloc, typename _ExtractKey, typename _Equal,
1967            typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
1968            typename _Traits>
1969     void
1970     _Hashtable<_Key, _Value, _Alloc, _ExtractKey, _Equal,
1971                _H1, _H2, _Hash, _RehashPolicy, _Traits>::
1972     _M_rehash_aux(size_type __n, std::true_type)
1973     {
1974       __bucket_type* __new_buckets = _M_allocate_buckets(__n);
1975       __node_type* __p = _M_begin();
1976       _M_before_begin._M_nxt = nullptr;
1977       std::size_t __bbegin_bkt = 0;
1978       while (__p)
1979         {
1980           __node_type* __next = __p->_M_next();
1981           std::size_t __bkt = __hash_code_base::_M_bucket_index(__p, __n);
1982           if (!__new_buckets[__bkt])
1983             {
1984               __p->_M_nxt = _M_before_begin._M_nxt;
1985               _M_before_begin._M_nxt = __p;
1986               __new_buckets[__bkt] = &_M_before_begin;
1987               if (__p->_M_nxt)
1988                 __new_buckets[__bbegin_bkt] = __p;
1989               __bbegin_bkt = __bkt;
1990             }
1991           else
1992             {
1993               __p->_M_nxt = __new_buckets[__bkt]->_M_nxt;
1994               __new_buckets[__bkt]->_M_nxt = __p;
1995             }
1996           __p = __next;
1997         }
1998 
1999       _M_deallocate_buckets();
2000       _M_bucket_count = __n;
2001       _M_buckets = __new_buckets;
2002     }
```
然后进行插入 \_M_insert_bucket_begin，总是在链表头部插入。这个过程如果有异常产生，插入失败，\__node 会被释放
```
/// @file bits/hashtable.h
1604           this->_M_store_code(__node, __code);
1605 
1606           // Always insert at the beginning of the bucket.
1607           _M_insert_bucket_begin(__bkt, __node);
1608           ++_M_element_count;
1609           return iterator(__node);
1610         }
1611       __catch(...)
1612         {
1613           this->_M_deallocate_node(__node);
1614           __throw_exception_again;
1615         }
1616     }
```