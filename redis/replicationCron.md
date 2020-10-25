## replicationCron()
replicationCron() 函数在 serverCron() 函数中每隔 1 秒被调用一次。

如果是 slave 服务器，replicationCron() 函数根据 server.repl_state 的状态和 master 进行连接或者处理错误等。另外，salve 服务器还会定期向 master 服务器发送 REPLCONF ACK 命令，将自己 reploff 告知 master 服务器。

如果是 master 服务器，服务器，replicationCron() 函数会进行
- 定期向 slave 发送 PING 命令，方便 slave 判断是否连接正常，另外，也将 master_repl_meaningful_offset 发送给 salve。
- 在同步阶段之前（pre-synchronization），master 向 slave 发送 '\n'，让 salve 等待生产 RDB 文件或者指示 slave，它们的 master 在线。
- 和产生 timeout 的 slave 断开连接。
- 如果没有 slave 但是 ReplicationBacklog 处于激活状态，释放 backlog 内存。
- 如果有 slave 处于 SLAVE_STATE_WAIT_BGSAVE_START 状态，开启一个 BGSAVE 线程。

```
// replication.c
void replicationCron(void) {
    static long long replication_cron_loops = 0;

    if (server.masterhost &&
        (server.repl_state == REPL_STATE_CONNECTING ||
         slaveIsInHandshakeState()) &&
         (time(NULL)-server.repl_transfer_lastio) > server.repl_timeout)
    {/* 正在连接但是连接超时 */
        serverLog(LL_WARNING,"Timeout connecting to the MASTER...");
        cancelReplicationHandshake();
    }

    if (server.masterhost && server.repl_state == REPL_STATE_TRANSFER &&
        (time(NULL)-server.repl_transfer_lastio) > server.repl_timeout)
    {/* 接收 RDB 文件超时 */
        serverLog(LL_WARNING,"Timeout receiving bulk data from MASTER... If the problem persists try to set the 'repl-timeout' parameter in redis.conf to a larger value.");
        cancelReplicationHandshake();
    }

    if (server.masterhost && server.repl_state == REPL_STATE_CONNECTED &&
        (time(NULL)-server.master->lastinteraction) > server.repl_timeout)
    {/* 已连接但是和 master 连接中断 */
        serverLog(LL_WARNING,"MASTER timeout: no data nor PING received...");
        freeClient(server.master);
    }

    if (server.repl_state == REPL_STATE_CONNECT) {/* 需要和 master 建立连接*/
        serverLog(LL_NOTICE,"Connecting to MASTER %s:%d",
            server.masterhost, server.masterport);
        if (connectWithMaster() == C_OK) {
            serverLog(LL_NOTICE,"MASTER <-> REPLICA sync started");
        }
    }

    if (server.masterhost && server.master &&
        !(server.master->flags & CLIENT_PRE_PSYNC))
        replicationSendAck(); /* 定期向 master 发送 ACK */

    listIter li;
    listNode *ln;
    robj *ping_argv[1];

    if ((replication_cron_loops % server.repl_ping_slave_period) == 0 &&
        listLength(server.slaves))
    {/* master 定期向 slaves 发送 PING */
        int manual_failover_in_progress =
            server.cluster_enabled &&
            server.cluster->mf_end &&
            clientsArePaused();

        if (!manual_failover_in_progress) {
            long long before_ping = server.master_repl_meaningful_offset;
            ping_argv[0] = createStringObject("PING",4);
            replicationFeedSlaves(server.slaves, server.slaveseldb,
                ping_argv, 1);
            decrRefCount(ping_argv[0]);
            server.master_repl_meaningful_offset = before_ping;
        }
    }

    listRewind(server.slaves,&li);
    while((ln = listNext(&li))) {
        client *slave = ln->value;

        int is_presync =
            (slave->replstate == SLAVE_STATE_WAIT_BGSAVE_START ||
            (slave->replstate == SLAVE_STATE_WAIT_BGSAVE_END &&
             server.rdb_child_type != RDB_CHILD_TYPE_SOCKET));

        if (is_presync) {
            connWrite(slave->conn, "\n", 1); /* 发送 newline */
        }
    }

    if (listLength(server.slaves)) {/* 断开处于 timeout 的 slave */
        listIter li;
        listNode *ln;

        listRewind(server.slaves,&li);
        while((ln = listNext(&li))) {
            client *slave = ln->value;

            if (slave->replstate != SLAVE_STATE_ONLINE) continue;
            if (slave->flags & CLIENT_PRE_PSYNC) continue;
            if ((server.unixtime - slave->repl_ack_time) > server.repl_timeout)
            {
                serverLog(LL_WARNING, "Disconnecting timedout replica: %s",
                    replicationGetSlaveName(slave));
                freeClient(slave);
            }
        }
    }

    if (listLength(server.slaves) == 0 && server.repl_backlog_time_limit &&
        server.repl_backlog && server.masterhost == NULL)
    {
        time_t idle = server.unixtime - server.repl_no_slaves_since;

        if (idle > server.repl_backlog_time_limit) {
            changeReplicationId();
            clearReplicationId2();
            freeReplicationBacklog();
            serverLog(LL_NOTICE,
                "Replication backlog freed after %d seconds "
                "without connected replicas.",
                (int) server.repl_backlog_time_limit);
        }
    }

    if (listLength(server.slaves) == 0 &&
        server.aof_state == AOF_OFF &&
        listLength(server.repl_scriptcache_fifo) != 0)
    {
        replicationScriptCacheFlush();
    }

    if (!hasActiveChildProcess()) {
        time_t idle, max_idle = 0;
        int slaves_waiting = 0;
        int mincapa = -1;
        listNode *ln;
        listIter li;

        listRewind(server.slaves,&li);
        while((ln = listNext(&li))) {
            client *slave = ln->value;
            if (slave->replstate == SLAVE_STATE_WAIT_BGSAVE_START) {
                idle = server.unixtime - slave->lastinteraction;
                if (idle > max_idle) max_idle = idle;
                slaves_waiting++;
                mincapa = (mincapa == -1) ? slave->slave_capa :
                                            (mincapa & slave->slave_capa);
            }
        }

        if (slaves_waiting &&
            (!server.repl_diskless_sync ||
             max_idle > server.repl_diskless_sync_delay))
        {
            startBgsaveForReplication(mincapa);
        }
    }

    removeRDBUsedToSyncReplicas();

    refreshGoodSlavesCount();
    replication_cron_loops++; /* Incremented with frequency 1 HZ. */
}
```