## 再探 server.c:main()
Sentinel 本质上是一个运行在特殊模式下的 Redis 服务器，启动 Sentinel 和初始化普通 Redis 服务器相似，只有细微的差别，具体如下：
1. 在 initServerConfig() 函数后，调用 initSentinelConfig() 和 initSentinel() 函数
    - 将 server.port 替换为 REDIS_SENTINEL_PORT
    - 将命令列表 server.commands 改变为 Sentinel 实现
    - 初始化 Sentinel 状态
1. 在 loadServerConfig() 函数中，加载 Sentinel 配置文件
1. 在 initServer() 函数后，相继执行 InitServerLast() 和 sentinelIsRunning() 函数。

除此之外，Sentinel 和普通 Redis 服务器启动相同，不过在时间处理函数 severCron() 中，Sentinel 和普通 Redis 服务器有较大的差别。如果是 Sentinel 模式，会执行 sentinelTimer() 函数。
```
// server.c
int main(int argc, char **argv) {
    struct timeval tv;
    int j;
/* ... */ /* 是否 Sentinel */
    server.sentinel_mode = checkForSentinelMode(argc,argv);
    initServerConfig(); /* 设置默认值 */
/* ... */
    moduleInitModulesSystem();
/* ... */ /* 保存启动命令，以便重启 */
    server.executable = getAbsolutePath(argv[0]);
    server.exec_argv = zmalloc(sizeof(char*)*(argc+1));
    server.exec_argv[argc] = NULL;
    for (j = 0; j < argc; j++) server.exec_argv[j] = zstrdup(argv[j]);
/* ... */ /* 初始化 Sentinel */
    if (server.sentinel_mode) {
        initSentinelConfig();
        initSentinel();
    }
/* ... 检查是否需要从 RDB 或者 AOF 文件恢复 ... */

    if (argc >= 2) { /* 解析命令参数 */
        j = 1; /* First option to parse in argv[] */
        sds options = sdsempty();
        char *configfile = NULL;
/* ... --help 或者 --version 等 */
        if (argv[j][0] != '-' || argv[j][1] != '-') { /* 配置文件 */
            configfile = argv[j];
            server.configfile = getAbsolutePath(configfile);
            zfree(server.exec_argv[j]);
            server.exec_argv[j] = zstrdup(server.configfile); /* 保存绝对路径 */
            j++;
        }
/* ... */ /* 复制参数到 options */
        while(j != argc) {
            if (argv[j][0] == '-' && argv[j][1] == '-') {
                /* Option name */
                if (!strcmp(argv[j], "--check-rdb")) {
                    /* Argument has no options, need to skip for parsing. */
                    j++;
                    continue;
                }
                if (sdslen(options)) options = sdscat(options,"\n");
                options = sdscat(options,argv[j]+2);
                options = sdscat(options," ");
            } else {
                /* Option argument */
                options = sdscatrepr(options,argv[j],strlen(argv[j]));
                options = sdscat(options," ");
            }
            j++;
        }
/* ... */ /* 加载配置参数 */
        loadServerConfig(configfile,options); 
        sdsfree(options);
    }
/* ... */
    server.supervised = redisIsSupervised(server.supervised_mode);
    int background = server.daemonize && !server.supervised;
    if (background) daemonize();

    initServer();
    if (background || server.pidfile) createPidFile();
    redisSetProcTitle(argv[0]);
    redisAsciiArt();
    checkTcpBacklogSettings();

    if (!server.sentinel_mode) {
/* ... 普通 Redis 服务器 ... */
    } else { /* Sentinel 模式 */
        InitServerLast();
        sentinelIsRunning();
    }
/* ... */
    aeSetBeforeSleepProc(server.el,beforeSleep);
    aeSetAfterSleepProc(server.el,afterSleep);
    aeMain(server.el);
    aeDeleteEventLoop(server.el);
    return 0;
}
```

## initSentinelConfig()
initSentinelConfig() 设置 Sentinel 的端口值
```
// sentinel.c
void initSentinelConfig(void) {
    server.port = REDIS_SENTINEL_PORT;
    server.protected_mode = 0; /* Sentinel must be exposed. */
}
```

## initSentinel()
Sentinel 用 sentinelState 结构记录运行过程中的相关状态，其定义如下：
```
// sentinel.c
struct sentinelState {
    char myid[CONFIG_RUN_ID_SIZE+1]; /* This sentinel ID. */
    uint64_t current_epoch;         /* Current epoch. */
    dict *masters;      /* Dictionary of master sentinelRedisInstances.
                           Key is the instance name, value is the
                           sentinelRedisInstance structure pointer. */
    int tilt;           /* Are we in TILT mode? */
    int running_scripts;    /* Number of scripts in execution right now. */
    mstime_t tilt_start_time;       /* When TITL started. */
    mstime_t previous_time;         /* Last time we ran the time handler. */
    list *scripts_queue;            /* Queue of user scripts to execute. */
    char *announce_ip;  /* IP addr that is gossiped to other sentinels if
                           not NULL. */
    int announce_port;  /* Port that is gossiped to other sentinels if
                           non zero. */
    unsigned long simfailure_flags; /* Failures simulation. */
    int deny_scripts_reconfig; /* Allow SENTINEL SET ... to change script
                                  paths at runtime? */
} sentinel;
```
masters 字典保存了所有被 Sentinel 监视的主服务器的相关信息，其中 Key 是被监视的主服务器的名字，Value 是表征被监视主服务器的 sentinelRedisInstance 变量。

启动时，sentinelState 在 initSentinel() 函数中被初始化。另外，initSentinel() 函数还负责将 server.commands 替换为 Sentinel 相关的命令实现。
```
// sentinel.c
void initSentinel(void) {
    unsigned int j;

    /* Remove usual Redis commands from the command table, then just add
     * the SENTINEL command. */
    dictEmpty(server.commands,NULL);
    for (j = 0; j < sizeof(sentinelcmds)/sizeof(sentinelcmds[0]); j++) {
        int retval;
        struct redisCommand *cmd = sentinelcmds+j;

        retval = dictAdd(server.commands, sdsnew(cmd->name), cmd);
        serverAssert(retval == DICT_OK);

        /* Translate the command string flags description into an actual
         * set of flags. */
        if (populateCommandTableParseFlags(cmd,cmd->sflags) == C_ERR)
            serverPanic("Unsupported command flag");
    }

    /* Initialize various data structures. */
    sentinel.current_epoch = 0;
    sentinel.masters = dictCreate(&instancesDictType,NULL);
    sentinel.tilt = 0;
    sentinel.tilt_start_time = 0;
    sentinel.previous_time = mstime();
    sentinel.running_scripts = 0;
    sentinel.scripts_queue = listCreate();
    sentinel.announce_ip = NULL;
    sentinel.announce_port = 0;
    sentinel.simfailure_flags = SENTINEL_SIMFAILURE_NONE;
    sentinel.deny_scripts_reconfig = SENTINEL_DEFAULT_DENY_SCRIPTS_RECONFIG;
    memset(sentinel.myid,0,sizeof(sentinel.myid));
}
```