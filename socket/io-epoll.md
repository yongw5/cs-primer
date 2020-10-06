## 基于 select() 的 IO 复用技术速度慢的原因
调用 select() 函数后常见的针对所有文件描述符的循环语句：调用 select() 函数后，并不是把发生变化的文件描述符单独集中到一起，而是通过棺材作为监视对象的 fd_set 变量的变化，找出发生变化的文件描述符，因此无法避免针对所有监视所有监视对象的循环语句

每次调用 select() 函数时都要向该函数传递监视对象信息：作为监视对象的 fd_set 变量会发生变化，所以调用 select() 函数前应复制并保存原有信息，并在每次调用 select() 函数时传递新的监视对象信息

## 实现 epoll() 时必要的函数和结构体
```
#include <sys/epoll.h>
typedef union epoll_data {
  void       *ptr;
  int        fd;
  __uint32_t u32;
  __uint64_t u64
} epoll_data_t;
struct epoll_even {
  __uint32_t events;
  epoll_data data;
};

/* @param
 * size：epoll 实例的大小，只是建议值，仅供操作系统参看
 * return：成功时返回 epoll 文件描述符（调用 close() 关闭），失败时返回 -1*/
int epoll_create(int size);

/* @param
 * epfd：用于注册监视对象的 epoll 例程的文件描述符
 * op：用于指定监视对象的添加、删除或更改操作
 *     EPOLL_CTL_ADD：将文件描述服注册到 epoll 例程
 *     EPOLL_CTL_DEL：从 epoll 例程中删除文件描述符
 *     EPOLL_CTL_MOD：更改注册的文件描述符的关注事件发生情况
 * fd：需要注册的监视对象文件描述符
 * event：监视对象的事件类型
 *     EPOLLIN：需要读取数据的情况
 *     EPOLLOUT：输出缓冲为空，可以立即发送数据的情况
 *     EPOLLPRI：收到 OOB 数据的情况
 *     EPOLLRDHUP：断开连接或半关闭的情况，在边缘触发方式下非常有用
 *     EPOLLERR：发生错误的情况
 *     EPOLLET：以边缘触发的方式得到事件通知
 *     EPOLLONESHOT：发生一次事件后，相应文件描述符不再收到事件通知。
 *                   因此需要向 epoll_ctl() 函数的第二个参数传递 EPOLL_CTL_MOD，再次设置事件
 * return：成功时返回0，失败返回-1
 */
int epoll_ctl(int epfd, int op, int fd, struct epoll *event);

/* @param
 * epfd：表示事件发生监视范围的 epoll 例程的文件描述符
 * events：保存发生事件的文件描述符集合的结构体地址
 * maxevents：第二个参数中可以保存的最大事件数
 * timeout：以1/1000 秒为单位的等待时间，传递 -1 时，一直等待直到发生事件
 * return：成功返回发生事件的文件描述符，失败返回 -1
 */
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timout);
```

## 条件触发和边缘触发的区别在于发生事件的时间点
- 条件触发方式中，只要输入缓冲有数据就会一直通知该事件
- 边缘触发中输入缓冲收到数据时仅注册 1 次该事件。即使输入缓冲中还留有数据，也不会再进行注册
- epoll() **默认以条件触发方式工作**

## 边缘触发的服务器端实现中必知的两点
通过 errno 变量验证错误原因：read() 函数返回 -1， errno 的值为 EAGAIN 时，说明没有数据可读

为了完成非阻塞（Non-blocking）IO，更改套接字特性
```
// linux提供更改或读取文件属性的方法
#include <fcntl.h>
/* @param
* filedes：属性更改目标的文件描述符
* cmd：表示函数调用的目的
*    F_GETFL：获得第一个参数所指文件描述符属性（int型）
*    F_SETFL：更改文件描述符属性
* return：成功返回cmd参数相关值，失败返回-1
*/
int fcntl(int filedes, int cmd, ...);

// 将文件（套接字）改为非阻塞模式
int flag = fcntl(sockfd, F_GETFL, 0);
fcntl(sockfd, F_SETFL, flag|O_NONBLOCK)
```