## 概述
同一进程内的所有线程除了共享全局变量外还共享
- 进程指令
- 大多数数据
- 打开的文件（即描述符）
- 当前工作目录
- 用户 ID 和组 ID

不过每个线程有各自的
- 线程 ID
- 寄存器集合，包括程序计数器和栈指针
- 栈（用于存放局部变量和返回地址）
- errno
- 信号掩码
- 优先级

## 基本线程函数：创建和终止
```
#include <pthread>
// 进程创建
int pthread_create(pthread *tid, const pthread_attr_t *attr, 
                   void *(*func)(void*), void *arg);

// 等待一个给定线程终止
int pthread_join(pthread_t *tid, void **status);

// 返回线程自身的 ID
pthread_t pthread_self(void);

// 不会阻塞，tid线程终止时，所有相关资源都被释放
int pthread_detach(pthread_t tid);

// 线程终止
void pthread_exit(void **status);
```

## 线程同步之互斥锁
```
#include <pthread.h>
/* @param
* mutex：保存互斥量的变量地址值
* attr：传递即将创建的互斥量属性，没有特别需要的属性传递 NULL
*    第二个参数为 NULL，等同于宏 PTHREAD_MUTEX_INITIALIZER
* return：成功返回0，失败返回其他值
*/
int pthread_mutex_init(pthread_mutex_t *mutex, 
                       const pthread_mutexattr_t *attr);

/* @param
* mutex：销毁互斥量的变量地址值
* return：成功返回 0，失败返回其他值
*/
int pthead_mutex_destroy(pthread_mutex_t *mutex);

int pthread_mutex_lock(pthread_mutex_t *mutex);  // 成功返回 0，失败返回其他值
int pthread_mutex_unlock(pthread_mutex_t *mutex); // 成功返回 0，失败返回其他值
```

## 线程同步之条件变量
```
#include <pthread.h>
int pthread_cond_wait(pthread_cond_t *cptr, pthread_mutex_t *mptr);
int pthread_cond_signal(pthread_cond_t *cptr);
```

## 线程同步之信号量
```
#include <semaphore.h>
/* @param
* sem：创建信号量的变量地址值
* pshared：创建可由多少个进程共享的信号量，传递 0 表示只允许 1个 进程内部使用
* value：指定初始值
* return：成功返回0，失败返回其他值
*/
int sem_init(sem_t *sem, int pshared, unsigned int value);

/* @param
* sem：销毁信号量的变量地址值
* return：成功返回0，失败返回其他值
*/
int sem_destroy(sem_t *sem);

int sem_post(sem_t *sem);  // +1 成功返回0，失败返回其他值
int sem_wait(sem_t *sem);  // -1 成功返回0，失败返回其他值
```

## 销毁线程的 2 种方法
- 调用 pthread_join() 函数： 调用者阻塞
- 调用 pthread_detach() 函数
  ```
  #include <pthread.h>
  int pthread_detach(pthread_t thread); // 成功返回 0，失败返回其他值
  ```
  - 不会阻塞