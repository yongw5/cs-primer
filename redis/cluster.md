## 结点
一个 Redis 集群通常由多个结点（node）组成，在刚开始的时候，每个结点都是互相独立的，它们都处于一个只包含自己的集群中，要组建一个真正可工作的集群，我们必须将各个独立的结点连接起来，构成一个包含多个结点的集群。

向一个结点发送 CLUSTER MEET 命令，可以让结点与 ip 和 port 所指定的结点进行握手，当握手成功时，结点就会将 ip 和 port 所指定的结点添加到自己所在的集群中

clusterNode 结构保存了一个结点的当前状态，比如结点创建时间、结点的名字、结点当前的配置纪元、结点的 ip 地址和端口号等
```
typedef struct clusterNode {
    mstime_t ctime; /* Node object creation time. */
    char name[CLUSTER_NAMELEN]; /* Node name, hex string, sha1-size */
    int flags;      /* CLUSTER_NODE_... */
    uint64_t configEpoch; /* Last configEpoch observed for this node */
    unsigned char slots[CLUSTER_SLOTS/8]; /* slots handled by this node */
    int numslots;   /* Number of slots handled by this node */
    int numslaves;  /* Number of slave nodes, if this is a master */
    struct clusterNode **slaves; /* pointers to slave nodes */
    struct clusterNode *slaveof; /* pointer to the master node. Note that it
                                    may be NULL even if the node is a slave
                                    if we don't have the master node in our
                                    tables. */
    mstime_t ping_sent;      /* Unix time we sent latest ping */
    mstime_t pong_received;  /* Unix time we received the pong */
    mstime_t fail_time;      /* Unix time when FAIL flag was set */
    mstime_t voted_time;     /* Last time we voted for a slave of this master */
    mstime_t repl_offset_time;  /* Unix time we received offset for this node */
    mstime_t orphaned_time;     /* Starting time of orphaned master condition */
    long long repl_offset;      /* Last known repl offset for this node. */
    char ip[NET_IP_STR_LEN];  /* Latest known IP address of this node */
    int port;                   /* Latest known clients port of this node */
    int cport;                  /* Latest known cluster port of this node. */
    clusterLink *link;          /* TCP/IP link with this node */
    list *fail_reports;         /* List of nodes signaling this as failing */
} clusterNode;
```
clusterLink 保存连接结点所需的有关信息，比如套接字描述符。输入和输出缓冲区
```
typedef struct clusterLink {
    mstime_t ctime;             /* Link creation time */
    connection *conn;           /* Connection to remote node */
    sds sndbuf;                 /* Packet send buffer */
    sds rcvbuf;                 /* Packet reception buffer */
    struct clusterNode *node;   /* Node related to this link if any, or NULL */
} clusterLink;
```
clusterState 记录了在当前结点的视角下，集群目前所处的状态，例如集群是在线还是下线，集群包含多少个结点，集群当前的配置纪元
```
typedef struct clusterState {
    clusterNode *myself;  /* This node */
    uint64_t currentEpoch;
    int state;            /* CLUSTER_OK, CLUSTER_FAIL, ... */
    int size;             /* Num of master nodes with at least one slot */
    dict *nodes;          /* Hash table of name -> clusterNode structures */
    dict *nodes_black_list; /* Nodes we don't re-add for a few seconds. */
    clusterNode *migrating_slots_to[CLUSTER_SLOTS];
    clusterNode *importing_slots_from[CLUSTER_SLOTS];
    clusterNode *slots[CLUSTER_SLOTS];
    uint64_t slots_keys_count[CLUSTER_SLOTS];
    rax *slots_to_keys;
    /* The following fields are used to take the slave state on elections. */
    mstime_t failover_auth_time; /* Time of previous or next election. */
    int failover_auth_count;    /* Number of votes received so far. */
    int failover_auth_sent;     /* True if we already asked for votes. */
    int failover_auth_rank;     /* This slave rank for current auth request. */
    uint64_t failover_auth_epoch; /* Epoch of the current election. */
    int cant_failover_reason;   /* Why a slave is currently not able to
                                   failover. See the CANT_FAILOVER_* macros. */
    /* Manual failover state in common. */
    mstime_t mf_end;            /* Manual failover time limit (ms unixtime).
                                   It is zero if there is no MF in progress. */
    /* Manual failover state of master. */
    clusterNode *mf_slave;      /* Slave performing the manual failover. */
    /* Manual failover state of slave. */
    long long mf_master_offset; /* Master offset the slave needs to start MF
                                   or zero if stil not received. */
    int mf_can_start;           /* If non-zero signal that the manual failover
                                   can start requesting masters vote. */
    /* The followign fields are used by masters to take state on elections. */
    uint64_t lastVoteEpoch;     /* Epoch of the last vote granted. */
    int todo_before_sleep; /* Things to do in clusterBeforeSleep(). */
    /* Messages received and sent by type. */
    long long stats_bus_messages_sent[CLUSTERMSG_TYPE_COUNT];
    long long stats_bus_messages_received[CLUSTERMSG_TYPE_COUNT];
    long long stats_pfail_nodes;    /* Number of nodes in PFAIL status,
                                       excluding nodes without address. */
} clusterState;
```

