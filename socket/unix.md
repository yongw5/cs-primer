## UNIX 域协议
在单个主机上执行客户/服务器通信的一种方法

## UNIX 域套接字地址结构
```
#include <sys/un.h>
struct sockaddr_un {
  __SOCKADDR_COMMON (sun_);
  char sun_path[108];		/* Path name.  */
};
```
## socketpair() 函数
```
#include <sys/socket.h>
/* @param
 * family：必须为 AF_LOCAL
 * type：SOCK_STREAM 或 SOCK_DGRAM
 * protocol：必须为 0
 * sockfd：套接字描述符数组
 * return：成功返回非 0，失败返回 -1
 */
int socketpair(int family, int type, int protocol, int sockfd[2]);
```