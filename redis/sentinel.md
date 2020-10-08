## Sentinel
Sentinel（哨兵）是 Redis 的高可用性解决方案：由一个或多个 Sentinel 实例组成的 Sentinel 系统可以监视任意多个主服务器，以及这些主服务器属下的所有从服务器，并在被监视的主服务器进入下线状态时，自动将下线主服务器属下的某个从服务器升级为新的主服务器，然后由新的主服务代替已下线的主服务器继续处理命令请求。

### 启动并初始化 Sentinel
当一个 Sentinel 启动时，需要执行以下步骤：
1. 初始化服务器；
2. 将普通 Redis 服务器使用的代码替换成 Sentinel 专用代码；
3. 初始化 Sentinel 状态
4. 根据给定的配置文件，初始化 Sentinel 的监视主服务器列表
5. 创建连向主服务器的网络连接

Sentinel 本质上是一个运行在特殊模式下的 Redis 服务器，启动 Sentinel 和初始化普通 Redis 服务器相似。不过 Sentinel 并不使用数据库，所以初始化 Sentinel 时不会载入 RDB 文件或者 AOF 文件。

启动 Sentinel 的第二步就是将一部分 Redis 服务器使用的代码替换成 Sentinel 专用代码。比如使用 sentinel.c:REDIS_SENTINEL_PORT 作为端口值，使用 sentinel.c:sentinelcmds 作为服务器的命令表等。

在应用了 Sentinel 专用代码之后，服务器会初始化一个 sentinel.c:sentinelState 结构。这个结构保存了服务器中所有和 Sentinel 功能有关的状态。
```
struct sentinelState {
    uint64_t current_epoch;
    dict *masters;
    int titl;
    int running_scripts;
    mstime_t titl_start_time;
    mstime_t previous_time;
    list *scripts_queue;
} sentinel;
```
sentinelState 中 masters 记录了所有被 Sentinel 监视的主服务器相关信息，其中：Key 是被监视主服务器的名字；Value 是被监视主服务器对应的 sentinel.c:sentinelRedisInstance 结构。

每个 sentinelRedisInstance 代表一个被 Sentinel 监视的 Redis 服务器实例，这个实例可以是主服务器、从服务器，或者另外一个 Sentinel。
```
typedef struct sentinelRedisInstance {
    int flags; // 实例类型
    char *name;
    char *runid;
    unit64_t config_epoch;
    sentinelAddr *addr; // char *ip, int port
    mstime_t down_after_period;
    int quorum;
    int parallel_sync;
    mastime_t failover_timeout;
    ...
} sentinelRedisInstance;
```
初始化 Sentinel 的最后一步是创建连向被监视的主服务器的网络连接，Sentinel 将成为主服务的客户端，它可以向主服务发送命令，并从命令回复中获取相关的信息。对于每个被 Sentinel 监视的主服务器，Sentinel 都会创建两个连向主服务器的一步网络连接：一个是命令连接，用于向主服务器发送命令和接受回复；另一个是订阅连接，这个连接专门用于订阅主服务的 `__sentienl__:hello` 频道。

## 获取主服务器信息
Sentinel 默认会以每 10 秒一次的频率，通过命令连接向被监视的主服务器发送 INFO 命令，并通过分析 INFO 命令的回复来获取主服务器的当前信息：一是主服务器本身的信息，比如 runid 等；另一方面是关于主服务器属下所有从服务器的信息。

### 获取从服务器信息
当 Sentinel 发现主服务器有新的从服务器出现时，Sentinel 除了会为这个新的从服务器创建相应的实例结构之外，Sentinel 还会创建连接到从服务器的命令连接和订阅连接。

在创建命令连接之后，Sentinel 默认以每 10 秒一次的频率，通过命令连接向从服务器发送 INFO 命令。根据 INFO 命令的回复，Sentinel 会提取出一下信息：runid、role、master_host、master_port、master_link_status、slave_priority、slave_repl_offset 等。Sentinel 根据这些信息对从服务器的 sentinelReidsInstance 结构进行更新。

### 向主服务器和从服务器发送信息
在默认情况下，Sentinel 会以每两秒一次的频率，通过命令连接向所有被监视的主服务器和从服务器发送一下格式的命令
```
PUBLICK __sentinel__:hello "<s_ip>,<s_port>,<s_runid>,<s_epoch>,<m_name>,<m_ip>,<m_port>,<m_epoch>"
```
这条命令向服务器的 `__sentinel__:hello`频道发送了一条消息，消息的内容由多个参数组成：气筒以 s_ 开头的参数记录的是 Sentinel 本身的信息，而以 m_ 开头的参数记录的则是主服务器的信息。

### 接收来自主服务器和从服务器的频道信息

### 更新 Sentinel 字典

### 创建连向其他 Sentinel 的命令连接

### 检测主观下线状态
某个实例在 down-after-milliseconds 毫秒内，连续向 sentinel 返回无效回复，那么 Sentinel 会修改这个实例所对应的实例结构，在结构的 flags 属性中打开 SRI_S_DOWN 标志，以表示这个实例已经进入主观下线状态。

### 检查客观下线状态
当 Sentinel 将一个主服务器判断为主观下线之后，为了确认这个主服务器是否真的下线了，它会想同样监视这个主服务器的其他 Sentinel 进行询问，看他们是否也认为这个主服务已经进入下线状态。当 Sentinel 从其他 Sentinel 那里接收到足够数量的已下线判断之后，Sentinel 就会将从服务器判定为客观下线，并对主服务器执行故障转移

### 选举领头 Sentinel
- 所有在线的 Sentinel 都有被选为领头 Sentinel 的资格
- 每次进行领头 Sentinel 选举之后，不论选举是否成功，所有 Sentinel 的配置纪元的值都会自增一次
- 在一个配置纪元里面，所有 Sentinel 都有一次将某个 Sentinel 设置为局部领头 Sentinel 的机会，并且局部领头一旦设置，在这个配置纪元里面就不能再更改
- 每个发现主服务器进入客观下线的 Sentinel 都会要求其他 Sentinel 将自己设置为局部领头 Sentinel
- 当一个 Sentinel 向另一个 Sentinel 发送 SENTINEL is-master-down-by-addr 命令，并且命令中的 runid 参数不是 * 符号而是源 Sentinel 的 runid 时，表示源 Sentinel 要求目标 Sentinel 将前者设置为后者的局部领头 Sentinel
- Sentinel 设置局部领头 Sentinel 的规则是先到先得：最先向目标 Sentinel 发送设置要求的源 Sentinel 将成为目标 Sentinel 的局部领头 Sentinel，而后接收到的所有设置要求都会被目标 Sentienl 拒绝

### 故障转移
1. 在已下线主服务器属下所有从服务器里面，挑选一个从服务器，并将其转换为主服务器
2. 让已下线主服务器属下的所有从服务器改为复制新的主服务器
3. 将已下线主服务设置为新的主服务器的从服务器，当这个旧的主服务器重新上线时，他就会成为新的主服务器的从服务器