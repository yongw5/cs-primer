## initServerConfig()
initServerConfig() 函数设置默认值，redisServer 数据结构中 redisCommand 在该函数中被初始化。redisServer 为常用的命令保留了一个 redisCommand 指针，指向 commands 中对应的 redisCommand 对象。
```
// server.c
void initServerConfig(void) {
    int j;

    updateCachedTime(1);
/* ... */
    server.hz = CONFIG_DEFAULT_HZ;
/* ... */
    server.client_max_querybuf_len = PROTO_MAX_QUERYBUF_LEN;
/* ... */
    server.cluster_configfile = zstrdup(CONFIG_DEFAULT_CLUSTER_CONFIG_FILE);
    server.cluster_module_flags = CLUSTER_MODULE_FLAG_NONE;
    server.migrate_cached_sockets = dictCreate(&migrateCacheDictType,NULL);
    server.next_client_id = 1; /* Client IDs, start from 1 .*/
    server.loading_process_events_interval_bytes = (1024*1024*2);

    server.lruclock = getLRUClock();
    resetServerSaveParams();

    appendServerSaveParams(60*60,1);  /* save after 1 hour and 1 change */
    appendServerSaveParams(300,100);  /* save after 5 minutes and 100 changes */
    appendServerSaveParams(60,10000); /* save after 1 minute and 10000 changes */

    /* Replication related */
    server.masterauth = NULL;
    server.masterhost = NULL;
    server.masterport = 6379;
    server.master = NULL;
    server.cached_master = NULL;
    server.master_initial_offset = -1;
    server.repl_state = REPL_STATE_NONE;
    server.repl_transfer_tmpfile = NULL;
    server.repl_transfer_fd = -1;
    server.repl_transfer_s = NULL;
    server.repl_syncio_timeout = CONFIG_REPL_SYNCIO_TIMEOUT;
    server.repl_down_since = 0; /* Never connected, repl is down since EVER. */
    server.master_repl_offset = 0;
    server.master_repl_meaningful_offset = 0;

    /* Replication partial resync backlog */
    server.repl_backlog = NULL;
    server.repl_backlog_histlen = 0;
    server.repl_backlog_idx = 0;
    server.repl_backlog_off = 0;
    server.repl_no_slaves_since = time(NULL);
/* ... */
    server.commands = dictCreate(&commandTableDictType,NULL);
    server.orig_commands = dictCreate(&commandTableDictType,NULL);
    populateCommandTable();
    server.delCommand = lookupCommandByCString("del");
    server.multiCommand = lookupCommandByCString("multi");
    server.lpushCommand = lookupCommandByCString("lpush");
    server.lpopCommand = lookupCommandByCString("lpop");
    server.rpopCommand = lookupCommandByCString("rpop");
    server.zpopminCommand = lookupCommandByCString("zpopmin");
    server.zpopmaxCommand = lookupCommandByCString("zpopmax");
    server.sremCommand = lookupCommandByCString("srem");
    server.execCommand = lookupCommandByCString("exec");
    server.expireCommand = lookupCommandByCString("expire");
    server.pexpireCommand = lookupCommandByCString("pexpire");
    server.xclaimCommand = lookupCommandByCString("xclaim");
    server.xgroupCommand = lookupCommandByCString("xgroup");
    server.rpoplpushCommand = lookupCommandByCString("rpoplpush");
/* ... */
    initConfigValues();
}
```
initConfigValues() 函数设置 redisServer 默认参数。

