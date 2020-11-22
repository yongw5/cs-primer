## sentinelTimer()
在 Redis 服务器时间事件处理函数 [severCron()](./time-event-severCron.md)中，如果是 Sentinel 模式，会调用 sentinelTimer() 函数
```
// server.c
int serverCron(struct aeEventLoop *eventLoop, long long id, void *clientData) {
/* ... *//* 处理 Sentinel 相关 */
    if (server.sentinel_mode) sentinelTimer();
/* ... */
}
```
sentinelTimer() 函数处理 Sentinel 相关的时间事件，定期处理如下事宜：
1. 调用 sentinelCheckTiltCondition() 函数，检查是否应该进入 TITL 模式；
2. 调用 sentinelHandleDictOfRedisInstances() 函数，进行监视或者故障检测；
3. 处理脚本

```
// sentinel.c
void sentinelTimer(void) {
    sentinelCheckTiltCondition();
    sentinelHandleDictOfRedisInstances(sentinel.masters);
    sentinelRunPendingScripts();
    sentinelCollectTerminatedScripts();
    sentinelKillTimedoutScripts();
    server.hz = CONFIG_DEFAULT_HZ + rand() % CONFIG_DEFAULT_HZ;
}
```
另外，每次调用 sentinelTimer() 函数都会更改 server.hz 的值，以使每个 Sentinel 之间彼此不同步。这种非确定性避免了 Sentinel 在同一时间开始正是继续保持同步要求被同时连连投票（结果是没有人会赢得因为脑裂投票选举）。

## sentinelCheckTiltCondition()
此函数检查是否需要进入 TITL 模式。如果检测到两次定时器中断之间的时间间隔为负或时间过长，则进入 TILT 模式。请注意，如果一切正常，我们预计将经过大约 100 毫秒。 但是，如果发生以下情况之一，我们将看到一个负数或一个大于 SENTINEL_TILT_TRIGGER 毫秒的差：
1. Sentiel 进程由于某种随机原因而被阻塞了一段时间：负载巨大，计算机在 I/O 或类似环境中冻结了一段时间，该进程被信号停止。 一切。
2. 系统时钟发生了重大变化。 在这两种情况下，我们都会看到一切都超时了，并且没有充分的理由失败。 相反，我们进入 TILT 模式并等待 SENTINEL_TILT_PERIOD 过去，然后再开始执行操作。
 
在TILT期间，我们仍在收集信息，我们只是不采取行动。
```
// sentinel.c
void sentinelCheckTiltCondition(void) {
    mstime_t now = mstime();
    mstime_t delta = now - sentinel.previous_time;

    if (delta < 0 || delta > SENTINEL_TILT_TRIGGER) {
        sentinel.tilt = 1;
        sentinel.tilt_start_time = mstime();
        sentinelEvent(LL_WARNING,"+tilt",NULL,"#tilt mode entered");
    }
    sentinel.previous_time = mstime();
}
```

## sentinelHandleDictOfRedisInstances()
sentinelHandleDictOfRedisInstances() 将逐一处理 instances 中的 sentinelRedisInstance 实例。如果是 SRI_MASTER，将递归调用，处理 ri->slaves 和 ri->sentinels 实例。如果 salve 或者 sentinel 中 failover_state 处于 SENTINEL_FAILOVER_STATE_UPDATE_CONFIG，将触发调用 sentinelFailoverSwitchToPromotedSlave() 函数。
```
// sentinel.c
void sentinelHandleDictOfRedisInstances(dict *instances) {
    dictIterator *di;
    dictEntry *de;
    sentinelRedisInstance *switch_to_promoted = NULL;

    /* There are a number of things we need to perform against every master. */
    di = dictGetIterator(instances);
    while((de = dictNext(di)) != NULL) {
        sentinelRedisInstance *ri = dictGetVal(de);

        sentinelHandleRedisInstance(ri);
        if (ri->flags & SRI_MASTER) {
            sentinelHandleDictOfRedisInstances(ri->slaves);
            sentinelHandleDictOfRedisInstances(ri->sentinels);
            if (ri->failover_state == SENTINEL_FAILOVER_STATE_UPDATE_CONFIG) {
                switch_to_promoted = ri;
            }
        }
    }
    if (switch_to_promoted)
        sentinelFailoverSwitchToPromotedSlave(switch_to_promoted);
    dictReleaseIterator(di);
}
```

