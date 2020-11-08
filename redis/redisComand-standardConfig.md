## redisCommand
Redis 中利用 redisComman 数据结构请求命令，其定义如下：
```
// server.h
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

redisServer.commands 保存了 Redis 支持的所有命令。commands 是一个 dict 数据结构，其中 Key 为命令的名字，Value 是 redisCommand 数据结构。populateCommandTable() 函数用于将 Redis 支持的所有命令及其实现填入 commands 字典。

## populateCommandTable()
redisCommandTable 是一个 redisCommand 数组，在 server.c 中定义，保存了 Redis 支持的所有命令。populateCommandTable() 函数将 redisCommandTable 的内容添加到 server.commands 中。
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

## standardConfig
standardConfig 表示 Redis 一项配置，其定义如下。
```
// config.c
typedef struct standardConfig {
    const char *name;
    const char *alias;
    const int modifiable;
    typeInterface interface;
    typeData data;
} standardConfig;
```
其中，typeInterface 是函数接口，用于处理当前配置数据，其定义如下
```
// config.c
typedef struct typeInterface {
    void (*init)(typeData data); /* 初始化函数，server 启动时调用 */
    int (*load)(typeData data, sds *argc, int argv, char **err);
    int (*set)(typeData data, sds value, int update, char **err);
    void (*get)(client *c, typeData data);
    void (*rewrite)(typeData data, const char *name, struct rewriteConfigState *state);
} typeInterface;
```
typeData 表示配置项的值，是一个联合体，可以是布尔、字符串、枚举或者数值中的一种，定义如下：
```
// config.c
typedef union typeData {
    boolConfigData yesno;
    stringConfigData string;
    enumConfigData enumd;
    numericConfigData numeric;
} typeData;
```
boolConfigData、stringConfigData、enumConfigData 和 numericType 定义如下，其中 config 成员指向 redisServer 中某一成员。
```
// config.c
typedef struct boolConfigData {
    int *config; /* 指向 server 相应成员 */
    const int default_value;
    int (*is_valid_fn)(int val, char **err);
    int (*update_fn)(int val, int prev, char **err);
} boolConfigData;

typedef struct stringConfigData {
    char **config;
    const char *default_value;
    int (*is_valid_fn)(char* val, char **err);
    int (*update_fn)(char* val, char* prev, char **err);
    int convert_empty_to_null;
} stringConfigData;

typedef struct enumConfigData {
    int *config;
    configEnum *enum_value;
    const int default_value;
    int (*is_valid_fn)(int val, char **err); 
    int (*update_fn)(int val, int prev, char **err);
} enumConfigData;

typedef enum numericType {
    NUMERIC_TYPE_INT,
    NUMERIC_TYPE_UINT,
    NUMERIC_TYPE_LONG,
    NUMERIC_TYPE_ULONG,
    NUMERIC_TYPE_LONG_LONG,
    NUMERIC_TYPE_ULONG_LONG,
    NUMERIC_TYPE_SIZE_T,
    NUMERIC_TYPE_SSIZE_T,
    NUMERIC_TYPE_OFF_T,
    NUMERIC_TYPE_TIME_T,
} numericType;

typedef struct numericConfigData {
    union {
        int *i;
        unsigned int *ui;
        long *l;
        unsigned long *ul;
        long long *ll;
        unsigned long long *ull;
        size_t *st;
        ssize_t *sst;
        off_t *ot;
        time_t *tt;
    } config;
    int is_memory;
    numericType numeric_type;
    long long lower_bound;
    long long upper_bound;
    const long long default_value;
    int (*is_valid_fn)(long long val, char **err);
    int (*update_fn)(long long val, long long prev, char **err);
} numericConfigData;
```
Redis 定义了一个 standardConfig 数组，设置了 Redis 默认配置
```
// config.c
standardConfig configs[] = {
/* ... */
    /* Integer configs */
    createIntConfig("databases", NULL, IMMUTABLE_CONFIG, 1, INT_MAX, server.dbnum, 16, INTEGER_CONFIG, NULL, NULL),
/* ... */
}
```
宏定义 createIntConfig 为一个 standardConfig 变量赋值，宏定义展开如下
```
{
    .name = ("databases"), 
    .alias = (((void *)0)), 
    .modifiable = (0), 
    .interface = { 
        .init = (numericConfigInit), 
        .set = (numericConfigSet), 
        .get = (numericConfigGet), 
        .rewrite = (numericConfigRewrite) 
    }, 
    .data.numeric = { 
        .lower_bound = (1), 
        .upper_bound = (2147483647), 
        .default_value = (16), 
        .is_valid_fn = (((void *)0)), 
        .update_fn = (((void *)0)), 
        .is_memory = (0), 
        .numeric_type = NUMERIC_TYPE_INT, 
        .config.i = &(server.dbnum) 
    }
}
```
可以看出，databases 默认值是 16

## initConfigValues()
initConfigValues() 函数将 standardConfig 数组 configs 中设置的默认值填入 redisServer 指定的成员
```
void initConfigValues() {
    for (standardConfig *config = configs; config->name != NULL; config++) {
        config->interface.init(config->data);
    }
}
```
以 "databases" 为例，其 interface.init 函数指针指向 numericConfigInit 函数，其定义如下：
```
static void numericConfigInit(typeData data) {
    SET_NUMERIC_TYPE(data.numeric.default_value)
}
```
宏定义 SET_NUMERIC_TYPE 定义如下：
```
// config.c
#define SET_NUMERIC_TYPE(val) \
    if (data.numeric.numeric_type == NUMERIC_TYPE_INT) { \
        *(data.numeric.config.i) = (int) val; \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_UINT) { \
        *(data.numeric.config.ui) = (unsigned int) val; \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_LONG) { \
        *(data.numeric.config.l) = (long) val; \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_ULONG) { \
        *(data.numeric.config.ul) = (unsigned long) val; \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_LONG_LONG) { \
        *(data.numeric.config.ll) = (long long) val; \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_ULONG_LONG) { \
        *(data.numeric.config.ull) = (unsigned long long) val; \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_SIZE_T) { \
        *(data.numeric.config.st) = (size_t) val; \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_SSIZE_T) { \
        *(data.numeric.config.sst) = (ssize_t) val; \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_OFF_T) { \
        *(data.numeric.config.ot) = (off_t) val; \
    } else if (data.numeric.numeric_type == NUMERIC_TYPE_TIME_T) { \
        *(data.numeric.config.tt) = (time_t) val; \
    }
```
可以看到，data.numeric.config.i 被赋值为 val，即 redisServer.dbnum 被赋值为 data.numeric.default_value。而 data.numeric.default_value 被设置为 16。