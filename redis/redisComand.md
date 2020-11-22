## redisCommand
Redis 中利用 redisComman 数据结构请求命令，其定义如下：
```
// server.h
typedef void redisCommandProc(client *c);
typedef int *redisGetKeysProc(struct redisCommand *cmd, robj **argv, int argc, int *numkeys);
struct redisCommand {
    char *name;
    redisCommandProc *proc;
    int arity;
    char *sflags;
    uint64_t flags;
    redisGetKeysProc *getkeys_proc;
    int firstkey; /* The first argument that's a key (0 = no keys) */
    int lastkey;  /* The last argument that's a key */
    int keystep;  /* The step between first and last key */
    long long microseconds, calls;
    int id;
};
```
其中 proc 函数指针，指向请求 name 对应的处理函数。

## redisCommandTable
redisCommandTable 是一个 redisCommand 数组，在 server.c 中定义，保存了 Redis 支持的所有命令。
```
// server.c
struct redisCommand redisCommandTable[] = {
    {"module",moduleCommand,-2,
     "admin no-script",
     0,NULL,0,0,0,0,0,0},

    {"get",getCommand,2,
     "read-only fast @string",
     0,NULL,1,1,1,0,0,0},

    /* Note that we can't flag set as fast, since it may perform an
     * implicit DEL of a large key. */
    {"set",setCommand,-3,
     "write use-memory @string",
     0,NULL,1,1,1,0,0,0},
/* ... */
    {"del",delCommand,-2,
     "write @keyspace",
     0,NULL,1,-1,1,0,0,0},
/* ... */
};
```

## populateCommandTable()
redisServer.commands 保存了 Redis 支持的所有命令。commands 是一个 dict 数据结构，其中 Key 为命令的名字，Value 是 redisCommand 数据结构。populateCommandTable() 函数用于将 Redis 支持的所有命令及其实现填入 commands 字典。populateCommandTable() 函数将 redisCommandTable 的内容添加到 server.commands 中。
```
// server.c
void populateCommandTable(void) {
    int j;
    int numcommands = sizeof(redisCommandTable)/sizeof(struct redisCommand);

    for (j = 0; j < numcommands; j++) {
        struct redisCommand *c = redisCommandTable+j;
        int retval1, retval2;

        if (populateCommandTableParseFlags(c,c->sflags) == C_ERR)
            serverPanic("Unsupported command flag");

        c->id = ACLGetCommandID(c->name); /* Assign the ID used for ACL. */
        retval1 = dictAdd(server.commands, sdsnew(c->name), c);
        retval2 = dictAdd(server.orig_commands, sdsnew(c->name), c);
        serverAssert(retval1 == DICT_OK && retval2 == DICT_OK);
    }
}
```

## lookupCommand\*()
lookupCommand\*() 函数族用于从 server.commands 查找相应的命令。
```
// server.c
struct redisCommand *lookupCommand(sds name) {
    return dictFetchValue(server.commands, name);
}

struct redisCommand *lookupCommandByCString(char *s) {
    struct redisCommand *cmd;
    sds name = sdsnew(s);

    cmd = dictFetchValue(server.commands, name);
    sdsfree(name);
    return cmd;
}

struct redisCommand *lookupCommandOrOriginal(sds name) {
    struct redisCommand *cmd = dictFetchValue(server.commands, name);

    if (!cmd) cmd = dictFetchValue(server.orig_commands,name);
    return cmd;
}
```