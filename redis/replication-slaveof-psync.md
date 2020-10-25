## SLAVEOF 命令
在 Redis 中，用户可以通过执行 SLAVEOF 命令或者设置 slaveof 选项，让一个服务器去复制（replicate）另一个服务器，被复制的服务器为主服务器（master），而对主服务器进行复制的服务器则被称为从服务器。主从服务器双方的数据库保存相同的数据，这种现象称为“数据库状态一致”，或者简称“一致”。SLAVEOF 常用用法如下
```
SLAVEOF host port // 复制 host:port 标识的服务器
SLAVEOF NO ONE    // 关闭复制功能，转变为主服务器
```
SLAVEOF 命令由 replicaofCommand() 函数实现
```
// replication.c
void replicaofCommand(client *c) {
    if (server.cluster_enabled) { /* 集群模式不能复制 */
        addReplyError(c,"REPLICAOF not allowed in cluster mode.");
        return;
    }

    if (!strcasecmp(c->argv[1]->ptr,"no") &&
        !strcasecmp(c->argv[2]->ptr,"one")) { /* 停止复制，转变为 Master*/
        if (server.masterhost) {
            replicationUnsetMaster();
            sds client = catClientInfoString(sdsempty(),c);
            serverLog(LL_NOTICE,"MASTER MODE enabled (user request from '%s')",
                client);
            sdsfree(client);
        }
    } else {
        long port;

        if (c->flags & CLIENT_SLAVE)
        {
            addReplyError(c, "Command is not valid when client is a replica.");
            return;
        }

        if ((getLongFromObjectOrReply(c, c->argv[2], &port, NULL) != C_OK))
            return;

        if (server.masterhost && !strcasecmp(server.masterhost,c->argv[1]->ptr)
            && server.masterport == port) { /* 已经在复制 host:port 主服务器 */
            serverLog(LL_NOTICE,"REPLICAOF would result into synchronization "
                                "with the master we are already connected "
                                "with. No operation performed.");
            addReplySds(c,sdsnew("+OK Already connected to specified "
                                 "master\r\n"));
            return;
        }

        replicationSetMaster(c->argv[1]->ptr, port); /* 开启复制 */
        sds client = catClientInfoString(sdsempty(),c);
        serverLog(LL_NOTICE,"REPLICAOF %s:%d enabled (user request from '%s')",
            server.masterhost, server.masterport, client);
        sdsfree(client);
    }
    addReply(c,shared.ok);
}
```
replicationSetMaster() 函数首先设置 redisServer.{masterhost, masterport} 的值，然后将 server.repl_state 设置为 REPL\_STATE\_CONNECT。
```
// replication.c
void replicationSetMaster(char *ip, int port) {
    int was_master = server.masterhost == NULL;

    sdsfree(server.masterhost);
    server.masterhost = sdsnew(ip);
    server.masterport = port;
    if (server.master) { /* */
        freeClient(server.master);
    }
    disconnectAllBlockedClients();

    /* Force our slaves to resync with us as well. They may hopefully be able
     * to partially resync with us, but we can notify the replid change. */
    disconnectSlaves();
    cancelReplicationHandshake();
    /* Before destroying our master state, create a cached master using
     * our own parameters, to later PSYNC with the new master. */
    if (was_master) {
        replicationDiscardCachedMaster();
        replicationCacheMasterUsingMyself();
    }

    /* Fire the role change modules event. */
    moduleFireServerEvent(REDISMODULE_EVENT_REPLICATION_ROLE_CHANGED,
                          REDISMODULE_EVENT_REPLROLECHANGED_NOW_REPLICA,
                          NULL);

    /* Fire the master link modules event. */
    if (server.repl_state == REPL_STATE_CONNECTED)
        moduleFireServerEvent(REDISMODULE_EVENT_MASTER_LINK_CHANGE,
                              REDISMODULE_SUBEVENT_MASTER_LINK_DOWN,
                              NULL);

    server.repl_state = REPL_STATE_CONNECT;
}
```
replicaofCommand() 命令返回时，并没有真正开始复制工作，真正复制工作在 replicationCron() 函数中进行。

## server.repl_state
replicationCron() 函数严格按照 server.repl_state 的状态进行不同的处理。server.repl_state 标志了服务器在主从复制过程中所处的阶段。从服务器状态如下：

|状态（宏定义）|值|含义|
|:-|:-:|:-|
|REPL_STATE_NONE|0|未激活复制|
|REPL_STATE_CONNECT|1|必须和 master 连接|
|REPL_STATE_CONNECTING|2|正在和 master 连接|
|REPL_STATE_RECEIVE_PONG|3|等待 PING 回复|
|REPL_STATE_SEND_AUTH|4|发送 AUTH 给 master|
|REPL_STATE_RECEIVE_AUTH|5|等待 AUTH 回复|
|REPL_STATE_SEND_PORT|6|发送 REPLCONF 监听端口|
|REPL_STATE_RECEIVE_PORT|7|等待 REPLCONF 回复|
|REPL_STATE_SEND_IP|8 |发送 REPLCONF IP 地址|
|REPL_STATE_RECEIVE_IP|9|等待 REPLCONF 回复|
|REPL_STATE_SEND_CAPA|10|发送 REPLCONF CAPA|
|REPL_STATE_RECEIVE_CAPA|11|等待 REPLCONF 回复|
|REPL_STATE_SEND_PSYNC|12|发送 PSYNC 命令|
|REPL_STATE_RECEIVE_PSYNC|13 |等待 PSYNC 回复|
|REPL_STATE_TRANSFER|14|正在从 master 接收 .rdb 文件|
|REPL_STATE_CONNECTED|15|已经和 master 连接|

