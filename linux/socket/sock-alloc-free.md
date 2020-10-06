## sk_alloc()
调用 sk_prot_alloc() 分配一个传输控制块，然后进行必要的构造。
```
/// @file net/core/sock.c
1403 struct sock *sk_alloc(struct net *net, int family, gfp_t priority,
1404               struct proto *prot)
1405 {
1406     struct sock *sk;
1407 
1408     sk = sk_prot_alloc(prot, priority | __GFP_ZERO, family); // 分配
1409     if (sk) { // 构造
1410         sk->sk_family = family;
1411         /*
1412          * See comment in struct sock definition to understand
1413          * why we need sk_prot_creator -acme
1414          */
1415         sk->sk_prot = sk->sk_prot_creator = prot;
1416         sock_lock_init(sk);
1417         sock_net_set(sk, get_net(net));
1418         atomic_set(&sk->sk_wmem_alloc, 1);
1419 
1420         sock_update_classid(sk);
1421         sock_update_netprioidx(sk);
1422     }
1423 
1424     return sk;
1425 }
```
sk_prot_alloc() 会根据传入的 prot 参数，决定从何处分配一个什么类型（或大小）的传输控制块。比如说传入指向全局变量 tcp_prot 的指针，就会创建一个 tcp_sock 对象
```
/// @file net/core/sock.c
1326 static struct sock *sk_prot_alloc(struct proto *prot, gfp_t priority,
1327         int family)
1328 {
1329     struct sock *sk;
1330     struct kmem_cache *slab;
1331 
1332     slab = prot->slab; // SLAB 分配器
1333     if (slab != NULL) { // 存在 SLAB 分配器，从 SLAB 中分配一个
1334         sk = kmem_cache_alloc(slab, priority & ~__GFP_ZERO);
1335         if (!sk)
1336             return sk;
1337         if (priority & __GFP_ZERO) {
1338             if (prot->clear_sk)
1339                 prot->clear_sk(sk, prot->obj_size);
1340             else
1341                 sk_prot_clear_nulls(sk, prot->obj_size);
1342         }
1343     } else // 不存在的话，用 kmalloc() 函数申请一个
1344         sk = kmalloc(prot->obj_size, priority);
1345 
1346     if (sk != NULL) { // 赋值
1347         kmemcheck_annotate_bitfield(sk, flags);
1348 
1349         if (security_sk_alloc(sk, family, priority))
1350             goto out_free;
1351 
1352         if (!try_module_get(prot->owner))
1353             goto out_free_sec;
1354         sk_tx_queue_clear(sk);
1355     }
1356 
1357     return sk;
1358 
1359 out_free_sec:
1360     security_sk_free(sk);
1361 out_free:
1362     if (slab != NULL)
1363         kmem_cache_free(slab, sk);
1364     else
1365         kfree(sk);
1366     return NULL;
1367 }
```

## sk_free()
```
/// @file net/core/sock.c
1428 static void __sk_free(struct sock *sk)
1429 {
1430     struct sk_filter *filter;
1431 
1432     if (sk->sk_destruct)
1433         sk->sk_destruct(sk);
1434 
1435     filter = rcu_dereference_check(sk->sk_filter,
1436                        atomic_read(&sk->sk_wmem_alloc) == 0);
1437     if (filter) {
1438         sk_filter_uncharge(sk, filter);
1439         RCU_INIT_POINTER(sk->sk_filter, NULL);
1440     }
1441 
1442     sock_disable_timestamp(sk, SK_FLAGS_TIMESTAMP);
1443 
1444     if (atomic_read(&sk->sk_omem_alloc))
1445         pr_debug("%s: optmem leakage (%d bytes) detected\n",
1446              __func__, atomic_read(&sk->sk_omem_alloc));
1447 
1448     if (sk->sk_frag.page) {
1449         put_page(sk->sk_frag.page);
1450         sk->sk_frag.page = NULL;
1451     }
1452 
1453     if (sk->sk_peer_cred)
1454         put_cred(sk->sk_peer_cred);
1455     put_pid(sk->sk_peer_pid);
1456     put_net(sock_net(sk));
1457     sk_prot_free(sk->sk_prot_creator, sk);
1458 }
1459 
1460 void sk_free(struct sock *sk)
1461 {
1462     /*
1463      * We subtract one from sk_wmem_alloc and can know if
1464      * some packets are still in some tx queue.
1465      * If not null, sock_wfree() will call __sk_free(sk) later
1466      */
1467     if (atomic_dec_and_test(&sk->sk_wmem_alloc))
1468         __sk_free(sk);
1469 }
```