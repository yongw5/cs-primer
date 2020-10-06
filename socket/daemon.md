## 守护进程
在后台运行且不与任何控制终端关联的进程（关闭标准输入、输出、出错文件描述符）

## syslog() 函数
```
// 从守护进程中登记消息
#include <syslog.h>
void syslog(int priority, const char *message, ...)
```