## 发布与订阅
当一个客户端执行 SUBSCRIBE 命令订阅某个或某些频道的时候，这个客户端与被订阅频道之间就建立起了一种订阅关系。

Redis 将所有频道的订阅关系都保存在服务器状态的 pubsub_channels 字典里面，这个字典的 Key 是某个被订阅的频道，而 Value 是一个所有订阅这个频道的客户端组成的链表

UNSUBSCRIBE 命令的行为和 SUBSCRIBE 命令的行为正好相反，当一个客户端退订某个或者某些频道的时候，服务器将从 pubsub_channels 中解除客户端与被退订频道之间的关联

模式订阅 PSUBSCRIBE 命令和 SUBSCRIBE 处理相似，只是将订阅关系保存到 pubsub_patterns 里面。

当一个 Redis 客户端执行 PUBLISH <channel> <message> 命令将 message 发送给 channel 的时候，服务器需要执行以下两个动作：
1. 将 message 发送给 channel 频道的所有订阅者
1. 如果有一个或多个 pattern 与 channel 相匹配，那么将 message 发送给 pattern 模式的订阅者