## sentinelRunPendingScripts()
```
/* Run pending scripts if we are not already at max number of running
 * scripts. */
// sentinel.c
void sentinelRunPendingScripts(void) {
    listNode *ln;
    listIter li;
    mstime_t now = mstime();

    /* Find jobs that are not running and run them, from the top to the
     * tail of the queue, so we run older jobs first. */
    listRewind(sentinel.scripts_queue,&li);
    while (sentinel.running_scripts < SENTINEL_SCRIPT_MAX_RUNNING &&
           (ln = listNext(&li)) != NULL)
    {
        sentinelScriptJob *sj = ln->value;
        pid_t pid;

        /* Skip if already running. */
        if (sj->flags & SENTINEL_SCRIPT_RUNNING) continue;

        /* Skip if it's a retry, but not enough time has elapsed. */
        if (sj->start_time && sj->start_time > now) continue;

        sj->flags |= SENTINEL_SCRIPT_RUNNING;
        sj->start_time = mstime();
        sj->retry_num++;
        pid = fork();

        if (pid == -1) {
            /* Parent (fork error).
             * We report fork errors as signal 99, in order to unify the
             * reporting with other kind of errors. */
            sentinelEvent(LL_WARNING,"-script-error",NULL,
                          "%s %d %d", sj->argv[0], 99, 0);
            sj->flags &= ~SENTINEL_SCRIPT_RUNNING;
            sj->pid = 0;
        } else if (pid == 0) {
            /* Child */
            execve(sj->argv[0],sj->argv,environ);
            /* If we are here an error occurred. */
            _exit(2); /* Don't retry execution. */
        } else {
            sentinel.running_scripts++;
            sj->pid = pid;
            sentinelEvent(LL_DEBUG,"+script-child",NULL,"%ld",(long)pid);
        }
    }
}
```

## sentinelCollectTerminatedScripts()
```
/* Check for scripts that terminated, and remove them from the queue if the
 * script terminated successfully. If instead the script was terminated by
 * a signal, or returned exit code "1", it is scheduled to run again if
 * the max number of retries did not already elapsed. */
// sentinel.c
void sentinelCollectTerminatedScripts(void) {
    int statloc;
    pid_t pid;

    while ((pid = wait3(&statloc,WNOHANG,NULL)) > 0) {
        int exitcode = WEXITSTATUS(statloc);
        int bysignal = 0;
        listNode *ln;
        sentinelScriptJob *sj;

        if (WIFSIGNALED(statloc)) bysignal = WTERMSIG(statloc);
        sentinelEvent(LL_DEBUG,"-script-child",NULL,"%ld %d %d",
            (long)pid, exitcode, bysignal);

        ln = sentinelGetScriptListNodeByPid(pid);
        if (ln == NULL) {
            serverLog(LL_WARNING,"wait3() returned a pid (%ld) we can't find in our scripts execution queue!", (long)pid);
            continue;
        }
        sj = ln->value;

        /* If the script was terminated by a signal or returns an
         * exit code of "1" (that means: please retry), we reschedule it
         * if the max number of retries is not already reached. */
        if ((bysignal || exitcode == 1) &&
            sj->retry_num != SENTINEL_SCRIPT_MAX_RETRY)
        {
            sj->flags &= ~SENTINEL_SCRIPT_RUNNING;
            sj->pid = 0;
            sj->start_time = mstime() +
                             sentinelScriptRetryDelay(sj->retry_num);
        } else {
            /* Otherwise let's remove the script, but log the event if the
             * execution did not terminated in the best of the ways. */
            if (bysignal || exitcode != 0) {
                sentinelEvent(LL_WARNING,"-script-error",NULL,
                              "%s %d %d", sj->argv[0], bysignal, exitcode);
            }
            listDelNode(sentinel.scripts_queue,ln);
            sentinelReleaseScriptJob(sj);
        }
        sentinel.running_scripts--;
    }
}

```

## sentinelKillTimedoutScripts()
```
/* Kill scripts in timeout, they'll be collected by the
 * sentinelCollectTerminatedScripts() function. */
// sentinel.c
void sentinelKillTimedoutScripts(void) {
    listNode *ln;
    listIter li;
    mstime_t now = mstime();

    listRewind(sentinel.scripts_queue,&li);
    while ((ln = listNext(&li)) != NULL) {
        sentinelScriptJob *sj = ln->value;

        if (sj->flags & SENTINEL_SCRIPT_RUNNING &&
            (now - sj->start_time) > SENTINEL_SCRIPT_MAX_RUNTIME)
        {
            sentinelEvent(LL_WARNING,"-script-timeout",NULL,"%s %ld",
                sj->argv[0], (long)sj->pid);
            kill(sj->pid,SIGKILL);
        }
    }
}
```