## Sentinel 基础
Sentinel（哨兵）是 Redis 的高可用性解决方案：由一个或多个 Sentinel 实例组成的 Sentinel 系统可以监视任意多个主服务器，以及这些主服务器属下的所有从服务器，并在被监视的主服务器进入下线状态时，自动将下线主服务器属下的某个从服务器升级为新的主服务器，然后由新的主服务代替已下线的主服务器继续处理命令请求。除此之外，Sentinel 还充当监控、通知、配置提供者等角色。
- 监控（Monitoring）：持续检查主服务器和从服务器是否正常工作；
- 通知（Notification）：Sentinel 提供接口可以通知系统管理员，或者其他人员，某个监控实例--主服务器或者从服务器--运行错误；
- 自动容错（Automatic failover）:如果主服务器没有按预期工作，Sentinel 将启动容错操作，该主服务器的某个从服务器被提升为主服务器，其他从服务器重新配置，成为新主服务器的从服务器。并且将新的服务器地址告知所有的客户以便再次连接。
- 配置提供者（Configuration provider）：客户端连接到 Sentinels，以询问负责给定服务的当前 Redis 主服务器的地址。 如果发生故障转移，Sentinels 将报告新地址。

Sentinel 是一个分布式系统，多个 Sentinel 进程协同运行。
- 当大多数 Sentinel 同意某个主服务器不再可用，才抛出错误检测结果，这降低了发生误判的概率。
- 即使某个或者某个 Sentinel 进程不能正常运行，Sentinel 系统也可以继续正常工作，加强系统容错的健壮性。

有关 Sentinel，需要说明的是
- 至少需要 3 个 Sentinel 实例才能保证 Sentinel 系统的健壮性；
- 三个 Sentinel 实例应该部署时应该相互独立，以独立的方式失效；
- Sentinel + Redis 分布系统并不能保证在服务器在出现错误时，所有收到的写操作都不被丢失。但是 Sentinel 可以将丢失的数据的时间窗口限制到某个时刻。
- Sentinel 和 Docker 或者其他网络地址转换或端口映射工具应当谨慎合并使用。Docker 的端口映射操作，或打断 Sentinel 自动探测其他 Sentinel 实例和某个主服务器所有的从服务器。

## SDOWN 和 ODOWN
Sentinel 认为某个主服务器有两种下线状态，一是主观下线（Subjectively Down，SDOWN），另外一个是客观下线（Objectively Down，ODOWN）。其中，SDOWN 表示单个 Sentinel 认为某个主服务器下线，而 ODOWN 表示多个（至少 quorum 个 Sentinel）认为某个主服务器处于 SDOWN 状态。

从 Sentinel 的角度来说，如果在 down-after-miliseconds 设置的时间内都没有收到某个主服务器发送的 PING 回复，该主服务器将被标记为 SDOWN 状态。主服务器对 PING 命令有效的回复可以是以下一种
- 返回 +PONG 命令
- 返回 -LOADING 错误
- 返回 -MASTERDOWN 错误

Redis 并不是根据某个强一致性算法将 SDOWN 提升为 ODOWN，而是在某个时间范围内，如果某个 Sentinel 大多数 Sentinel 告知某个主服务器处于 SDOWN 状态，那么 SDOWN 状态被提升为 ODOWN 状态。只有主服务器处于 ODOWN 状态，才会进一步采取容错操作。

## 配置 Sentinel
在配置文件中，只需要指定需要监控的主服务器（需要为每个主服务配置一个不同的名字），不需要指定从服务器，从服务器可以被自动探测。Sentinel 会自动将有关从服务器的信息更新至配置文件，并且每当自动容错时，某个从服务器被提升为主服务器，或者一个新的 Sentinel 实体被发现，都会更新配置。一个简单的、典型的 Sentinel 配置文件如下所示：
```
sentinel monitor mymaster 127.0.0.1 6379 2
sentinel down-after-milliseconds mymaster 60000
sentinel failover-timeout mymaster 180000
sentinel parallel-syncs mymaster 1

sentinel monitor resque 192.168.1.3 6380 4
sentinel down-after-milliseconds resque 10000
sentinel failover-timeout resque 180000
sentinel parallel-syncs resque 5
```
上述配置文件指定了两个 Redis 实例集，一个叫做 mymaster，另一个叫做 resque，每个包含一个主服务器和未知个数的从服务器。sentinel monitor 语法如下
```
sentinel monitor <master-group-name> <ip> <port> <quorum>
```
quorum 参数表示 quotum 个 Sentinel 实例认为某个主服务器不可达，便开启自动容错。不过，quorum 只用于错误检测阶段，某个从服务器提升为主服务器，仍然需要得到大多数 Sentinel 实例的支持。

