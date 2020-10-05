## STEP2、若STEP1失败，调用 cache_alloc_refill()
在第一次申请对象时，force_refill 一定是 false，我们就按照这个分析。可以看到传入参数 force_refill 为 true 的话，直接跳转到 force_grow 进行申请页面，分割对象。
```
/// @file mm/slab.c
2901 static void *cache_alloc_refill(struct kmem_cache *cachep, gfp_t flags,
2902                             bool force_refill)
2903 {
2904     int batchcount;
2905     struct kmem_cache_node *n;
2906     struct array_cache *ac;
2907     int node;
2908 
2909     check_irq_off();
2910     node = numa_mem_id();
2911     if (unlikely(force_refill)) // unlikely表示不执行if的可能性大
2912         goto force_grow; // 强制填充
```
retry 是分配对象完成的地方，force_grow 只是申请页面，分割出对象，不会执行分配对象的操作。retry会尝试从其他地方转移一部分（数目为 batchcount）的对象到 array_cache 。第一步是尝试从 array_cache::shared 上获取一部分对象，根据前面的分析 shared 为空。
```
/// @file mm/slab.c
2913 retry:
2914     ac = cpu_cache_get(cachep);
2915     batchcount = ac->batchcount;
2916     if (!ac->touched && batchcount > BATCHREFILL_LIMIT) {
2917         /*
2918          * If there was little recent activity on this cache, then
2919          * perform only a partial refill.  Otherwise we could generate
2920          * refill bouncing.
2921          */
2922         batchcount = BATCHREFILL_LIMIT; // 16
2923     }
2924     n = cachep->node[node];
2925 
2926     BUG_ON(ac->avail > 0 || !n);
2927     spin_lock(&n->list_lock);
2928 
2929     /* See if we can refill from the shared array */
2930     if (n->shared && transfer_objects(ac, n->shared, batchcount)) {
2931         n->shared->touched = 1;
2932         goto alloc_done;
2933     }
```
接下来尝试从 slabs_partial 和 slabs_free 链表上 batchcount 个对象。
```
/// @file mm/slab.c
2935     while (batchcount > 0) {
2936         struct list_head *entry;
2937         struct page *page;
2938         /* Get slab alloc is to come from. */
2939         entry = n->slabs_partial.next; // 从slabs_partial获取
2940         if (entry == &n->slabs_partial) { // slabs_partial为空
2941             n->free_touched = 1;
2942             entry = n->slabs_free.next;//从slabs_free获取
2943             if (entry == &n->slabs_free) // slabs_free为空
2944                 goto must_grow; // 必须增长，第一次申请的时候必定跳转
2945         }
2946         // 通过entry获取struct page的地址
2947         page = list_entry(entry, struct page, lru);
2948         check_spinlock_acquired(cachep);
2949 
2950         /*
2951          * The slab was either on partial or free list so
2952          * there must be at least one object available for
2953          * allocation.
2954          */
2955         BUG_ON(page->active >= cachep->num);
2956         // 转移对象
2957         while (page->active < cachep->num && batchcount--) {
2958             STATS_INC_ALLOCED(cachep);
2959             STATS_INC_ACTIVE(cachep);
2960             STATS_SET_HIGH(cachep);
2961 
2962             ac_put_obj(cachep, ac, slab_get_obj(cachep, page,
2963                                     node));
2964         }
2965 
2966         /* move slabp to correct slabp list: */
2967         list_del(&page->lru);
2968         if (page->active == cachep->num)
2969             list_add(&page->lru, &n->slabs_full);
2970         else
2971             list_add(&page->lru, &n->slabs_partial);
2972     }
2973 
2974 must_grow: // 第一次申请必定跳转到这里
2975     n->free_objects -= ac->avail;
2976 alloc_done:
2977     spin_unlock(&n->list_lock);
2978 
2979     if (unlikely(!ac->avail)) { // 再次尝试
2980         int x;
2981 force_grow: // 申请页面
2982         x = cache_grow(cachep, flags | GFP_THISNODE, node, NULL);
2983 
2984         /* cache_grow can reenable interrupts, then ac could change. */
2985         ac = cpu_cache_get(cachep);
2986         node = numa_mem_id();
2987 
2988         /* no objects in sight? abort */
2989         if (!x && (ac->avail == 0 || force_refill))
2990             return NULL;
2991 
2992         if (!ac->avail)     /* objects refilled by interrupt? */
2993             goto retry;
2994     }
2995     ac->touched = 1;
2996 
2997     return ac_get_obj(cachep, ac, flags, force_refill);
2998 }
```