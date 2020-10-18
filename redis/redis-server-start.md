## Redis 服务器启动
在启动服务器时，管理员通过给定配置参数或者指定配置文件来修改服务器的默认配置，比如
```
redis-server --port 9999
```
或者通过传递配置文件
```
server-server redis.conf
```
来启动服务器进程

## Redis 初始化
```
// server.c
int main(int argc, char **argv) {
    struct timeval tv;
    int j;
/* ... */ /* 是否启用 Sentinel */
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
/* ... */ /* 检查是否需要从 RDB 或者 AOF 文件恢复 */
    if (strstr(argv[0],"redis-check-rdb") != NULL)
        redis_check_rdb_main(argc,argv,NULL);
    else if (strstr(argv[0],"redis-check-aof") != NULL)
        redis_check_aof_main(argc,argv);

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
/* ... */
    } else {
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

## Sentinel 模式
Redis 服务器可以复制其他服务器，其中，执行复制命令的服务器是从服务器，被复制的服务器是主服务器。比如在某一时刻，服务器 A 启动，复制服务器 B 的全部数据，则 A 是从服务器，B 是主服务器。从服务器永远不主动更新其数据库，只有收到来自主服务器的数据更新命令时，才更新自己的数据。

Sentinel（哨兵）是 Redis 的高可用性解决方案：由一个或多个 Sentinel 实例组成的 Sentinel 系统可以监视任意多个主服务器，以及这些主服务器属下的所有从服务器。当被监视的主服务器进入下线状态时，自动将下线主服务器属下的某个从服务器升级为新的主服务器，然后由新的主服务代替已下线的主服务器继续处理命令请求。

## initServer()
initServer() 函数初始化 Redis 服务器
- 创建共享对象，比如小于 1000 的整数、错误字符串等等
- 创建事件循环 EventLoop 对象
- 创建并初始化 redisDb 数组
- 注册事件处理函数

## aeMain()
aeMain() 函数开始事件循环。Redis 服务器是一个事件驱动的服务程序，定义了一系列的事件处理函数来处理对应的事件。Redis 服务器总共处理两种事件，一是文件事件，主要是处理套接字读写；二是事件事件，提供定时的功能。