其他设置语法如下
```
sentinel <option-name> <master-name> <option-value>
```
其中，上述配置文件中，down-after-milliseconds 表示某个主服务器在特定时间后仍然不可达，Sentinel 将认为其下线。 parallel-syncs 表示在容错后，从服务器并行重配置新主服务器的数目。

TILT 模式
Sentinel 强依赖于系统时间，例如为了判断某个实例是否可用，需要记录该实例上一次回复 PING 命令的时间，然后将其和当前时间比较以确定该实例“失联”时间。而如果系统时间出现非预期的变化，或者系统非常忙碌，亦或是进程因为某种原因被阻塞，Sentinel 将可能有意料之外的行为。

TILT 模式是一种特殊的“保护”模式，当检测到会降低系统可靠性的异常事件发生时，Sentinel 进入该模式。Sentinel 计时器中断函数通常每秒调用 10 次，因此我们希望定时器两次中断的时间差控制在 100 毫秒。

Sentinel 的做法是记录上一次调用定时器中断函数的时间戳，并将其与当前调用时间戳比较。如果时间差为负数或者非常大（2 秒或者更长时间），则进入 TILT 模式（如果已经进入，则延迟退出）。

当 Sentinel 进入 TILT 模式，做出如下改变：
1. 停止所有动作；
1. 消极回复有关 is-master-down-by-addr 的请求，因为其故障检测的能力已经不可靠；

如果 30 秒内一切正常，Sentinel 则退出 TILT 模式。

## Raft 算法
https://www.cnblogs.com/myd620/p/7811156.html

Raft协议描述的节点共有三种状态：Leader, Follower, Candidate。在系统运行正常的时候只有Leader和Follower两种状态的节点。一个Leader节点，其他的节点都是Follower。Candidate是系统运行不稳定时期的中间状态，当一个Follower对Leader的的心跳出现异常，就会转变成Candidate，Candidate会去竞选新的Leader，它会向其他节点发送竞选投票，如果大多数节点都投票给它，它就会替代原来的Leader，变成新的Leader，原来的Leader会降级成Follower。

在分布式系统中，各个节点的时间同步是一个很大的难题，但是为了识别过期时间，时间信息又必不可少。Raft协议为了解决这个问题，引入了term（任期）的概念。Raft协议将时间切分为一个个的Term，可以认为是一种“逻辑时间”。

Raft采用心跳机制触发Leader选举。系统启动后，全部节点初始化为Follower，term为0.节点如果收到了RequestVote或者AppendEntries，就会保持自己的Follower身份。如果一段时间内没收到AppendEntries消息直到选举超时，说明在该节点的超时时间内还没发现Leader，Follower就会转换成Candidate，自己开始竞选Leader。一旦转化为Candidate，该节点立即开始下面几件事情：

1、增加自己的term。
2、启动一个新的定时器。
3、给自己投一票。
4、向所有其他节点发送RequestVote，并等待其他节点的回复。
如果在这过程中收到了其他节点发送的AppendEntries，就说明已经有Leader产生，自己就转换成Follower，选举结束。

如果在计时器超时前，节点收到多数节点的同意投票，就转换成Leader。同时向所有其他节点发送AppendEntries，告知自己成为了Leader。

每个节点在一个term内只能投一票，采取先到先得的策略，Candidate前面说到已经投给了自己，Follower会投给第一个收到RequestVote的节点。每个Follower有一个计时器，在计时器超时时仍然没有接受到来自Leader的心跳RPC, 则自己转换为Candidate, 开始请求投票，就是上面的的竞选Leader步骤。

如果多个Candidate发起投票，每个Candidate都没拿到多数的投票（Split Vote），那么就会等到计时器超时后重新成为Candidate，重复前面竞选Leader步骤。

Raft协议的定时器采取随机超时时间，这是选举Leader的关键。每个节点定时器的超时时间随机设置，随机选取配置时间的1倍到2倍之间。由于随机配置，所以各个Follower同时转成Candidate的时间一般不一样，在同一个term内，先转为Candidate的节点会先发起投票，从而获得多数票。多个节点同时转换为Candidate的可能性很小。即使几个Candidate同时发起投票，在该term内有几个节点获得一样高的票数，只是这个term无法选出Leader。由于各个节点定时器的超时时间随机生成，那么最先进入下一个term的节点，将更有机会成为Leader。连续多次发生在一个term内节点获得一样高票数在理论上几率很小，实际上可以认为完全不可能发生。一般1-2个term类，Leader就会被选出来。

