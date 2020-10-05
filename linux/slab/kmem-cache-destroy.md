## SLAB 分配器释放
### kmem_cache_destroy()
kmem_cache_destroy() 的工作是释放一个 cache
```
/// @file mm/slab_common.c
335 void kmem_cache_destroy(struct kmem_cache *s)
336 {
337     get_online_cpus();
338     get_online_mems();
339 
340     mutex_lock(&slab_mutex);
341 
342     s->refcount--; // 递减引用计数
343     if (s->refcount) // 仍然有其他用户
344         goto out_unlock;
345 
346     if (memcg_cleanup_cache_params(s) != 0)
347         goto out_unlock;
348 
349     if (__kmem_cache_shutdown(s) != 0) { // 直接返回0，不会执行
350         printk(KERN_ERR "kmem_cache_destroy %s: "
351                "Slab cache still has objects\n", s->name);
352         dump_stack();
353         goto out_unlock;
354     }
355 
356     list_del(&s->list); // 将kmem_cache对象从链表中取下
357 
358     mutex_unlock(&slab_mutex);
359     if (s->flags & SLAB_DESTROY_BY_RCU)
360         rcu_barrier();
361 
362     memcg_free_cache_params(s);
363 #ifdef SLAB_SUPPORTS_SYSFS
364     sysfs_slab_remove(s);
365 #else
366     slab_kmem_cache_release(s); // 释放工作发生的地方
367 #endif
368     goto out;
369 
370 out_unlock:
371     mutex_unlock(&slab_mutex);
372 out:
373     put_online_mems();
374     put_online_cpus();
375 }
```
slab_kmem_cache_release() 进行释放操作
```
/// @file mm/slab_common.c
329 void slab_kmem_cache_release(struct kmem_cache *s)
330 {
331     kfree(s->name); // 释放名字占用的空间
332     kmem_cache_free(kmem_cache, s); // 释放
333 }
```
需要注意的是， kmem_cache 是一个全局变量，指向一个 cache，这个 cache 管理分配 kmem_cache 对象。kmem_cache 对象的释放和普通对象的释放一样，kmem_cache_free() 的处理也一样。