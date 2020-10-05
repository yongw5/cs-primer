## 释放对象 kmem_cache_free()
首先是根据obj的地址找到所属的 cache ，这时通过 cache_from_obj() 实现的。基本的流程是先通过 obj 的地址找到所属的页面 page，通过 page->slab_cache 找到所属的 cache。然后调用 __cache_free() 释放对象。
```
/// @file mm/slab.c
3703 void kmem_cache_free(struct kmem_cache *cachep, void *objp)
3704 {
3705     unsigned long flags;
3706     cachep = cache_from_obj(cachep, objp); // 根据obj找到所属的cachep
3707     if (!cachep)
3708         return;
3709 
3710     local_irq_save(flags);
3711     debug_check_no_locks_freed(objp, cachep->object_size);
3712     if (!(cachep->flags & SLAB_DEBUG_OBJECTS))
3713         debug_check_no_obj_freed(objp, cachep->object_size);
3714     __cache_free(cachep, objp, _RET_IP_); // 释放工作
3715     local_irq_restore(flags);
3716 
3717     trace_kmem_cache_free(_RET_IP_, objp);
3718 }
```

### __cache_free()
当 array_cache 还有空闲时，调用 ac_put_obj() 直接放入。否则要先刷新 cache_flusharray()，再放入。
```
/// @file mm/slab.c
3510 static inline void __cache_free(struct kmem_cache *cachep, void *objp,
3511                 unsigned long caller)
3512 {
3513     struct array_cache *ac = cpu_cache_get(cachep);
3514 
3515     check_irq_off();
3516     kmemleak_free_recursive(objp, cachep->flags);
3517     objp = cache_free_debugcheck(cachep, objp, caller);
3518 
3519     kmemcheck_slab_free(cachep, objp, cachep->object_size);
3520 
3521     /*
3522      * Skip calling cache_free_alien() when the platform is not numa.
3523      * This will avoid cache misses that happen while accessing slabp (which
3524      * is per page memory  reference) to get nodeid. Instead use a global
3525      * variable to skip the call, which is mostly likely to be present in
3526      * the cache.
3527      */
3528     if (nr_online_nodes > 1 && cache_free_alien(cachep, objp))
3529         return;
3530 
3531     if (likely(ac->avail < ac->limit)) {
3532         STATS_INC_FREEHIT(cachep); // 空函数
3533     } else { // array_cache中的对象数量达到最大值
3534         STATS_INC_FREEMISS(cachep);
3535         cache_flusharray(cachep, ac); // 刷新array_cache
3536     }
3537 
3538     ac_put_obj(cachep, ac, objp);
3539 }
```

### cache_flusharray()
```
/// @file mm/slab.c
3455 static void cache_flusharray(struct kmem_cache *cachep, struct array_cache *ac)
3456 {   
3457     int batchcount;
3458     struct kmem_cache_node *n;
3459     int node = numa_mem_id();
3460     
3461     batchcount = ac->batchcount;
3462 #if DEBUG
3463     BUG_ON(!batchcount || batchcount > ac->avail);
3464 #endif
3465     check_irq_off();
3466     n = cachep->node[node];
3467     spin_lock(&n->list_lock);
3468     if (n->shared) { // 首先尝试转移到shared
3469         struct array_cache *shared_array = n->shared;
3470         int max = shared_array->limit - shared_array->avail;
3471         if (max) {
3472             if (batchcount > max)
3473                 batchcount = max;
3474             memcpy(&(shared_array->entry[shared_array->avail]),
3475                    ac->entry, sizeof(void *) * batchcount);
3476             shared_array->avail += batchcount;
3477             goto free_done;
3478         }
3479     }
3480     
3481     free_block(cachep, ac->entry, batchcount, node); // 主要工作
3482 free_done:
3483 #if STATS // 忽略
3484     {
3485         int i = 0;
3486         struct list_head *p;
3487 
3488         p = n->slabs_free.next;
3489         while (p != &(n->slabs_free)) {
3490             struct page *page;
3491             
3492             page = list_entry(p, struct page, lru);
3493             BUG_ON(page->active);
3494 
3495             i++;
3496             p = p->next;
3497         }   
3498         STATS_SET_FREEABLE(cachep, i);
3499     }   
3500 #endif  
3501     spin_unlock(&n->list_lock);
3502     ac->avail -= batchcount; // 修复array_cache
3503     memmove(ac->entry, &(ac->entry[batchcount]), sizeof(void *)*ac->avail);
3504 }
```

### free_block()
将 nr_objects 个对象放回所属的页面中
```
/// @file mm/slab.c
3410 static void free_block(struct kmem_cache *cachep, void **objpp, int nr_objects,
3411                int node)
3412 {   
3413     int i; 
3414     struct kmem_cache_node *n;
3415     
3416     for (i = 0; i < nr_objects; i++) {
3417         void *objp; 
3418         struct page *page;
3419 
3420         clear_obj_pfmemalloc(&objpp[i]); // 清除一些标志
3421         objp = objpp[i];
3422 
3423         page = virt_to_head_page(objp); // objp ==> page
3424         n = cachep->node[node];
3425         list_del(&page->lru);
3426         check_spinlock_acquired_node(cachep, node);
3427         slab_put_obj(cachep, page, objp, node); // 还会 obj
3428         STATS_DEC_ACTIVE(cachep);
3429         n->free_objects++; // 可用对象递增
3430 
3431         /* fixup slab chains */
3432         if (page->active == 0) { // page的对象全部回收
3433             if (n->free_objects > n->free_limit) { // node管理的超过阈值
3434                 n->free_objects -= cachep->num;
3435                 /* No need to drop any previously held
3436                  * lock here, even if we have a off-slab slab
3437                  * descriptor it is guaranteed to come from
3438                  * a different cache, refer to comments before
3439                  * alloc_slabmgmt.
3440                  */
3441                 slab_destroy(cachep, page); // 释放页面
3442             } else { // 否则挂到空闲链表
3443                 list_add(&page->lru, &n->slabs_free);
3444             }
3445         } else { // 挂到部分链表
3446             /* Unconditionally move a slab to the end of the
3447              * partial list on free - maximum time for the
3448              * other objects to be freed, too.
3449              */
3450             list_add_tail(&page->lru, &n->slabs_partial);
3451         }
3452     }
3453 }
```
slab_put_obj() 的工作很简单
```
/// @file mm/slab.c
2699 static void slab_put_obj(struct kmem_cache *cachep, struct page *page,
2700                 void *objp, int nodeid)
2701 {
2702     unsigned int objnr = obj_to_index(cachep, page, objp);
2703 #if DEBUG
/// ..
2717 #endif
2718     page->active--;
2719     set_free_obj(page, page->active, objnr);
2720 }
```
slab_destroy() 的工作是释放页面和 slab 管理结构（如果在外部）
```
/// @file mm/slab.c
2034 static void slab_destroy(struct kmem_cache *cachep, struct page *page)
2035 {
2036     void *freelist;
2037 
2038     freelist = page->freelist;
2039     slab_destroy_debugcheck(cachep, page);
2040     if (unlikely(cachep->flags & SLAB_DESTROY_BY_RCU)) {
2041         struct rcu_head *head;
2049         head = (void *)&page->rcu_head;
2050         call_rcu(head, kmem_rcu_free);
2051 
2052     } else {
2053         kmem_freepages(cachep, page); // 回收页面
2054     }
2055 
2060     if (OFF_SLAB(cachep)) // 释放slab管理结构
2061         kmem_cache_free(cachep->freelist_cache, freelist);
2062 }
```