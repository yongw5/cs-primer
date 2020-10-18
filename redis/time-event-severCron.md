## severCron()
serverCron() 函数每秒执行 server.hz 次（默认每秒执行 10 次，也就是每隔 100 毫秒执行一次）。serverCron 主要的工作：
- 更新服务器时间缓存；
- 更新 LRU 时钟；
- 更新服务器每秒执行命令的次数；
- 更新服务器内存峰值记录；
- 处理 SIGTERM 信号
- 调用 clientsCron() 与客户端的连接是否正常，客客户端的输入缓冲区是否合适；
- 调用 databaseCron 函数，删除过期的 Key-Value 数据，在必要时，对 dict 进行收缩操作；
- 执行被延迟的 BGFRWRITEAOF；
- 检查持久化操作的运行状态；
- 将 AOF 缓冲区中的内容写入 AOF 文件；
- 关闭异步客户端；
- 增加 cronloops 计数器的值；

```
// server.c
int serverCron(struct aeEventLoop *eventLoop, long long id, void *clientData) {
    int j;
    UNUSED(eventLoop);
    UNUSED(id);
    UNUSED(clientData);
/* 如果 server.watchdog_period 函数没有返回，发送一个 SIGALRM 信号 */
    if (server.watchdog_period) watchdogScheduleSignal(server.watchdog_period);
/* 更新系统缓存时间 */
    updateCachedTime(1);
/* 根据 client 数量调整 server.hz 值，如果 client 数量多，serverCron() 调用更频繁 */
    server.hz = server.config_hz;
    if (server.dynamic_hz) {
        while (listLength(server.clients) / server.hz >
               MAX_CLIENTS_PER_CLOCK_TICK)
        {
            server.hz *= 2;
            if (server.hz > CONFIG_MAX_HZ) {
                server.hz = CONFIG_MAX_HZ;
                break;
            }
        }
    }
/* 每 100 毫秒执行一次 */
    run_with_period(100) {
        trackInstantaneousMetric(STATS_METRIC_COMMAND,server.stat_numcommands);
        trackInstantaneousMetric(STATS_METRIC_NET_INPUT,
                server.stat_net_input_bytes);
        trackInstantaneousMetric(STATS_METRIC_NET_OUTPUT,
                server.stat_net_output_bytes);
    }
/* 更新 lruclock 缓存值 */
    server.lruclock = getLRUClock();
/* 记录内存最大使用量 */
    /* Record the max memory used since the server was started. */
    if (zmalloc_used_memory() > server.stat_peak_memory)
        server.stat_peak_memory = zmalloc_used_memory();

    run_with_period(100) {
        server.cron_malloc_stats.process_rss = zmalloc_get_rss();
        server.cron_malloc_stats.zmalloc_used = zmalloc_used_memory();
        zmalloc_get_allocator_info(&server.cron_malloc_stats.allocator_allocated,
                                   &server.cron_malloc_stats.allocator_active,
                                   &server.cron_malloc_stats.allocator_resident);
        if (!server.cron_malloc_stats.allocator_resident) {
            size_t lua_memory = lua_gc(server.lua,LUA_GCCOUNT,0)*1024LL;
            server.cron_malloc_stats.allocator_resident = server.cron_malloc_stats.process_rss - lua_memory;
        }
        if (!server.cron_malloc_stats.allocator_active)
            server.cron_malloc_stats.allocator_active = server.cron_malloc_stats.allocator_resident;
        if (!server.cron_malloc_stats.allocator_allocated)
            server.cron_malloc_stats.allocator_allocated = server.cron_malloc_stats.zmalloc_used;
    }
/* 处理 SIGTERM 信号 */
    if (server.shutdown_asap) {
        if (prepareForShutdown(SHUTDOWN_NOFLAGS) == C_OK) exit(0);
        serverLog(LL_WARNING,"SIGTERM received but errors trying to shut down the server, check the logs for more information");
        server.shutdown_asap = 0;
    }
/* 每 5000 毫秒执行一次，输出每个数据库大小信息 */
    run_with_period(5000) {
        for (j = 0; j < server.dbnum; j++) {
            long long size, used, vkeys;

            size = dictSlots(server.db[j].dict);
            used = dictSize(server.db[j].dict);
            vkeys = dictSize(server.db[j].expires);
            if (used || vkeys) {
                serverLog(LL_VERBOSE,"DB %d: %lld keys (%lld volatile) in %lld slots HT.",j,used,vkeys,size);
                /* dictPrintStats(server.dict); */
            }
        }
    }
/* 每 5000 毫秒执行一次，输出 Sentinel 监视主服务器和从服务器信息 */
    /* Show information about connected clients */
    if (!server.sentinel_mode) {
        run_with_period(5000) {
            serverLog(LL_DEBUG,
                "%lu clients connected (%lu replicas), %zu bytes in use",
                listLength(server.clients)-listLength(server.slaves),
                listLength(server.slaves),
                zmalloc_used_memory());
        }
    }

/* 每次调用尽量处理 numclients/server.hz 个 client，比如超时等 */
    clientsCron();

/* 处理 database 相关操作，比如过期键、再哈希等 */
    databasesCron();

/* 开始一个 AOF 持久化进程 */
    if (!hasActiveChildProcess() &&
        server.aof_rewrite_scheduled)
    {
        rewriteAppendOnlyFileBackground();
    }

    if (hasActiveChildProcess() || ldbPendingChildren())
    {
        checkChildrenDone();
    } else {
        for (j = 0; j < server.saveparamslen; j++) {
            struct saveparam *sp = server.saveparams+j;

            if (server.dirty >= sp->changes &&
                server.unixtime-server.lastsave > sp->seconds &&
                (server.unixtime-server.lastbgsave_try >
                 CONFIG_BGSAVE_RETRY_DELAY ||
                 server.lastbgsave_status == C_OK))
            {
                serverLog(LL_NOTICE,"%d changes in %d seconds. Saving...",
                    sp->changes, (int)sp->seconds);
                rdbSaveInfo rsi, *rsiptr;
                rsiptr = rdbPopulateSaveInfo(&rsi);
                rdbSaveBackground(server.rdb_filename,rsiptr);
                break;
            }
        }

        /* Trigger an AOF rewrite if needed. */
        if (server.aof_state == AOF_ON &&
            !hasActiveChildProcess() &&
            server.aof_rewrite_perc &&
            server.aof_current_size > server.aof_rewrite_min_size)
        {
            long long base = server.aof_rewrite_base_size ?
                server.aof_rewrite_base_size : 1;
            long long growth = (server.aof_current_size*100/base) - 100;
            if (growth >= server.aof_rewrite_perc) {
                serverLog(LL_NOTICE,"Starting automatic rewriting of AOF on %lld%% growth",growth);
                rewriteAppendOnlyFileBackground();
            }
        }
    }

    if (server.aof_flush_postponed_start) flushAppendOnlyFile(0);

    run_with_period(1000) {
        if (server.aof_last_write_status == C_ERR)
            flushAppendOnlyFile(0);
    }

/* 清空暂停 client */
    clientsArePaused(); /* Don't check return value, just use the side effect.*/

/* 处理复制操作 */
    run_with_period(1000) replicationCron();

/* 处理 Cluster 相关 */
    run_with_period(100) {
        if (server.cluster_enabled) clusterCron();
    }

/* 处理 Cluster 相关，比如心跳检测等 */
    if (server.sentinel_mode) sentinelTimer();

/* 处理过期的 MIGRATE 缓存 */
    run_with_period(1000) {
        migrateCloseTimedoutSockets();
    }

/* 停止 IO 线程 */
    stopThreadedIOIfNeeded();

/* 开始 BGSAVE 线程 */
    if (!hasActiveChildProcess() &&
        server.rdb_bgsave_scheduled &&
        (server.unixtime-server.lastbgsave_try > CONFIG_BGSAVE_RETRY_DELAY ||
         server.lastbgsave_status == C_OK))
    {
        rdbSaveInfo rsi, *rsiptr;
        rsiptr = rdbPopulateSaveInfo(&rsi);
        if (rdbSaveBackground(server.rdb_filename,rsiptr) == C_OK)
            server.rdb_bgsave_scheduled = 0;
    }

    RedisModuleCronLoopV1 ei = {REDISMODULE_CRON_LOOP_VERSION,server.hz};
    moduleFireServerEvent(REDISMODULE_EVENT_CRON_LOOP,
                          0,
                          &ei);

    server.cronloops++;
    return 1000/server.hz;
}
```