CLUSTEM MEET 命令的实现（结点 A 收到命令，将结点 B 加入集群）
1. A 为 B 创建一个 clusterNode 结构，并将其添加到自己的 clusterState.nodes 字典里面
1. A 向 B 发送一个 MEET 消息
1. B 收到 A 发送的 MEET 消息，为 A 创建一个 clusterNode 结构，并将其添加到自己的 clusterState.nodes 字典里面
1. B 向 A 返回一个 PONG 消息
1. A 收到 B 返回的 PONG 消息，确认 B 收到 MEET 消息
1. A 向 B 返回一条 PING 消息
1. B 收到 A 返回的 PING 消息，握手完成。

之后，结点 A 将 B 通过 Gossip 协议传播给集群中的其他结点，让其他结点也与 B 进行握手。最终，经过一段时间之后，B 会被集群的所有结点认识。

## 槽（slot）指派
Redis 集群通过分片的方式来保存数据库中的 Key-Value 对，集群的整个数据库被分为 16384 个槽，数据库中每个 Key-Value 都属于这 16384 个槽中的一个，集群中的每个结点都可以处理 [0, 16384] 个槽。

通过向结点发送 CLUSTER ADDSLOTS 命令，可以将一个或者多个槽指派给结点负责。

一个结点除了会将自己负责处理的槽记录在 clusterNode.{slots，numslots} 之外，还会将自己的 slots 数组通过消息发送给集群中的其他结点，以此来告知其他结点自己负责处理哪些槽

clusterState.slots 记录了集群中所有 16384 个槽的指派信息，slots 数组包含 16384 个项，每个数组项都是一个指向 clusterNode 结构的指针

## 在集群中执行命令
16384 个槽都被指派后，集群就会进入上线状态，这时客户端就可以像集群中的结点发送数据命令了。接受命令的结点会计算出命令要处理的数据库 Key-Value 属于哪个槽，并检查这个槽是否指派给了自己。
```
def slot_number(key):
    return CRC16(key) & 16383
```
当结点发现 Key-Value 所在的槽并非由自己负责处理的时候，结点就会向客户端返回一个 MOVED 错误，指引客户端转向至正在负责槽的结点

## 重新分片
Redis 集群的重新分片操作可以将任意数量已经由指派给某个结点（源结点）的槽改为指派给另一个结点（目标结点），并且相关槽所属的 Key-Value 也会从源结点转移到目标结点。

