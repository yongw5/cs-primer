## 设计模式总结
### 八大设计原则
- 依赖倒置原则（DIP）
- 开放封闭原则（OCP）
- 单一职责原则（SRP）
- Liskov 替换原则（LSP）
- 接口隔离原则（ISP）
- 对象组合由于类继承
- 封装变化点
- 面向接口编程

### 重构技法
- 静态-->多态
- 早绑定-->晚绑定
- 继承-->组合
- 编译时依赖-->运行时依赖
- 紧耦合-->松耦合

### 什么时候不用模式
- 代码可读性很差时
- 需求理解还很浅时
- 变化没有显现时
- 不是系统的关键依赖点
- 项目没有复用价值时
- 项目将要发布时

### 经验只谈
- 不要为模式而模式
- 关注抽象类和接口
- 清理变化点和稳定点
- 审视依赖关系
- 要有 Framework 和 Application 的区隔思维
- 良好的设计是演化的结果

### 设计模式成长之路
- “手中无剑，心中无剑”：见模式而不知
- “手中有剑，心中无剑”：可以识别模式，作为应用开发人员使用模式
- “手中有剑，心中有剑”：作为框架开发人员为应用设计某些模式
- “手中无剑，心中有剑”：忘掉模式，只有原则