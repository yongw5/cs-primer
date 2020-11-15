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