Redis 集群重新分片操作是由集群管理软件 redis-trib 负责执行，redis-trib 对集群的单个槽 slot 进行重新分片的步骤如下：
1. redis-trib 对目标结点发送 CLUSTER SETSLOT <slot> IMPORTING <source-id> 命令，让目标结点准备好从源结点导入属于槽 slot 的 Key-Value
1. redis-trib 对源结点发送 CLUSTER SETSLOT <slot> MIGRATING <target-id> 命令，让源结点准备好将属于槽 slot 的 Key-Value 迁移至目标结点
1. redis-trib 向源结点发送 CLUSTER GETKEYSINSLOT <slot> <count> 命令，获得最多 count 个属于槽 slot 的 Key-Value 数据的 Key
1. 对于获得的每个键名，redis-trib 都向源结点发送一个 MIGRATE <target-ip> <target-port> <key-name> 0 <timeout> 命令，将被选中的键原子地从源结点迁移至目标几点
1. 重复步骤 3 和步骤 4，直到源结点保存的所有属于槽 slot 的 Key-Value 数据都被迁移至目标结点为止
1. redis-trib 向集群中的任意一个结点发送 CLUSTER SETSLOT <slot> NODE <target-id> 名，将槽 slot 指派给目标结点，这一指派信息会通过消息发送至整个集群，最终集群中的所有结点都会知道槽 slot 已经指派给了目标结点。

## 复制与故障转移
Redis 集群中的结点分为主结点（master）和从结点（slave），其中主结点用于处理槽，而从结点则用于复制某个主结点，并在被复制的结点下线时，代替下线主结点继续处理命令请求。

向一个结点发送 CLUSTER REPLICATE <node-id> 可以将接受命令的结点成为 node-id 所指的结点的从结点，并开始对主结点进行复制。

集群中的每个结点都会定期地向集群中的其他结点发送 PING 消息，以此来检测对方是否在线，如果接受 PING 消息的结点没有在规定的时间内，向发送 PING 消息的结点返回 PONG 消息，那么发送 PING 消息的结点就会将接受 PING 消息的结点标记为疑似下线 PFAIL。

如果在一个集群中，半数以上负责处理槽的主结点都将某个主结点 x 报告为疑似下线，那么这个主结点 x 将被标记为已下线 FAIL。将主结点 x 标记为 FAIL 的结点会向集群广播一条关于主节点 x 的 FAIL 消息，所有收到这条 FAIL 消息的结点都会立即将 x 标记为 FAIL。

当一个从结点发现自己正在复制的主结点进入 FAIL 状态时，从结点将开始对下线主结点进行故障转移，以下是故障转移的执行步骤：
1. 下线结点的所有从结点里面，会有一个被选中
1. 被选中的从结点会执行 SLAVEOF no one 命令，成为新的主结点
1. 新的主结点将原主节点的槽全部指派给自己
1. 新的主结点向集群广播一条 PONG 消息，通知集群其他结点，自己已经代替原主结点

集群选举新主结点的方法（Raft 领头选举算法）
1. 集群的配置纪元是一个自增计数器，初始值为 0
1. 当集群的某个结点开始一次故障转移操作时，集群配置纪元的值递增 1
1. 对于每个配置纪元，集群里每个负责处理槽的主结点都有一次投票机会，而第一个想主结点要求投票的从结点将获得主结点的投票
1. 当从结点发现自己正在复制的结点进入已下线状态时，从结点向集群广播一条 CLUSTERMSG\_TYPE\_FAILOVER\_AUTH\_REQUEST 消息，要求所有收到此消息、并且具有投票权的主节点向这个从结点投票
1. 如果一个主结点具有投票权（它正在负责处理槽），并且这个主结点尚未投票给其他从结点，那么主结点将向要求投票的从结点返回一条 CLUSTERMSG\_TYPE\_FAILOVER\_AUTH\_ACK 消息，表示主结点支持从结点成为新的主结点
1. 每个参与选举的从结点都会统计接受 CLUSTERMSG\_TYPE\_FAILOVER\_AUTH\_ACK 消息的个数，由此计算自己获得了多少主结点的支持
1. 如果具有投票权的主结点数目为 N 个，则收到支持票数大于等于 N/2+1 的从结点成为新的主结点（最多只有一个）
1. 如果在一个配置纪元里没有选举成功，那么集群进入下一个配置纪元，并再次进行选举，直到选出新的主结点为止。

MEET、PING、PONG 消息的格式