## Sentinel的选举流程
Sentinel集群正常运行的时候每个节点epoch相同，当需要故障转移的时候会在集群中选出Leader执行故障转移操作。Sentinel采用了Raft协议实现了Sentinel间选举Leader的算法，不过也不完全跟论文描述的步骤一致。Sentinel集群运行过程中故障转移完成，所有Sentinel又会恢复平等。Leader仅仅是故障转移操作出现的角色。

选举流程
1、某个Sentinel认定master客观下线的节点后，该Sentinel会先看看自己有没有投过票，如果自己已经投过票给其他Sentinel了，在2倍故障转移的超时时间自己就不会成为Leader。相当于它是一个Follower。
2、如果该Sentinel还没投过票，那么它就成为Candidate。
3、和Raft协议描述的一样，成为Candidate，Sentinel需要完成几件事情
1）更新故障转移状态为start
2）当前epoch加1，相当于进入一个新term，在Sentinel中epoch就是Raft协议中的term。
3）更新自己的超时时间为当前时间随机加上一段时间，随机时间为1s内的随机毫秒数。
4）向其他节点发送is-master-down-by-addr命令请求投票。命令会带上自己的epoch。
5）给自己投一票，在Sentinel中，投票的方式是把自己master结构体里的leader和leader_epoch改成投给的Sentinel和它的epoch。
4、其他Sentinel会收到Candidate的is-master-down-by-addr命令。如果Sentinel当前epoch和Candidate传给他的epoch一样，说明他已经把自己master结构体里的leader和leader_epoch改成其他Candidate，相当于把票投给了其他Candidate。投过票给别的Sentinel后，在当前epoch内自己就只能成为Follower。
5、Candidate会不断的统计自己的票数，直到他发现认同他成为Leader的票数超过一半而且超过它配置的quorum（quorum可以参考《redis sentinel设计与实现》）。Sentinel比Raft协议增加了quorum，这样一个Sentinel能否当选Leader还取决于它配置的quorum。
6、如果在一个选举时间内，Candidate没有获得超过一半且超过它配置的quorum的票数，自己的这次选举就失败了。
7、如果在一个epoch内，没有一个Candidate获得更多的票数。那么等待超过2倍故障转移的超时时间后，Candidate增加epoch重新投票。
8、如果某个Candidate获得超过一半且超过它配置的quorum的票数，那么它就成为了Leader。
9、与Raft协议不同，Leader并不会把自己成为Leader的消息发给其他Sentinel。其他Sentinel等待Leader从slave选出master后，检测到新的master正常工作后，就会去掉客观下线的标识，从而不需要进入故障转移流程。
关于Sentinel超时时间的说明
Sentinel超时机制有几个超时概念。

failover_start_time 下一选举启动的时间。默认是当前时间加上1s内的随机毫秒数
failover_state_change_time 故障转移中状态变更的时间。
failover_timeout 故障转移超时时间。默认是3分钟。
election_timeout 选举超时时间，是默认选举超时时间和failover_timeout的最小值。默认是10s。
Follower成为Candidate后，会更新failover_start_time为当前时间加上1s内的随机毫秒数。更新failover_state_change_time为当前时间。

Candidate的当前时间减去failover_start_time大于election_timeout，说明Candidate还没获得足够的选票，此次epoch的选举已经超时，那么转变成Follower。需要等到mstime() - failover_start_time < failover_timeout\*2的时候才开始下一次获得成为Candidate的机会。

如果一个Follower把某个Candidate设为自己认为的Leader，那么它的failover_start_time会设置为当前时间加上1s内的随机毫秒数。这样它就进入了上面说的需要等到mstime() - failover_start_time < failover_timeout\*2的时候才开始下一次获得成为Candidate的机会。

因为每个Sentinel判断节点客观下线的时间不是同时开始的，一般都有先后，这样先开始的Sentinel就更有机会赢得更多选票，另外failover_state_change_time为1s内的随机毫秒数，这样也把各个节点的超时时间分散开来。本人尝试过很多次，Sentinel间的Leader选举过程基本上一个epoch内就完成了