其中，REPL_STATE_RECEIVE_PONG 到 REPL_STATE_RECEIVE_PSYNC 处于主从服务器握手（handshake）阶段，状态必须按照循序转换。另外，master 服务器在 client->replstate 记录了从服务器的状态，有如下状态：

|状态（宏定义）|值|含义|
|:-|:-:|:-|
|SLAVE_STATE_WAIT_BGSAVE_START|6|需要重新生成 RDB 文件|
|SLAVE_STATE_WAIT_BGSAVE_END|7|等待 RDB 文件生成|
|SLAVE_STATE_SEND_BULK|8|将 RDB 文件发送给 slave|
|SLAVE_STATE_ONLINE|9|RDB 文件以及发送，只需要发送更新.|

## PSYNC 命令
slave 服务器通过向 master 服务器发送 PSYNC 命令，开启部分同步或者全部同步。master 服务器 PSYNC 命令处理如下：
```
// replication.c
void syncCommand(client *c) {
    /* ignore SYNC if already slave or in monitor mode */
    if (c->flags & CLIENT_SLAVE) return;

    if (server.masterhost && server.repl_state != REPL_STATE_CONNECTED) {
        addReplySds(c,sdsnew("-NOMASTERLINK Can't SYNC while not connected with my master\r\n"));
        return;
    }

    if (clientHasPendingReplies(c)) {
        addReplyError(c,"SYNC and PSYNC are invalid with pending output");
        return;
    }

    serverLog(LL_NOTICE,"Replica %s asks for synchronization",
        replicationGetSlaveName(c));

    /* Try a partial resynchronization if this is a PSYNC command.
     * If it fails, we continue with usual full resynchronization, however
     * when this happens masterTryPartialResynchronization() already
     * replied with:
     *
     * +FULLRESYNC <replid> <offset>
     *
     * So the slave knows the new replid and offset to try a PSYNC later
     * if the connection with the master is lost. */
/* master 服务器尝试进行部分同步*/
    if (!strcasecmp(c->argv[0]->ptr,"psync")) {
        if (masterTryPartialResynchronization(c) == C_OK) {
            server.stat_sync_partial_ok++;
            return; /* No full resync needed, return. */
        } else { /* 部分同步失败 */
            char *master_replid = c->argv[1]->ptr;
/* master_replid[0] != '?' 表示 slave 强制全同步 */
            if (master_replid[0] != '?') server.stat_sync_partial_err++;
        }
    } else { /* SYNC 命令*/
        c->flags |= CLIENT_PRE_PSYNC;
    }

/* 部分同步失败，全部同步 */
    server.stat_sync_full++;

/* 标志 slave 态为 SLAVE_STATE_WAIT_BGSAVE_START */
    c->replstate = SLAVE_STATE_WAIT_BGSAVE_START;
    if (server.repl_disable_tcp_nodelay)
        connDisableTcpNoDelay(c->conn); /* Non critical if it fails. */
    c->repldbfd = -1;
    c->flags |= CLIENT_SLAVE;
    listAddNodeTail(server.slaves,c);

/* 如果必要，创建 backlog */
    if (listLength(server.slaves) == 1 && server.repl_backlog == NULL) {
        changeReplicationId();
        clearReplicationId2();
        createReplicationBacklog();
    }

/* CASE 1，BGSAVE 正在进行并且写入磁盘上，检查 RDB 是否可复用 */
    if (server.rdb_child_pid != -1 &&
        server.rdb_child_type == RDB_CHILD_TYPE_DISK)
    {
        client *slave;
        listNode *ln;
        listIter li;

        listRewind(server.slaves,&li);
        while((ln = listNext(&li))) {
            slave = ln->value;
            if (slave->replstate == SLAVE_STATE_WAIT_BGSAVE_END) break;
        }
        if (ln && ((c->slave_capa & slave->slave_capa) == slave->slave_capa)) {
            copyClientOutputBuffer(c,slave);
            replicationSetupSlaveForFullResync(c,slave->psync_initial_offset);
            serverLog(LL_NOTICE,"Waiting for end of BGSAVE for SYNC");
        } else {
            serverLog(LL_NOTICE,"Can't attach the replica to the current BGSAVE. Waiting for next BGSAVE for SYNC");
        }

/* CASE 2，BGSAVE 正在进行但是写入 socket，等待 */
    } else if (server.rdb_child_pid != -1 &&
               server.rdb_child_type == RDB_CHILD_TYPE_SOCKET)
    {
        serverLog(LL_NOTICE,"Current BGSAVE has socket target. Waiting for next BGSAVE for SYNC");

/* CASE 3， BGSAVE 未启动，启动 BGSAVE */
    } else {
        if (server.repl_diskless_sync && (c->slave_capa & SLAVE_CAPA_EOF)) {
            if (server.repl_diskless_sync_delay)
                serverLog(LL_NOTICE,"Delay next BGSAVE for diskless SYNC");
        } else {
            if (!hasActiveChildProcess()) {
                startBgsaveForReplication(c->slave_capa);
            } else {
                serverLog(LL_NOTICE,
                    "No BGSAVE in progress, but another BG operation is active. "
                    "BGSAVE for replication delayed");
            }
        }
    }
    return;
}
```