## 分配对象 kmem_cache_alloc()
```
/// @file mm/slab.c
3549 void *kmem_cache_alloc(struct kmem_cache *cachep, gfp_t flags)
3550 {
3551     void *ret = slab_alloc(cachep, flags, _RET_IP_);
3552 
3553     trace_kmem_cache_alloc(_RET_IP_, ret,
3554                    cachep->object_size, cachep->size, flags);
3555 
3556     return ret;
3557 }
```
分配操作在 slab_alloc() 中完成
```
/// @file mm/slab.c
3374 static __always_inline void *
3375 slab_alloc(struct kmem_cache *cachep, gfp_t flags, unsigned long caller)
3376 {
3377     unsigned long save_flags;
3378     void *objp;
3379 
3380     flags &= gfp_allowed_mask;
3381 
3382     lockdep_trace_alloc(flags);
3383 
3384     if (slab_should_failslab(cachep, flags))
3385         return NULL;
3386 
3387     cachep = memcg_kmem_get_cache(cachep, flags); // return cachep
3388 
3389     cache_alloc_debugcheck_before(cachep, flags);
3390     local_irq_save(save_flags);
3391     objp = __do_cache_alloc(cachep, flags); // 分配对象
3392     local_irq_restore(save_flags);
3393     objp = cache_alloc_debugcheck_after(cachep, flags, objp, caller);
3394     kmemleak_alloc_recursive(objp, cachep->object_size, 1, cachep->flags,
3395                  flags);
3396     prefetchw(objp);
3397 
3398     if (likely(objp)) {
3399         kmemcheck_slab_alloc(cachep, flags, objp, cachep->object_size);
3400         if (unlikely(flags & __GFP_ZERO))
3401             memset(objp, 0, cachep->object_size);
3402     }
3403 
3404     return objp;
3405 }
```
\__do_cache_alloc() 继续调用 ____cache_alloc()
```
/// @file mm/slab.c
3366 static __always_inline void *
3367 __do_cache_alloc(struct kmem_cache *cachep, gfp_t flags)
3368 {
3369     return ____cache_alloc(cachep, flags);
3370 }
```
___cache_alloc() 的定义如下：
```
/// @file mm/slab.c
3070 static inline void *____cache_alloc(struct kmem_cache *cachep, gfp_t flags)
3071 {   
3072     void *objp;
3073     struct array_cache *ac;
3074     bool force_refill = false; // 预定不用填充（转移对象）
3075     
3076     check_irq_off();
3077     
3078     ac = cpu_cache_get(cachep); // cachep->array[0]
3079     if (likely(ac->avail)) { // 有可用对象
3080         ac->touched = 1; // 有过访问记录
3081         objp = ac_get_obj(cachep, ac, flags, false); // 返回一个对象
3082 
3083         /*
3084          * Allow for the possibility all avail objects are not allowed
3085          * by the current flags
3086          */
3087         if (objp) {
3088             STATS_INC_ALLOCHIT(cachep);
3089             goto out; // 返回
3090         } // 分配失败，需要填充对象
3091         force_refill = true; // 需要强制填充
3092     }
3093     // 没有可用对象，填充
3094     STATS_INC_ALLOCMISS(cachep);
3095     objp = cache_alloc_refill(cachep, flags, force_refill); // 填充
3096     /*
3097      * the 'ac' may be updated by cache_alloc_refill(),
3098      * and kmemleak_erase() requires its correct value.
3099      */
3100     ac = cpu_cache_get(cachep);
3101 
3102 out:
3103     /*
3104      * To avoid a false negative, if an object that is in one of the
3105      * per-CPU caches is leaked, we need to make sure kmemleak doesn't
3106      * treat the array pointers as a reference to the object.
3107      */
3108     if (objp)
3109         kmemleak_erase(&ac->entry[ac->avail]);
3110     return objp;
3111 }
```
从上面的分析可知，如果 array_cache 中有可用对象，直接从中获取。如果分配失败或者 array_cache 没有可用对象，需要调用 cache_alloc_refill() 进行处理。

## STEP1、从 array_cache 获取对象
调用 ac_get_obj() 获取一个对象，过程很简单。
```
/// @file mm/slab.c
902 static inline void *ac_get_obj(struct kmem_cache *cachep,
903             struct array_cache *ac, gfp_t flags, bool force_refill)
904 {
905     void *objp;
906 
907     if (unlikely(sk_memalloc_socks()))
908         objp = __ac_get_obj(cachep, ac, flags, force_refill);
909     else
910         objp = ac->entry[--ac->avail]; // 取一个对象并且递减可用对象计数ac->avail
911 
912     return objp; // 返回对象
913 }
```