## loadServerConfig()
initConfigValues() 函数加载指定的配置文件
```
// config.c
void loadServerConfig(char *filename, char *options) {
    sds config = sdsempty();
    char buf[CONFIG_MAX_LINE+1];

    /* Load the file content */
    if (filename) {
        FILE *fp;

        if (filename[0] == '-' && filename[1] == '\0') {
            fp = stdin;
        } else {
            if ((fp = fopen(filename,"r")) == NULL) {
                serverLog(LL_WARNING,
                    "Fatal error, can't open config file '%s'", filename);
                exit(1);
            }
        }
        while(fgets(buf,CONFIG_MAX_LINE+1,fp) != NULL)
            config = sdscat(config,buf);
        if (fp != stdin) fclose(fp);
    }
    /* Append the additional options */
    if (options) {
        config = sdscat(config,"\n");
        config = sdscat(config,options);
    }
    loadServerConfigFromString(config);
    sdsfree(config);
}
```
loadServerConfigFromString() 函数从配置文件内容 config 中解析出配置数据，然后将其填入 redisServer 对应的成员中。
```
// config.c
void loadServerConfigFromString(char *config) {
    char *err = NULL;
    int linenum = 0, totlines, i;
    int slaveof_linenum = 0;
    sds *lines;

    lines = sdssplitlen(config,strlen(config),"\n",1,&totlines);

    for (i = 0; i < totlines; i++) {
        sds *argv;
        int argc;

        linenum = i+1;
        lines[i] = sdstrim(lines[i]," \t\r\n");

        /* 跳过注释和空白行 */
        if (lines[i][0] == '#' || lines[i][0] == '\0') continue;

        /* 解析一行配置 */
        argv = sdssplitargs(lines[i],&argc);
        if (argv == NULL) {
            err = "Unbalanced quotes in configuration line";
            goto loaderr;
        }

        /* Skip this line if the resulting command vector is empty. */
        if (argc == 0) {
            sdsfreesplitres(argv,argc);
            continue;
        }
        sdstolower(argv[0]);

        /* 遍历 configs 数组，找到匹配的配置项，设置新值 */
        int match = 0;
        for (standardConfig *config = configs; config->name != NULL; config++) {
            if ((!strcasecmp(argv[0],config->name) ||
                (config->alias && !strcasecmp(argv[0],config->alias))))
            {
                if (argc != 2) {
                    err = "wrong number of arguments";
                    goto loaderr;
                } /* 设置新值 */
                if (!config->interface.set(config->data, argv[1], 0, &err)) {
                    goto loaderr;
                }

                match = 1;
                break;
            }
        }

        if (match) {
            sdsfreesplitres(argv,argc);
            continue;
        }

        /* 直接处理配置项 */
        if (!strcasecmp(argv[0],"bind") && argc >= 2) {
            int j, addresses = argc-1;

            if (addresses > CONFIG_BINDADDR_MAX) {
                err = "Too many bind addresses specified"; goto loaderr;
            }
            /* Free old bind addresses */
            for (j = 0; j < server.bindaddr_count; j++) {
                zfree(server.bindaddr[j]);
            }
            for (j = 0; j < addresses; j++)
                server.bindaddr[j] = zstrdup(argv[j+1]);
            server.bindaddr_count = addresses;
        } else if (!strcasecmp(argv[0],"unixsocketperm") && argc == 2) {
            errno = 0;
            server.unixsocketperm = (mode_t)strtol(argv[1], NULL, 8);
            if (errno || server.unixsocketperm > 0777) {
                err = "Invalid socket file permissions"; goto loaderr;
            }
        } else if (!strcasecmp(argv[0],"save")) {
            if (argc == 3) {
                int seconds = atoi(argv[1]);
                int changes = atoi(argv[2]);
                if (seconds < 1 || changes < 0) {
                    err = "Invalid save parameters"; goto loaderr;
                }
                appendServerSaveParams(seconds,changes);
            } else if (argc == 2 && !strcasecmp(argv[1],"")) {
                resetServerSaveParams();
            }
        } else if (!strcasecmp(argv[0],"dir") && argc == 2) {
            if (chdir(argv[1]) == -1) {
                serverLog(LL_WARNING,"Can't chdir to '%s': %s",
                    argv[1], strerror(errno));
                exit(1);
            }
        } else if (!strcasecmp(argv[0],"logfile") && argc == 2) {
            FILE *logfp;

            zfree(server.logfile);
            server.logfile = zstrdup(argv[1]);
            if (server.logfile[0] != '\0') {
                /* Test if we are able to open the file. The server will not
                 * be able to abort just for this problem later... */
                logfp = fopen(server.logfile,"a");
                if (logfp == NULL) {
                    err = sdscatprintf(sdsempty(),
                        "Can't open the log file: %s", strerror(errno));
                    goto loaderr;
                }
                fclose(logfp);
            }
        } else if (!strcasecmp(argv[0],"include") && argc == 2) {
            loadServerConfig(argv[1],NULL);
        } else if ((!strcasecmp(argv[0],"client-query-buffer-limit")) && argc == 2) {
             server.client_max_querybuf_len = memtoll(argv[1],NULL);
        } else if ((!strcasecmp(argv[0],"slaveof") ||
                    !strcasecmp(argv[0],"replicaof")) && argc == 3) {
            slaveof_linenum = linenum;
            server.masterhost = sdsnew(argv[1]);
            server.masterport = atoi(argv[2]);
            server.repl_state = REPL_STATE_CONNECT;
        } else if (!strcasecmp(argv[0],"requirepass") && argc == 2) {
            if (strlen(argv[1]) > CONFIG_AUTHPASS_MAX_LEN) {
                err = "Password is longer than CONFIG_AUTHPASS_MAX_LEN";
                goto loaderr;
            }
            /* The old "requirepass" directive just translates to setting
             * a password to the default user. The only thing we do
             * additionally is to remember the cleartext password in this
             * case, for backward compatibility with Redis <= 5. */
            ACLSetUser(DefaultUser,"resetpass",-1);
            sds aclop = sdscatprintf(sdsempty(),">%s",argv[1]);
            ACLSetUser(DefaultUser,aclop,sdslen(aclop));
            sdsfree(aclop);
            sdsfree(server.requirepass);
            server.requirepass = sdsnew(argv[1]);
        } else if (!strcasecmp(argv[0],"list-max-ziplist-entries") && argc == 2){
            /* DEAD OPTION */
        } else if (!strcasecmp(argv[0],"list-max-ziplist-value") && argc == 2) {
            /* DEAD OPTION */
        } else if (!strcasecmp(argv[0],"rename-command") && argc == 3) {
            struct redisCommand *cmd = lookupCommand(argv[1]);
            int retval;

            if (!cmd) {
                err = "No such command in rename-command";
                goto loaderr;
            }

            /* If the target command name is the empty string we just
             * remove it from the command table. */
            retval = dictDelete(server.commands, argv[1]);
            serverAssert(retval == DICT_OK);

            /* Otherwise we re-add the command under a different name. */
            if (sdslen(argv[2]) != 0) {
                sds copy = sdsdup(argv[2]);

                retval = dictAdd(server.commands, copy, cmd);
                if (retval != DICT_OK) {
                    sdsfree(copy);
                    err = "Target command name already exists"; goto loaderr;
                }
            }
        } else if (!strcasecmp(argv[0],"cluster-config-file") && argc == 2) {
            zfree(server.cluster_configfile);
            server.cluster_configfile = zstrdup(argv[1]);
        } else if (!strcasecmp(argv[0],"client-output-buffer-limit") &&
                   argc == 5)
        {
            int class = getClientTypeByName(argv[1]);
            unsigned long long hard, soft;
            int soft_seconds;

            if (class == -1 || class == CLIENT_TYPE_MASTER) {
                err = "Unrecognized client limit class: the user specified "
                "an invalid one, or 'master' which has no buffer limits.";
                goto loaderr;
            }
            hard = memtoll(argv[2],NULL);
            soft = memtoll(argv[3],NULL);
            soft_seconds = atoi(argv[4]);
            if (soft_seconds < 0) {
                err = "Negative number of seconds in soft limit is invalid";
                goto loaderr;
            }
            server.client_obuf_limits[class].hard_limit_bytes = hard;
            server.client_obuf_limits[class].soft_limit_bytes = soft;
            server.client_obuf_limits[class].soft_limit_seconds = soft_seconds;
        } else if (!strcasecmp(argv[0],"notify-keyspace-events") && argc == 2) {
            int flags = keyspaceEventsStringToFlags(argv[1]);

            if (flags == -1) {
                err = "Invalid event class character. Use 'g$lshzxeA'.";
                goto loaderr;
            }
            server.notify_keyspace_events = flags;
        } else if (!strcasecmp(argv[0],"user") && argc >= 2) {
            int argc_err;
            if (ACLAppendUserForLoading(argv,argc,&argc_err) == C_ERR) {
                char buf[1024];
                char *errmsg = ACLSetUserStringError();
                snprintf(buf,sizeof(buf),"Error in user declaration '%s': %s",
                    argv[argc_err],errmsg);
                err = buf;
                goto loaderr;
            }
        } else if (!strcasecmp(argv[0],"loadmodule") && argc >= 2) {
            queueLoadModule(argv[1],&argv[2],argc-2);
        } else if (!strcasecmp(argv[0],"sentinel")) {
            /* argc == 1 is handled by main() as we need to enter the sentinel
             * mode ASAP. */
            if (argc != 1) {
                if (!server.sentinel_mode) {
                    err = "sentinel directive while not in sentinel mode";
                    goto loaderr;
                }
                err = sentinelHandleConfiguration(argv+1,argc-1);
                if (err) goto loaderr;
            }
        } else {
            err = "Bad directive or wrong number of arguments"; goto loaderr;
        }
        sdsfreesplitres(argv,argc);
    }

    /* Sanity checks. */
    if (server.cluster_enabled && server.masterhost) {
        linenum = slaveof_linenum;
        i = linenum-1;
        err = "replicaof directive not allowed in cluster mode";
        goto loaderr;
    }

    sdsfreesplitres(lines,totlines);
    return;

loaderr:
    fprintf(stderr, "\n*** FATAL CONFIG FILE ERROR (Redis %s) ***\n",
        REDIS_VERSION);
    fprintf(stderr, "Reading the configuration file, at line %d\n", linenum);
    fprintf(stderr, ">>> '%s'\n", lines[i]);
    fprintf(stderr, "%s\n", err);
    exit(1);
}
```